#ifndef BMEMORY_H
#define BMEMORY_H

#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <vector>
#include <iostream>
#include <atomic>
#include "data/Data.h"
#include "tsl/hopscotch_map.h"
#include "tsl/hopscotch_set.h"
class Future;
class Struct;
class Command;

#include "memory.h"
#include "data/BError.h"
#include <unordered_set>  
#include <bit>
#include <xmmintrin.h>
#include <new>     // For std::align


class VariableManager {
private:
    tsl::hopscotch_map<std::string, int> registeredSymbols;
    tsl::hopscotch_map<int, std::string> registeredIds;
public:
    static constexpr int thisId = 0;
    static constexpr int argsId = 1;
    static constexpr int noneId = 2;
    static constexpr int callId = 3;

    VariableManager() {
        getId("this");
        getId("args");
        getId("#");
        getId("call");
    }
    int getId(const std::string& symbol) {
        if(registeredSymbols.find(symbol) == registeredSymbols.end()) {
            int id = registeredSymbols.size();
            registeredSymbols[symbol] = id;
            registeredIds[id] = symbol;
        }
        return registeredSymbols[symbol];
    }
    const std::string getSymbol(int id) {return registeredIds[id];}
};

#define CACHE_SIZE 32

class BMemory {
private:
    DataPtr cache[CACHE_SIZE];
    tsl::hopscotch_map<int, DataPtr> data;
    //std::vector<DataPtr> contents;
    void unsafeSet(int item, const DataPtr& value);
    std::vector<Code*> finally;
    int first_item;
    bool hasAtLeastOneFinal;
    const DataPtr& resolveFuture(int item, const DataPtr& ret);
    const DataPtr& get(int item, bool allowMutable);
    DataPtr& find(int item) {
        if(item<4) return data[item];
        if(first_item==INT_MAX) [[unlikely]] first_item = item;
        // have there be a difference everywhere to save one instruction
        int tentativeidx = item-first_item;
        // prioritize the more likely comparison given that finds are mostly called after setting the first_item
        //if(tentativeidx<max_cache_size && tentativeidx>=0) return tentativeidx;  
        // if we have a cache size up to MAX_INT/2, the following works just fine thanks to overload making all negative values very large
        if(static_cast<unsigned int>(tentativeidx)>=CACHE_SIZE) return data[item];
        return cache[tentativeidx];
    }
public:
    inline BMemory* getParentWithFinals() {
        // this is used to skip intermediate useless memory contexts in get() within function 
        if(hasAtLeastOneFinal) return this; 
        if(parent) return parent->getParentWithFinals();
        return nullptr;
    }
    void release();
    tsl::hopscotch_map<Code*, Struct*> codeOwners;
    BMemory* parent;
    tsl::hopscotch_set<Future*> attached_threads;

    bool allowMutables;
    void prefetch() const;

    explicit BMemory(BMemory* par, int expectedAssignments, DataPtr thisObject=DataPtr::NULLP);
    ~BMemory();

    const DataPtr& get(int item) {
        DataPtr& ret = find(item);
        if(ret.islit()) return ret; // this is a hot path for numbers
        if(!ret.exists()) {
            if (parent) return parent->get(item, allowMutables);
            bberror("Missing value: " + variableManager.getSymbol(item));
        }
        if(ret->getType()==FUTURE) [[unlikely]] return resolveFuture(item, ret);
        return ret;
    }
    const DataPtr& getShallow(int item);
    const DataPtr& getOrNull(int item, bool allowMutable);
    const DataPtr& getOrNullShallow(int item);
    void directTransfer(int to, int from);
    void set(int item, const DataPtr& value);
    void setFuture(int item, const DataPtr& value);

    void unsafeSetLiteral(int item, const DataPtr& value) {
        auto& prev = find(item);
        if(prev.isA()) bberror("Cannot overwrite final value: " + variableManager.getSymbol(item));
        if(prev.exists()) {
            Data* prevData = prev.get();
            if(prevData->getType()==ERRORTYPE && !static_cast<BError*>(prevData)->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prevData->toString(this));
            prevData->removeFromOwner();
        }
        prev = value;
        //prev.setAFalse(); // this is not needed because this function is called only for newlly constructed literals. that said, it somehow speeds up the code
    }

    void setToNullIgnoringFinals(int item);
    int size() const;
    void setFinal(int item);

    void pull(BMemory* other);
    void replaceMissing(BMemory* other);
    void await();
    void detach(BMemory* par);
    void runFinally();
    void addFinally(Code* code);

    static void verify_noleaks();
    friend class Struct;
};

#endif // MEMORY_H
