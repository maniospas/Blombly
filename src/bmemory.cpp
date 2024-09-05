// Memory.cpp
#include "BMemory.h"
#include "common.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include <unordered_set>  

tsl::hopscotch_set<BMemory*> memories;
std::atomic<int> countUnrealeasedMemories;
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    bbassert(memories.size()==0, "There are "+std::to_string(memories.size())+" leftover memory contexts");
    bbassert(countUnrealeasedMemories.load()==0, "There are "+std::to_string(countUnrealeasedMemories.load())+" leftover memory contexts");
}

BMemory::BMemory(BMemory*  par, int expectedAssignments) : parent(par), allowMutables(true), fastLastAccessId(-1), countDependencies(0) {
    //std::cout << "inited "<<this<<"\n";
    //memories.insert(this);
    countUnrealeasedMemories++;
    data.reserve(expectedAssignments);
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for memory read/write");
    }
}

bool BMemory::isOrDerivedFrom(BMemory*  memory) const{
    if(memory==this)
        return true;
    if(parent)
        return parent->isOrDerivedFrom(memory);
    return false;
}


void BMemory::release() {
    //std::cout << "releasing "<<this<<"\n";
    //fastLastAccessId = -1;
    std::string err;
    std::string destroyerr;
    for(const auto& element : data) {
        Data* dat = element.second;
        if(dat && dat->getType()==FUTURE) {
            Future* prevRet = (Future*)dat;
            try {
                data[element.first] = prevRet->getResult();
            }
            catch(const BBError& e) {
                if(!destroyerr.size()) 
                    destroyerr += "Error occured during the execution of spawned code.";
                destroyerr += std::string("\n ( \x1B[31m ERROR \033[0m ) ")+e.what();
            }
            data[element.first] = nullptr;
            attached_threads.erase(prevRet);
            if(dat->isDestroyable)
                delete dat;
        }
    }
    for(Future* thread : attached_threads) 
        thread->getResult();
    //attached_threads.clear();
    // automatically performed in the delete of locals/data
    for(const auto& element : data) {
        Data* dat = element.second;
        if(!dat)
            continue;
        //std::cout<<"deleting "<<variableManager.getSymbol(element.first) << " "<<element.second<<"\n";
        if(dat->getType()==ERRORTYPE && !((BError*)dat)->isConsumed()){
            if(!err.size())
                err += "Intercepted error not handled."
                        "\n   \033[33m!!!\033[0m One or more errors that were intercepted (with `try` were"
                        "\n       neither handled with a `catch` clause nor converted to bool or str."
                        "\n       Neglecting to catch may not be an issue, as the `try` may be meant to"
                        "\n       intercept `return` values only and cause this message otherwise."
                        "\n       All the uncought errors are listed below.";
            err += "\n ( \x1B[31m ERROR \033[0m ) "+dat->toString();
        }
        if(dat->isDestroyable) {
            try {
                delete dat;
            }
            catch(const BBError& e) {
                if(!destroyerr.size()) 
                    destroyerr += "Error occured during the destruction of nested object.";
                destroyerr += std::string("\n ( \x1B[31m ERROR \033[0m ) ")+e.what();
            }
        }
    }
    //data.clear();
    if(err.size() || destroyerr.size()) {
        if(err.size() && destroyerr.size()) {
            bberror(err+"\n\nAnother error occured simultaneously to the above\n\n"+destroyerr);
        }
        else if(err.size())
            bberror(err);
        else
            bberror(destroyerr);
    }
}

// Destructor to ensure that all threads are finished
BMemory::~BMemory() {
    release();
    countUnrealeasedMemories--;
    //memories.erase(memories.find(this));
    pthread_mutex_destroy(&memoryLock);
}

int BMemory::size() const {
    return data.size();
}

// Lock the memory for thread-safe access
void BMemory::lock() {
    /*if (parent) {
        parent->lock();
    }*/
    pthread_mutex_lock(&memoryLock);
}

// Unlock the memory
void BMemory::unlock() {
    pthread_mutex_unlock(&memoryLock);
    /*if (parent) {
        parent->unlock();
    }*/
}

// Get a data item with mutability check
Data* BMemory::get(int item) {
    // DO NOT COMPUTE fastLastAccessId for simple gets, as these can be accessed from different threads
    //if(fastLastAccessId==item && fastLastAccess->getType() != FUTURE) {
    //    return fastLastAccess;
    //}
    Data* ret = data[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
        if(prevRet->isDestroyable)
            delete prevRet;
        return ret;
    }

    // If not found locally, check parent memory
    if (!ret && parent) 
        ret = parent->get(item, allowMutables);

    // Missing value error
    if (!ret) {
        bberror("Missing value: " +variableManager.getSymbol(item));
    }

    return ret;
}


bool BMemory::contains(int item) const {
    //if(fastLastAccessId==item)
    //    return fastLastAccess!=nullptr;
    auto it = data.find(item);
    if(it==data.end())
        return false;
    return it->second!=nullptr;
}

void BMemory::setFinal(int item) {
    finals.insert(item);
}

bool BMemory::isFinal(int item) const {
    return finals.find(item)!=finals.end();
}

// Get a data item, optionally allowing mutable values
Data* BMemory::get(int item, bool allowMutable) {
    /*if(fastLastAccessId==item && fastLastAccess->getType() != FUTURE) {
        return fastLastAccess;
    }*/
    Data* ret = data[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
        if(prevRet->isDestroyable)
            delete prevRet;
        return ret;
    }

    if(ret) {
        bbassert(allowMutable || isFinal(item), 
                    "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item)
                    +"\n   \033[33m!!!\033[0m Either declare this in the current scope"
                    +"\n       or make its original declaration final.");
    }
    else {
        // If not found locally, check parent memory
        if (parent) 
            ret = parent->get(item, allowMutables && allowMutable);
    }

    // Missing value error
    if (!ret) {
        bberror("Missing value: " + variableManager.getSymbol(item));
    }

    return ret;
}

Data* BMemory::getOrNullShallow(int item) {
    /*if(fastLastAccessId==item && fastLastAccess->getType() != FUTURE) {
        return fastLastAccess;
    }*/
    auto it = data.find(item);
    if(it==data.end())
        return nullptr;
    Data* ret = it->second;
    //std::cout << variableManager.getSymbol(item)<<"\n";
    if(ret && ret->getType()==FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
        if(prevRet->isDestroyable)
            delete prevRet;
        return ret;
    }
    return ret;
}

// Get a data item or return nullptr if not found
Data* BMemory::getOrNull(int item, bool allowMutable) {
    /*if(fastLastAccessId==item && fastLastAccess->getType() != FUTURE) {
        return fastLastAccess;
    }*/
    Data* ret = data[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
        if(prevRet->isDestroyable)
            delete prevRet;
        return ret;
    }

    // Handle mutability restrictions
    if (ret && !allowMutable && !isFinal(item)) {
        bberror("Mutable symbol cannot be accessed from a nested block: "+variableManager.getSymbol(item)
                +"\n   \033[33m!!!\033[0m Either declare this in the current scope\n       or make its original declaration final.");
        return nullptr;
    }

    // If not found locally, check parent memory
    if (!ret && parent) {
        ret = parent->getOrNull(item, allowMutables && allowMutable);
    }

    return ret;
}

void BMemory::removeWithoutDelete(int item) {
    data[item] = nullptr;
}

/*void BMemory::set(int item, Data* value) {
    fastLastAccessId = item;
    fastLastAccess = value;
    Data* prev = getOrNullShallow(item);
    if(prev==value) 
        return;
    data[item] = value;
    if (prev) {
        if(prev->isDestroyable)
            delete prev;
        if(isFinal(item)) {
            bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
            return;
        }
    }
}*/


void BMemory::unsafeSet(int item, Data*value, Data* prev) {
    //fastLastAccessId = item;
    //fastLastAccess = value;
    if(prev==value)
        return;
    if (prev) {
        if(isFinal(item)) {
            if(value && value->isDestroyable)
                delete value;
            bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
            return;
        }
        if(prev->isDestroyable) {
            delete prev;
        }
    }
    //std::cout << "Setting" << variableManager.getSymbol(item) << " "<<value->toString()<<"\n";
    data[item] = value;
}

// Pull data from another Memory object
void BMemory::pull(BMemory* other) {
    for (auto& it : other->data) {
        if(it.second)
            unsafeSet(it.first, it.second->shallowCopyIfNeeded(), getOrNullShallow(it.first));
    }
}

// Replace missing values with those from another Memory object
void BMemory::replaceMissing(BMemory* other) {
    for (auto& it : other->data) {
        int item = it.first;
        Data* existing = getOrNullShallow(item);
        if (existing==nullptr) {
            Data* dat = it.second;
            if(dat)
                unsafeSet(item, dat->shallowCopyIfNeeded(), nullptr);
        }
    }
}

// Detach this memory from its parent
void BMemory::detach() {
    for(const auto& element : data) {
        if(element.second && element.second->getType()==FUTURE) {
            Future* prevRet = (Future*)element.second;
            data[element.first] = prevRet->getResult();
            attached_threads.erase(prevRet);
            if(prevRet->isDestroyable)
                delete prevRet;
        }
    }
    for(Future* thread : attached_threads) 
        thread->getResult();

    allowMutables = false;
    parent = nullptr;
}

// Detach this memory from its parent but retain reference
void BMemory::detach(BMemory*  par) {
    for(const auto& element : data) {
        if(element.second && element.second->getType()==FUTURE) {
            Future* prevRet = (Future*)element.second;
            data[element.first] = prevRet->getResult();
            attached_threads.erase(prevRet);
            if(prevRet->isDestroyable)
                delete prevRet;
        }
    }
    for(Future* thread : attached_threads) 
        thread->getResult();
        
    allowMutables = false;
    parent = par;
}