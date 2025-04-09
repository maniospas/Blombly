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

#include "data/Struct.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include "interpreter/functional.h"
#include <iostream>
#include <stdexcept>
#include "common.h"

extern std::vector<SymbolWorries> symbolUsage;
extern std::mutex ownershipMutex;
extern BError* OUT_OF_RANGE;
extern std::atomic<unsigned long long> countUnrealeasedMemories;

Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory) {
    BMemory* mem;
    DataPtr implementation;
    unsigned int depth = calledMemory->getDepth();
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getOrNull(implementationCode);
    }

    //if(implementation.existsAndTypeEquals(ERRORTYPE)) bberror(implementation->toString(nullptr));
    bbassert(implementation.exists(), "Must define `" + variableManager.getSymbol(implementationCode) + "` for the struct to overload the corresponding operation");
    bbassert(implementation->getType() == CODE, "Struct field `"+variableManager.getSymbol(implementationCode) + "` is not a method and therefore the corresponding operation is not overloaded (even callable structs are not allowed)");
    Code* code = static_cast<Code*>(implementation.get());

    /*bool forceAwait(false);
    {
        std::lock_guard<std::mutex> lock(ownershipMutex);
        for(int access : code->requestAccess) {
            auto& symbol = symbolUsage[access];
            if(symbol.modification) forceAwait = true;
            symbol.access++;
        }
        for(int access : code->requestModification) {
            auto& symbol = symbolUsage[access];
            if(symbol.access || symbol.modification) forceAwait = true;
            symbol.modification++;
        }
    }
    if(forceAwait) {calledMemory->tempawait();}
    CodeExiter codeExiter(code);*/
    BList* args = new BList(0);

    BMemory newMemory(depth, calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(depth, code, &newMemory, true);
    return executor.run(code).result;
}

Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory, const DataPtr& other) {
    BMemory* mem;
    DataPtr implementation;
    unsigned int depth = calledMemory->getDepth();
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getOrNull(implementationCode);
    }

    bbassert(implementation.exists(), "Must define `" + variableManager.getSymbol(implementationCode) + "` for the struct to overload the corresponding operation");
    bbassert(implementation->getType() == CODE, "Struct field `"+variableManager.getSymbol(implementationCode) + "` is not a method and therefore the corresponding operation is not overloaded (even callable structs are not allowed)");

    Code* code = static_cast<Code*>(implementation.get());
    /*bool forceAwait(false);
    {
        std::lock_guard<std::mutex> lock(ownershipMutex);
        for(int access : code->requestAccess) {
            auto& symbol = symbolUsage[access];
            if(symbol.modification) forceAwait = true;
            symbol.access++;
        }
        for(int access : code->requestModification) {
            auto& symbol = symbolUsage[access];
            if(symbol.access || symbol.modification) forceAwait = true;
            symbol.modification++;
        }
    }
    if(forceAwait) {calledMemory->tempawait();}
    CodeExiter codeExiter(code);*/

    BList* args = new BList(1);  // will be destroyed alongside the memory
    other.existsAddOwner();
    args->contents.emplace_back(other);

    BMemory newMemory(depth, calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(0, code, &newMemory, true);
    return executor.run(code).result;
}

Struct::Struct() : Data(STRUCT) {countUnrealeasedMemories++;}
Struct::Struct(int defaultSize) : Data(STRUCT) {data.reserve(defaultSize);countUnrealeasedMemories++;}
Struct::~Struct() {countUnrealeasedMemories--;releaseMemory();}

DataPtr Struct::get(int id) const {
    auto item = data.find(id);
    if (item == data.end()) bberror("Cannot find struct field: " + variableManager.getSymbol(id));
    return item->second;
}

DataPtr Struct::getOrNull(int id) const {
    auto item = data.find(id);
    if (item == data.end()) return DataPtr::NULLP;
    return item->second;
}

void Struct::set(int id, const DataPtr& value) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    value.existsAddOwner();
    if(data.find(id)!=data.end()){
        DataPtr prev = data[id];
        if(prev.isA()) bberror("Cannot overwrite final struct field: " + variableManager.getSymbol(id));
        //if(prev.existsAndTypeEquals(ERRORTYPE) && !static_cast<BError*>(prev.get())->isConsumed()) bberror("Trying to overwrite an unhandled error:\n"+prev->toString(this));
        prev.existsRemoveFromOwner();
    }
    data[id] = value;
    data[id].setAFalse();
}

void Struct::transferNoChecks(int id, const DataPtr& value) {
    data[id] = value;
}

void Struct::releaseMemory() {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string destroyerr;
    for(const auto& dat_ : data) {
        auto& dat = dat_.second;
        try {
           dat.existsRemoveFromOwner();
        }
        catch(const BBError& e) {destroyerr += std::string(e.what())+"\n";}
    }
    data.clear();
    if(destroyerr.size()) throw BBError(destroyerr.substr(0, destroyerr.size()-1));
}


Result Struct::push(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structPush, scopeMemory, other); }
Result Struct::pop(BMemory* scopeMemory) { 
    auto ret = simpleImplement(variableManager.structPop, scopeMemory); 
    if(!ret.get().islitorexists()) return Result(OUT_OF_RANGE);
    return ret;
}
Result Struct::next(BMemory* scopeMemory) { 
    auto ret = simpleImplement(variableManager.structNext, scopeMemory); 
    if(!ret.get().islitorexists()) return Result(OUT_OF_RANGE);
    return ret;
}
Result Struct::at(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structAt, scopeMemory, other); }

Result Struct::put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value) {
    bberror("Not implemented: put(" + std::string(datatypeName[getType()]) + ", " + position.torepr() + ", " + value.torepr() + ")");
}

void Struct::clear(BMemory* scopeMemory) {
    DataPtr implementation;
    unsigned int depth;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getOrNull(variableManager.structClear);
        depth = scopeMemory->getDepth();
    }
    if(implementation.islitorexists()) simpleImplement(variableManager.structClear, scopeMemory);
    else {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        releaseMemory();
    }
}

int64_t Struct::len(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structLen, scopeMemory);
    const auto& ret = res.get();
    if(ret.existsAndTypeEquals(ERRORTYPE)) throw BBError(ret->toString(nullptr));
    bbassert(ret.isint(), "Struct implemented `len` but did not return an int");
    return ret.unsafe_toint();
}

Result Struct::move(BMemory* scopeMemory) {
    DataPtr implementation;
    unsigned int depth;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getOrNull(variableManager.structMove);
        depth = scopeMemory->getDepth();
    }
    if(implementation.islitorexists()) return simpleImplement(variableManager.structMove, scopeMemory);

    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    Struct* ret = new Struct();
    ret->data = std::move(data);
    data = tsl::hopscotch_map<int, DataPtr>();
    return RESMOVE(Result(DataPtr(ret)));
}

Result Struct::iter(BMemory* scopeMemory) { return simpleImplement(variableManager.structIter, scopeMemory); }

double Struct::toFloat(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structFloat, scopeMemory);
    const auto& ret = res.get();
    if(ret.existsAndTypeEquals(ERRORTYPE)) throw BBError(ret->toString(nullptr));
    bbassert(ret.isfloat(), "Struct implemented `float` and used it to overload a float cast, but the method did not actually return a float");
    return ret.unsafe_tofloat();
}

std::string Struct::toString(BMemory* calledMemory) {
    Result res = simpleImplement(variableManager.structStr, calledMemory);
    const auto& ret = res.get();
    if(ret.existsAndTypeEquals(ERRORTYPE)) throw BBError(ret->toString(nullptr));
    bbassert(ret.existsAndTypeEquals(STRING), "Struct implemented `str` and used it to overload a string cast, but the method did not actually return a string");
    return ret.get()->toString(nullptr);
}

bool Struct::toBool(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structBool, scopeMemory);
    const auto& ret = res.get();
    if(ret.existsAndTypeEquals(ERRORTYPE)) throw BBError(ret->toString(nullptr));
    bbassert(ret.isbool(), "Struct implemented `bool` and used it to overload a bool cast, but the method did not actually return a bool");
    return ret.unsafe_tobool();
}

int64_t Struct::toInt(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structInt, scopeMemory);
    const auto& ret = res.get();
    if(ret.existsAndTypeEquals(ERRORTYPE)) throw BBError(ret->toString(nullptr));
    bbassert(ret.isint(), "Struct implemented `int` and used it to overload an int cast, but the method did not actually return an int");
    return ret.unsafe_toint();
}

Result Struct::add(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structAdd, scopeMemory, other); }
Result Struct::sub(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structSub, scopeMemory, other); }
Result Struct::rsub(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structRSub, scopeMemory, other); }
Result Struct::mul(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structMul, scopeMemory, other); }
Result Struct::div(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structDiv, scopeMemory, other); }
Result Struct::rdiv(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structRDiv, scopeMemory, other); }
Result Struct::pow(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structPow, scopeMemory, other); }
Result Struct::rpow(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structRPow, scopeMemory, other); }
Result Struct::mmul(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structMMul, scopeMemory, other); }
Result Struct::mod(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structMod, scopeMemory, other); }
Result Struct::rmod(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structRMod, scopeMemory, other); }
Result Struct::lt(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structLT, scopeMemory, other); }
Result Struct::le(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structLE, scopeMemory, other); }
Result Struct::gt(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structGT, scopeMemory, other); }
Result Struct::ge(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structGE, scopeMemory, other); }
Result Struct::eq(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structEq, scopeMemory, other); }
Result Struct::neq(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structNEq, scopeMemory, other); }
Result Struct::opand(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structAnd, scopeMemory, other); }
Result Struct::opor(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structOr, scopeMemory, other); }
Result Struct::min(BMemory* scopeMemory) { return simpleImplement(variableManager.structMin, scopeMemory); }
Result Struct::max(BMemory* scopeMemory) { return simpleImplement(variableManager.structMax, scopeMemory); }
Result Struct::logarithm(BMemory* scopeMemory) { return simpleImplement(variableManager.structLog, scopeMemory); }
Result Struct::sum(BMemory* scopeMemory) { return simpleImplement(variableManager.structSum, scopeMemory); }
Result Struct::opnot(BMemory* scopeMemory) { return simpleImplement(variableManager.structNot, scopeMemory); }
