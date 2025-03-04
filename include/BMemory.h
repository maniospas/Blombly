/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
    tsl::hopscotch_set<int> retainInStruct;
public:
    static constexpr int thisId = 0;
    static constexpr int argsId = 1;
    static constexpr int noneId = 2;
    static constexpr int callId = 3;
    static constexpr int structIter = 4;
    static constexpr int structPush = 5;
    static constexpr int structPop  = 6;
    static constexpr int structNext = 7;
    static constexpr int structAt   = 8;
    static constexpr int structLen  = 9;
    static constexpr int structClear= 10;
    static constexpr int structMove = 11;
    static constexpr int structStr  = 12;
    static constexpr int structBool = 13;
    static constexpr int structInt  = 14;
    static constexpr int structFloat= 15;
    static constexpr int structAdd  = 16;
    static constexpr int structSub  = 17;
    static constexpr int structRSub = 18;
    static constexpr int structMul  = 19;
    static constexpr int structDiv  = 20;
    static constexpr int structRDiv = 21;
    static constexpr int structPow  = 22;
    static constexpr int structRPow = 23;
    static constexpr int structMod  = 24;
    static constexpr int structRMod = 25;
    static constexpr int structMMul = 26;
    static constexpr int structLT   = 27;
    static constexpr int structLE   = 28;
    static constexpr int structGT   = 29;
    static constexpr int structGE   = 30;
    static constexpr int structEq   = 31;
    static constexpr int structNEq  = 32;
    static constexpr int structAnd  = 33;
    static constexpr int structOr   = 34;
    static constexpr int structMin  = 35;
    static constexpr int structMax  = 36;
    static constexpr int structSum  = 37;
    static constexpr int structNot  = 38;
    static constexpr int structLog  = 39;

    static constexpr int consoleId = 40;
    static constexpr int synchronizedListModification = 41;
    static constexpr int mainScopeNameId = 42;

    static constexpr int maximumReservedId = 42; // needs to be exactly equal to the last value


    VariableManager() {
        getId("this");
        getId("args");
        getId("#");
        getId("call");
        getId("iter");
        getId("push");
        getId("pop");
        getId("next");
        getId("at");
        getId("len");
        getId("clear");
        getId("move");
        getId("str");
        getId("bool");
        getId("int");
        getId("float");
        getId("add");
        getId("sub");
        getId("rsub");
        getId("mul");
        getId("div");
        getId("rdiv");
        getId("pow");
        getId("rpow");
        getId("mod");
        getId("rmod");
        getId("mmul");
        getId("lt");
        getId("le");
        getId("gt");
        getId("ge");
        getId("eq");
        getId("neq");
        getId("and");
        getId("or");
        getId("min");
        getId("max");
        getId("sum");
        getId("not");
        getId("log");
        getId("_bbconsole");
        getId("_bbanylist");
        getId("_bbmain");
    }
    const int size() {return registeredSymbols.size();}
    int getId(const std::string& symbol) {
        if(registeredSymbols.find(symbol) == registeredSymbols.end()) {
            int id = registeredSymbols.size();
            registeredSymbols[symbol] = id;
            registeredIds[id] = symbol;
            if(symbol.size()>8 && symbol.substr(0, 8)=="_bbmacro") retainInStruct.insert(id);
            else if(symbol.size()<3 || symbol.substr(0, 3)!="_bb") retainInStruct.insert(id);
        }
        return registeredSymbols[symbol];
    }
    const std::string getSymbol(int id) {return registeredIds[id];}
    const bool getIdRetain(int symbol) const {return retainInStruct.find(symbol) != retainInStruct.end();}
    void setIdRetain(int symbol) {retainInStruct.insert(symbol);}
};


class BMemory {
private:
    DataPtr* cache;
    unsigned int cache_size;
    tsl::hopscotch_map<int, DataPtr> data;
    //std::vector<DataPtr> contents;
    void unsafeSet(int item, const DataPtr& value);
    std::vector<Code*> finally;
    int first_item;
    bool hasAtLeastOneFinal;
    void resolveFuture(DataPtr& ret);
    const DataPtr& get(int item, bool allowMutable);
    DataPtr& find(int item) {
        if(item<=variableManager.maximumReservedId) return data[item];
        if(first_item==INT_MAX) [[unlikely]] first_item = item;
        // have there be a difference everywhere to save one instruction
        int tentativeidx = item-first_item;
        // prioritize the more likely comparison given that finds are mostly called after setting the first_item
        //if(tentativeidx<max_cache_size && tentativeidx>=0) return tentativeidx;  
        // if we have a cache size up to MAX_INT/2, the following works just fine thanks to overload making all negative values very large
        if(static_cast<unsigned int>(tentativeidx)>=cache_size) return data[item];
        return cache[tentativeidx];
    }
    unsigned int depth;
public:
    BMemory* parent;
    tsl::hopscotch_map<Code*, Struct*> codeOwners;
    std::vector<Result> attached_threads;
    inline BMemory* getParentWithFinals() {
        // this is used to skip intermediate useless memory contexts in get() within function 
        if(hasAtLeastOneFinal) return this; 
        if(parent) return parent->getParentWithFinals();
        return nullptr;
    }
    unsigned int getDepth() const {return depth;}
    void release();
    bool allowMutables;

    explicit BMemory(unsigned int depth, BMemory* par, int expectedAssignments);
    ~BMemory();

    const DataPtr& get(int item) {
        DataPtr& ret = find(item);
        if(ret.islit()) return ret; // this is a hot path for numbers
        if(!ret.exists()) {
            if (parent) return parent->get(item, allowMutables);
            bberror("Missing value: " + variableManager.getSymbol(item));
        }
        if(ret->getType()==FUTURE) [[unlikely]] {
            resolveFuture(ret);
            return get(item);
        }
        return ret;
    }
    const DataPtr& getShallow(int item);
    const DataPtr& getOrNull(int item, bool allowMutable);
    const DataPtr& getOrNullShallow(int item);
    void directTransfer(Struct* to);
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
    void tempawait();
    void detach(BMemory* par);
    void runFinally();
    void addFinally(Code* code);

    static void verify_noleaks();
    friend class Struct;
};

#endif // MEMORY_H
