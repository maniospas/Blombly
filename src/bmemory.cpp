#include "BMemory.h"
#include "common.h"
#include "memory.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include <unordered_set>  

tsl::hopscotch_set<BMemory*> memories;
std::atomic<unsigned long long> countUnrealeasedMemories(0);
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts");
    bbassert(countUnrealeasedMemories.load() == 0, "There are " + std::to_string(countUnrealeasedMemories.load()) + " leftover memory contexts");  // the main memory is a global object (needed to sync threads on errors)
}

BMemory::BMemory(BMemory* par, int expectedAssignments, Data* thisObject) : parent(par), allowMutables(true), fastId(-1), thisObject(thisObject) {
    //std::cout << "created "<<this<<"\n";
    ++countUnrealeasedMemories;
    data.reserve(expectedAssignments);
}

bool BMemory::isOrDerivedFrom(BMemory* memory) const {
    if (memory == this)
        return true;  // stop on cycle
    if (parent)
        return parent->isOrDerivedFrom(memory);
    return false;
}

void BMemory::leak() { // only do this after detach so that there are no leftover FUTURE values
    for (const auto& element : data) {
        Data* ret = element.second;
        /*if(ret && ret->getType()==FUTURE) {
            auto prevRet = static_cast<Future*>(ret);
            auto resVal = prevRet->getResult();
            ret = resVal.get();
            unsafeSet(element.first, ret); 
            attached_threads.erase(prevRet);
            if(ret)
                ret->leak();
        }
        else */if(ret)
            ret->leak();
    }
}

void BMemory::release() {
    //std::cout << "releasing "<<this<<"\n";
    std::string destroyerr = "";
    for (const auto& thread : attached_threads) {
        try {
            thread->getResult();
        }
        catch (const BBError& e) {
            destroyerr += std::string(e.what())+"\n";
        }
    }
    attached_threads.clear();
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == ERRORTYPE && !static_cast<BError*>(dat)->isConsumed())  
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was caught but never handled:\n"+dat->toString()+"\n";
        try {
            if(dat) 
                dat->removeFromOwner();
        }
        catch (const BBError& e) {
            destroyerr += std::string(e.what())+"\n";
        }
    }

    if(destroyerr.size()) 
        throw BBError(destroyerr.substr(0, destroyerr.size()-1));
        //bberror("The following error(s) were found while releasing a struct or memory:"+destroyerr);
}



BMemory::~BMemory() {
    // first remove any this object the memory might have, as this is what is actually calling the destruction (unless there is no this object)
    data[variableManager.thisId] = nullptr;
    release();
    //data.clear();
    countUnrealeasedMemories--;
}

int BMemory::size() const {
    return data.size();
}

Data* BMemory::get(int item) { // allowMutable = true
    if(item==fastId)
        return fastData;
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret); 
        attached_threads.erase(prevRet);
        return ret;
    }
    if (!ret && parent) 
        ret = parent->get(item, allowMutables);
    if (!ret) {
        bberror("Missing value: " + variableManager.getSymbol(item));
    }
    return ret;
}

Data* BMemory::get(int item, bool allowMutable) {
    if(item==fastId)
        return fastData;
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret); 
        attached_threads.erase(prevRet);
        bbassert(ret, "Missing value: " + variableManager.getSymbol(item));
        return ret;
    }
    if (ret) {
        bbassert(allowMutable || isFinal(item), 
            "Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));
    } 
    else if (parent) 
        ret = parent->get(item, allowMutables && allowMutable);
    
    bbassert(ret, "Missing value: " + variableManager.getSymbol(item));
    return ret;
}


bool BMemory::containsAnywhere(BMemory* other) {
    if(other==this)
        return true;
    detach(parent); // synchronize all threads
    for(const auto& it : data)
        if(it.second && it.second->getType()==STRUCT && it.first!=variableManager.thisId && static_cast<Struct*>(it.second)->getMemory()->containsAnywhere(other)) 
            return true;
    return false;
}


bool BMemory::contains(int item) {
    if(item==fastId)
        return fastData;
    auto it = data.find(item);
    if (it == data.end())
        return false;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret); 
        attached_threads.erase(prevRet);
    }
    return ret!=nullptr;
}

Data* BMemory::getShallow(int item) {
    if(item==fastId)
        return fastData;
    auto it = data.find(item);
    if (it == data.end()) 
        bberror("Missing value: " + variableManager.getSymbol(item));
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        //std::cout << "here4\n";
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret);
        attached_threads.erase(prevRet);
        //std::cout << "done\n";
    }
    if(!ret)
        bberror("Missing value: " + variableManager.getSymbol(item));
    return ret;
}

Data* BMemory::getOrNullShallow(int item) {
    if(item==fastId)
        return fastData;
    auto it = data.find(item);
    if (it == data.end())
        return nullptr;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    return ret;
}

Data* BMemory::getOrNull(int item, bool allowMutable) {
    if(item==fastId)
        return fastData;
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = static_cast<Future*>(ret);
        auto resVal = prevRet->getResult();
        ret = resVal.get();
        unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    if (ret && !allowMutable && !isFinal(item)) 
        bberror("Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));
    if (!ret && parent) 
        ret = parent->getOrNull(item, allowMutables && allowMutable);
    
    return ret;
}

void BMemory::removeWithoutDelete(int item) {
    data[item] = nullptr;
    if(fastId==item)
        fastData = nullptr;
}

void BMemory::unsafeSet(BMemory* handler, int item, Data* value, Data* prev) {
    if (isFinal(item))
        bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(value && value->getType()==FUTURE) 
        fastId = -1;
    else {
        fastId = item;
        fastData = value;
    }
    if(value)
        value->addOwner();
    if(prev)
        prev->removeFromOwner();
    data[item] = value;
}

void BMemory::unsafeSet(int item, Data* value, Data* prev) {
    if (isFinal(item))
        bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    if(value) {
        if(value->getType()!=FUTURE) {
            fastId = item;
            fastData = value;
        }
    }
    else
        fastId = -1;
    prev = data[item];
    if(value)
        value->addOwner();
    if(prev)
        prev->removeFromOwner();
    data[item] = value;
    //std::cout << "set "<<variableManager.getSymbol(item)<<" to "<<value<<"\n";
}

void BMemory::unsafeSet(int item, Data* value) {
    Data* prev = data[item];
    if(value) 
        value->addOwner();
    if(prev)
        prev->removeFromOwner();
    data[item] = value;
    //std::cout << "set "<<variableManager.getSymbol(item)<<" to "<<value<<"\n";
}

void BMemory::setFinal(int item) {
    finals.insert(item);
}

bool BMemory::isFinal(int item) const {
    return finals.find(item) != finals.end();
}

void BMemory::pull(BMemory* other) {
    for (const auto& it : other->data) {
        if (it.second && it.first!=variableManager.thisId) {
            it.second->leak();
            unsafeSet(it.first, it.second, getOrNullShallow(it.first));
        }
    }
}

void BMemory::replaceMissing(BMemory* other) {
    for (const auto& it : other->data) {
        int item = it.first;
        auto existing = getOrNullShallow(item);
        if (!existing) {
            auto dat = it.second;
            if (dat) 
                unsafeSet(item, dat, nullptr);
        }
    }
}

void BMemory::await() {
    std::string destroyerr = "";

    for (const auto& thread : attached_threads) {
        try {
            thread->getResult();
           // if(ret)
           //     ret->removeFromOwner();
        }
        catch (const BBError& e) {
            destroyerr += std::string(e.what())+"\n";
        }
    }
    
    attached_threads.clear();
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == ERRORTYPE && !static_cast<BError*>(dat)->isConsumed())  {
            static_cast<BError*>(dat)->consume();
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was intercepted with `try` but never handled:\n"+dat->toString()+"\n";
        }
    }
    
    if(destroyerr.size())
        throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}

void BMemory::detach(BMemory* par) {
    await();
    if(!par)
        allowMutables = false;
    parent = par;
}
