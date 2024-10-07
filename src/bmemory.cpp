#include "BMemory.h"
#include "common.h"
#include "memory.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include <unordered_set>  

tsl::hopscotch_set<std::shared_ptr<BMemory>> memories;
std::atomic<int> countUnrealeasedMemories;
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts");
    bbassert(countUnrealeasedMemories.load() == 1, "There are " + std::to_string(countUnrealeasedMemories.load()-1) + " leftover memory contexts");  // the main memory is a global object (needed to sync threads on errors)
}

BMemory::BMemory(const std::shared_ptr<BMemory>& par, int expectedAssignments)
    : parent(par), allowMutables(true), fastLastAccessId(-1) {
    countUnrealeasedMemories++;
    data.reserve(expectedAssignments);
}

bool BMemory::isOrDerivedFrom(const BMemory* memory) const {
    if (memory == this)
        return true;  // stop on cycle
    if (parent)
        return parent->isOrDerivedFrom(memory);
    return false;
}

void BMemory::release() {
    std::string destroyerr = "";
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == FUTURE) {
            auto prevRet = std::dynamic_pointer_cast<Future>(dat);
            try {
                data[element.first] = prevRet->getResult();
            }
            catch (const BBError& e) {
                destroyerr += std::string(e.what())+"\n";
            }
            data[element.first] = nullptr;
            attached_threads.erase(prevRet);
        }
    }
    for (const auto& thread : attached_threads) {
        try {
            thread->getResult();
        }
        catch (const BBError& e) {
            destroyerr += std::string(e.what())+"\n";
        }
        attached_threads.erase(thread);
    }
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == ERRORTYPE && !std::dynamic_pointer_cast<BError>(dat)->isConsumed()) {
            destroyerr += "\033[0m(\x1B[31m ERROR \033[0m) The following error was caught but never handled:"+dat->toString()+"\n";
        }
    }
    if(destroyerr.size())
        throw BBError(destroyerr.substr(0, destroyerr.size()-1));
        //bberror("The following error(s) were found while releasing a struct or memory:"+destroyerr);
}

BMemory::~BMemory() {
    release();
    data.clear();
    countUnrealeasedMemories--;
}

int BMemory::size() const {
    return data.size();
}

std::shared_ptr<Data> BMemory::get(int item) {
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
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

std::shared_ptr<Data> BMemory::get(int item, bool allowMutable) {
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
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


bool BMemory::contains(int item) {
    if(item==fastLastAccessId) 
        return true;
    auto it = data.find(item);
    if (it == data.end())
        return false;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    return ret!=nullptr;
}

std::shared_ptr<Data> BMemory::getShallow(int item) {
    if(item==fastLastAccessId) 
        return fastLastAccess.lock();
    auto it = data.find(item);
    if (it == data.end()) 
        bberror("Missing value: " + variableManager.getSymbol(item));
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    if(!ret)
        bberror("Missing value: " + variableManager.getSymbol(item));
    return ret;
}

std::shared_ptr<Data> BMemory::getOrNullShallow(int item) {
    if(item==fastLastAccessId) 
        return fastLastAccess.lock();
    auto it = data.find(item);
    if (it == data.end())
        return nullptr;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    return ret;
}

std::shared_ptr<Data> BMemory::getOrNull(int item, bool allowMutable) {
    if(item==fastLastAccessId) 
        return fastLastAccess.lock();
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        ret = unsafeSet(item, ret);
        attached_threads.erase(prevRet);
    }
    if (ret && !allowMutable && !isFinal(item)) {
        bberror("Mutable symbol cannot be accessed from a nested block: " + variableManager.getSymbol(item));
        return nullptr;
    }
    if (!ret && parent) 
        ret = parent->getOrNull(item, allowMutables && allowMutable);
    
    return ret;
}

void BMemory::removeWithoutDelete(int item) {
    data[item] = nullptr;
    //fastLastAccessId = -1;
}

void BMemory::unsafeSet(const std::shared_ptr<BMemory>& handler, int item, const std::shared_ptr<Data>& value, const std::shared_ptr<Data>& prev) {
    if (prev == value)
        return;
    if (prev && isFinal(item))
        bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    
    std::shared_ptr<Data> val;
    if(value && value->getType()==STRUCT) // here we may convert strong pointer structs to weak pointer structs so that deleting the memory will also delete those pointers even if there are cycles
        val = std::static_pointer_cast<Struct>(value)->modifyBeforeAttachingToMemory(handler, std::static_pointer_cast<Struct>(value), this);
    else 
        val = value;
    //fastLastAccess = val;
    //fastLastAccessId = item;
    data[item] = std::move(val);
}

void BMemory::unsafeSet(int item, const std::shared_ptr<Data>& value, const std::shared_ptr<Data>& prev) {
    if (prev == value)
        return;
    if (prev && isFinal(item))
        bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    
    std::shared_ptr<Data> val;
    if(value && value->getType()==STRUCT) // here we may convert strong pointer structs to weak pointer structs so that deleting the memory will also delete those pointers even if there are cycles
        val = std::static_pointer_cast<Struct>(value)->modifyBeforeAttachingToMemory(nullptr, std::static_pointer_cast<Struct>(value), this);
    else 
        val = value;
    //fastLastAccess = val;
    //fastLastAccessId = item;
    data[item] = std::move(val);
}

std::shared_ptr<Data> BMemory::unsafeSet(int item, const std::shared_ptr<Data>& value) {
    std::shared_ptr<Data> val;
    if(value && value->getType()==STRUCT) // here we may convert strong pointer structs to weak pointer structs so that deleting the memory will also delete those pointers even if there are cycles
        val = std::static_pointer_cast<Struct>(value)->modifyBeforeAttachingToMemory(nullptr, std::static_pointer_cast<Struct>(value), this);
    else
        val = value;
    data[item] = std::move(val);  // TODO: this is a prime suspect for a bug
    //if(item==fastLastAccessId)
    //    fastLastAccessId = -1;
    return data[item];
}

void BMemory::setFinal(int item) {
    finals.insert(item);
}

bool BMemory::isFinal(int item) const {
    return finals.find(item) != finals.end();
}

void BMemory::pull(const std::shared_ptr<BMemory>& other) {
    for (const auto& it : other->data) {
        if (it.second) {
            unsafeSet(it.first, INLINE_SCOPY(it.second), getOrNullShallow(it.first));
        }
    }
}

void BMemory::replaceMissing(const std::shared_ptr<BMemory>& other) {
    for (const auto& it : other->data) {
        int item = it.first;
        auto existing = getOrNullShallow(item);
        if (!existing) {
            auto dat = it.second;
            if (dat) 
                unsafeSet(item, INLINE_SCOPY(dat), nullptr);
        }
    }
}

void BMemory::detach() {
    for (const auto& element : data) {
        if (element.second && element.second->getType() == FUTURE) {
            auto prevRet = std::dynamic_pointer_cast<Future>(element.second);
            unsafeSet(element.first, prevRet->getResult());
            attached_threads.erase(prevRet);
        }
    }
    for (const auto& thread : attached_threads) 
        thread->getResult();
    attached_threads.clear();
    allowMutables = false;
    parent = nullptr;
}

void BMemory::detach(const std::shared_ptr<BMemory>& par) {
    detach();
    parent = par;
}
