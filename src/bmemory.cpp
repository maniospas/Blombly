#include "BMemory.h"
#include "common.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Code.h"
#include <unordered_set>  

tsl::hopscotch_set<std::shared_ptr<BMemory>> memories;
std::atomic<int> countUnrealeasedMemories;
extern VariableManager variableManager;

void BMemory::verify_noleaks() {
    bbassert(memories.size() == 0, "There are " + std::to_string(memories.size()) + " leftover memory contexts");
    bbassert(countUnrealeasedMemories.load() == 0, "There are " + std::to_string(countUnrealeasedMemories.load()) + " leftover memory contexts");
}

BMemory::BMemory(const std::shared_ptr<BMemory>& par, int expectedAssignments)
    : parent(par), allowMutables(true), fastLastAccessId(-1) {
    countUnrealeasedMemories++;
    data.reserve(expectedAssignments);
}

bool BMemory::isOrDerivedFrom(const std::shared_ptr<BMemory>& memory) const {
    if (memory.get() == this)
        return true;  // stop on cycle
    if (parent)
        return parent->isOrDerivedFrom(memory);
    return false;
}

void BMemory::release() {
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == FUTURE) {
            auto prevRet = std::dynamic_pointer_cast<Future>(dat);
            try {
                data[element.first] = prevRet->getResult();
            }
            catch (const BBError& e) {
                std::string destroyerr = "Error occurred during the execution of spawned code.";
                destroyerr += "\n ( \x1B[31m ERROR \033[0m ) " + std::string(e.what());
                bberror(destroyerr);
            }
            data[element.first] = nullptr;
            attached_threads.erase(prevRet);
        }
    }
    for (const auto& thread : attached_threads) {
        thread->getResult();
    }
    for (const auto& element : data) {
        auto dat = element.second;
        if (dat && dat->getType() == ERRORTYPE && !std::dynamic_pointer_cast<BError>(dat)->isConsumed()) {
            bberror("Intercepted error not handled: " + dat->toString());
        }
    }
}

BMemory::~BMemory() {
    release();
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
        data[item] = ret;
        attached_threads.erase(prevRet);
        return ret;
    }
    if (!ret && parent) {
        ret = parent->get(item, allowMutables);
    }
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
        data[item] = ret;
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
    auto it = data.find(item);
    if (it == data.end())
        return false;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
    }
    return ret!=nullptr;
}

std::shared_ptr<Data> BMemory::getOrNullShallow(int item) {
    auto it = data.find(item);
    if (it == data.end())
        return nullptr;
    auto ret = it->second;
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        data[item] = ret;
        attached_threads.erase(prevRet);
    }
    return ret;
}

std::shared_ptr<Data> BMemory::getOrNull(int item, bool allowMutable) {
    auto ret = data[item];
    if (ret && ret->getType() == FUTURE) {
        auto prevRet = std::dynamic_pointer_cast<Future>(ret);
        ret = prevRet->getResult();
        data[item] = ret;
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
}

void BMemory::unsafeSet(int item, const std::shared_ptr<Data>& value, const std::shared_ptr<Data>& prev) {
    if (prev == value)
        return;
    if (prev && isFinal(item))
        bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
    data[item] = value;
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
            unsafeSet(it.first, it.second->shallowCopy(), getOrNullShallow(it.first));
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
                unsafeSet(item, dat->shallowCopy(), nullptr);
        }
    }
}

void BMemory::detach() {
    for (const auto& element : data) {
        if (element.second && element.second->getType() == FUTURE) {
            auto prevRet = std::dynamic_pointer_cast<Future>(element.second);
            data[element.first] = prevRet->getResult();
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
