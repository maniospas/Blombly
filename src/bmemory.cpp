// Memory.cpp
#include "BMemory.h"
#include "common.h"
#include "data/BError.h"
#include "data/Future.h"

extern VariableManager variableManager;

BMemory::BMemory(const std::shared_ptr<BMemory>& par, int expectedAssignments) : parent(par), allowMutables(true) {
    if(parent) {
        mapPool = parent->mapPool;
        if(!mapPool->empty()) {
            data = mapPool->top();
            mapPool->pop();
            data->reserve(expectedAssignments);
        }
        else {
            data = new tsl::hopscotch_map<int, Data*>();
            data->reserve(expectedAssignments);
        }
    }
    else {
        mapPool = new std::stack<tsl::hopscotch_map<int, Data*>*>();
        data = new tsl::hopscotch_map<int, Data*>();
        data->reserve(expectedAssignments);
    }
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for memory read/write");
    }
}

bool BMemory::isOrDerivedFrom(const std::shared_ptr<BMemory>& memory) const{
    if(memory.get()==this)
        return true;
    if(parent)
        return parent->isOrDerivedFrom(memory);
    return false;
}


void BMemory::releaseNonFinals() {
    for(Future* thread : attached_threads) 
        thread->getResult();
    attached_threads.clear();
    std::string err;
    // automatically performed in the delete of locals/data
    for(const auto& element : *data) {
        if(!element.second || isFinal(element.first))
            continue;
        Data* dat = element.second;
        if(dat->getType()==ERRORTYPE && !((BError*)dat)->isConsumed()){
            if(!err.size())
                err += "Intercepted error not handled.\n   \033[33m!!!\033[0m One or more errors that were intercepted with `try`\n       were neither handled with a `catch` clause nor converted to bool or str.\n       This is not necessarily an issue, as the `try` may be meant to\n       intercept `return` values only and cause this message otherwise.\n       The errors are listed below.";
            err += "\n ( \x1B[31m ERROR \033[0m ) "+dat->toString();
        }
        if(dat->isDestroyable) {
            //std::cout << "deleting\n";
            //std::cout << "#"<<element.second << "\n";
            //std::cout << element.second->toString() << "\n";
            dat->isDestroyable = false;
            delete dat;
        }
    }
    data->clear();
    if(err.size())
        bberror(err);
}


void BMemory::release() {
    for(Future* thread : attached_threads) 
        thread->getResult();
    attached_threads.clear();
    std::string err;
    // automatically performed in the delete of locals/data
    for(const auto& element : *data) {
        if(!element.second)
            continue;
        Data* dat = element.second;
        if(dat->getType()==ERRORTYPE && !((BError*)dat)->isConsumed()){
            if(!err.size())
                err += "Intercepted error not handled.\n   \033[33m!!!\033[0m One or more errors that were intercepted with `try`\n       were neither handled with a `catch` clause nor converted to bool or str.\n       This is not necessarily an issue, as the `try` may be meant to\n       intercept `return` values only and cause this message otherwise.\n       The errors are listed below.";
            err += "\n ( \x1B[31m ERROR \033[0m ) "+dat->toString();
        }
        if(dat->isDestroyable) {
            //std::cout << "deleting\n";
            //std::cout << "#"<<element.second << "\n";
            //std::cout << element.second->toString() << "\n";
            dat->isDestroyable = false;
            delete dat;
        }
    }
    data->clear();
    if(err.size())
        bberror(err);
}

// Destructor to ensure that all threads are finished
BMemory::~BMemory() {
    release();
    // transfer remainder map pool elements to the parent
    if(parent) {
        // do this afterwards so that recalling the same methods preallocates things correctly
        parent->mapPool->push(data);
    }
    else {
        while(!mapPool->empty()) {
            delete mapPool->top();
            mapPool->pop();
        }
        //delete mapPool; // TODO: without this line we have a memory leak, find why it crashes
        delete data;
    }
    pthread_mutex_destroy(&memoryLock);
}

int BMemory::size() const {
    return data->size();
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
    Data* ret = (*data)[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        (*data)[item] = ret;
        attached_threads.erase(prevRet);
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
    auto it = data->find(item);
    if(it==data->end())
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
    Data* ret = (*data)[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        (*data)[item] = ret;
        attached_threads.erase(prevRet);
        delete prevRet;
        return ret;
    }

    if(ret) {
        // Handle mutability restrictions
        if (!allowMutable && !isFinal(item)) {
            bberror("Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item)
                    +"\n   \033[33m!!!\033[0m Either declare this in the current scope\n       or make its original declaration final.");
            return nullptr;
        }
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
    auto it = data->find(item);
    if(it==data->end())
        return nullptr;
    Data* ret = it->second;
    //std::cout << variableManager.getSymbol(item)<<"\n";
    if(ret && ret->getType()==FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        (*data)[item] = ret;
        attached_threads.erase(prevRet);
        delete prevRet;
        return ret;
    }
    return ret;
}

// Get a data item or return nullptr if not found
Data* BMemory::getOrNull(int item, bool allowMutable) {
    Data* ret = (*data)[item];

    // Handle future values
    if (ret && ret->getType() == FUTURE) {
        Future* prevRet = (Future*)ret;
        ret = prevRet->getResult();
        (*data)[item] = ret;
        attached_threads.erase(prevRet);
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
    (*data)[item] = nullptr;
}

// Set a data item, ensuring mutability rules are followed
void BMemory::set(int item, Data*value) {
    Data* prev = getOrNullShallow(item);
    if(prev==value) 
        return;
    if (prev) {
        if(!isFinal(item)) {
           if(prev->isDestroyable)
                delete prev;
        }
        else {
            bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
            return;
        }
    }
    //if(item==variableManager.noneId)
    //    delete value;
    //else
        (*data)[item] = value;
}

// Set a data item, ensuring mutability rules are followed
void BMemory::unsafeSet(int item, Data*value, Data* prev) {
    if(prev==value)
        return;
    if (prev) {
        if(!isFinal(item)) {
            if(prev->isDestroyable)
                delete prev;
        }
        else {
            bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
            return;
        }
    }
    (*data)[item] = value;
}

// Pull data from another Memory object
void BMemory::pull(const std::shared_ptr<BMemory>& other) {
    for (auto& it : *other->data) {
        set(it.first, it.second->shallowCopyIfNeeded());
    }
}

// Replace missing values with those from another Memory object
void BMemory::replaceMissing(const std::shared_ptr<BMemory>& other) {
    for (auto& it : *other->data) 
        if (data->find(it.first)==data->end()) {
            int item = it.first;
            set(item, other->get(item)->shallowCopyIfNeeded());
        }
}

// Detach this memory from its parent
void BMemory::detach() {
    if(!parent) {
        while(!mapPool->empty()) {
            delete mapPool->top();
            mapPool->pop();
        }
        delete mapPool;
        mapPool = new std::stack<tsl::hopscotch_map<int, Data*>*>();
    }
    allowMutables = false;
    parent = nullptr;
}

// Detach this memory from its parent but retain reference
void BMemory::detach(const std::shared_ptr<BMemory>& par) {
    if(!parent) {
        while(!mapPool->empty()) {
            delete mapPool->top();
            mapPool->pop();
        }
        delete mapPool;
    }
    if(par)
        mapPool = par->mapPool;
    else
        mapPool = new std::stack<tsl::hopscotch_map<int, Data*>*>();
    allowMutables = false;
    parent = par;
}
