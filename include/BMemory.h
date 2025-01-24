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
    const std::string getSymbol(int id) {
        return registeredIds[id];
    }
};


inline void* aligned_alloc(size_t newSize, size_t alignment) {
    void* newPtr = nullptr;
    if (posix_memalign(&newPtr, alignment, newSize*alignment)) bberror("Failed to allocate more memory");
    return newPtr;
}

inline void* aligned_realloc(void* oldPtr, size_t oldSize, size_t newSize, size_t alignment) {
    void* newPtr = nullptr;
    if (posix_memalign(&newPtr, alignment, newSize*alignment)) bberror("Failed to allocate more memory");
    std::memcpy(newPtr, oldPtr, oldSize);
    std::free(oldPtr);
    return newPtr;
}


    class BMemory {
    private:
        tsl::hopscotch_map<int, int> data;
        //std::vector<DataPtr> contents;
        void unsafeSet(int item, const DataPtr& value);
        std::vector<Code*> finally;
        inline int find(int item) const {
            // have there be a difference everywhere to save one instruction
            int tentativeidx = item-first_item;
            // prioritize the more likely comparison given that finds are mostly called after setting the first_item
            //if(tentativeidx<max_cache_size && tentativeidx>=0) return tentativeidx;  
            // if we have a cache size up to MAX_INT/2, the following works just fine thanks to overload making all negative values very large
            if(static_cast<unsigned int>(tentativeidx)>=max_cache_size) [[unlikely]] {
                const auto& idx = data.find(item);
                if(idx==data.end()) return end;
                return idx->second;
            }
            return tentativeidx;
        }
        static const int end = -1;
        int first_item;
        DataPtr* rawContents;
        int rawContentsSize;
        int rawContentsReserved;
        unsigned int max_cache_size;
        bool hasAtLeastOneFinal;
        const DataPtr& resolveFuture(int item, const DataPtr& ret);
        const DataPtr& get(int item, bool allowMutable);
    public:
        inline BMemory* getParentWithFinals() {
            // this is used to skip intermediate useless memory contexts in get() within function 
            if(hasAtLeastOneFinal) return this; 
            if(parent) return parent->getParentWithFinals();
            return nullptr;
        }
        bool isBlank() {return max_cache_size==0;}
        void release();
        tsl::hopscotch_map<Code*, Struct*> codeOwners;
        BMemory* parent;
        tsl::hopscotch_set<Future*> attached_threads;

        bool allowMutables;
        void prefetch() const;

        explicit BMemory(BMemory* par, int expectedAssignments, DataPtr thisObject=nullptr);
        ~BMemory();

        const DataPtr& get(int item) {
            // allowMutable = true
            int idx = find(item);
            if (idx == end) goto MISSING;
            {
                const auto& ret = rawContents[idx];
                if(ret.islit()) return ret; // this is a hot path for numbers
                if(!ret.exists()) goto MISSING;
                if(ret->getType()==FUTURE) [[unlikely]] return resolveFuture(item, ret);
                return ret;
            }

            MISSING:
                if (parent) return parent->get(item, allowMutables);
                bberror("Missing value: " + variableManager.getSymbol(item));
        }
        const DataPtr& getShallow(int item);
        const DataPtr& getOrNull(int item, bool allowMutable);
        const DataPtr& getOrNullShallow(int item);
        void directTransfer(int to, int from);
        void set(int item, const DataPtr& value);
        void setFuture(int item, const DataPtr& value);

        inline void unsafeSetLiteral(int item, const DataPtr& value) {
            // when this is called, we are guaranteed that value.exists() == false
            int idx = find(item);
            if(idx==end) [[unlikely]] {
                if(first_item==INT_MAX && item>=4) [[unlikely]] { // will occur once
                    first_item = item;
                    rawContents[0] = value;
                    //rawContents[0].setAFalse();
                    return;
                }
                data[item] = rawContentsSize;
                rawContents[rawContentsSize] = value;
                ++rawContentsSize;
                if(rawContentsSize>=rawContentsReserved) [[unlikely]] {
                    int newRawContentsReserved = rawContentsSize + rawContentsSize>>2; // + 25% growth (right shift is 1/4)
                    rawContents = (DataPtr*)aligned_realloc(rawContents, rawContentsReserved, newRawContentsReserved, sizeof(DataPtr));
                    rawContentsReserved = newRawContentsReserved;
                }
                return;
            }
            auto& prev = rawContents[idx];
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
