#include "data/Struct.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include "interpreter/functional.h"
#include <iostream>
#include <stdexcept>
#include "common.h"

Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory) {
    BMemory* mem;
    DataPtr implementation;
    unsigned int depth = calledMemory->getDepth();
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        mem = getMemory();
        implementation = mem->getOrNullShallow(implementationCode);
    }

    bbassert(implementation.exists(), "Must define `" + variableManager.getSymbol(implementationCode) + "` for the struct to overload the corresponding operation");
    bbassert(implementation->getType() == CODE, "Struct field `"+variableManager.getSymbol(implementationCode) + "` is not a method and therefore the corresponding operation is not overloaded (even callable structs are not allowed)");

    BList* args = new BList(0);

    Code* code = static_cast<Code*>(implementation.get());
    BMemory newMemory(depth, calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(depth, code, &newMemory, true);
    Result value = executor.run(code);
    newMemory.setToNullIgnoringFinals(variableManager.thisId);
    return Result(value);
}

Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory, const DataPtr& other) {
    BMemory* mem;
    DataPtr implementation;
    unsigned int depth = calledMemory->getDepth();
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        mem = getMemory();
        implementation = mem->getOrNullShallow(implementationCode);
    }

    bbassert(implementation.exists(), "Must define `" + variableManager.getSymbol(implementationCode) + "` for the struct to overload the corresponding operation");
    bbassert(implementation->getType() == CODE, "Struct field `"+variableManager.getSymbol(implementationCode) + "` is not a method and therefore the corresponding operation is not overloaded (even callable structs are not allowed)");

    BList* args = new BList(1);  // will be destroyed alongside the memory
    other.existsAddOwner();
    args->contents.emplace_back(other);

    Code* code = static_cast<Code*>(implementation.get());
    BMemory newMemory(depth, calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(0, code, &newMemory, true);
    Result value = executor.run(code);
    newMemory.setToNullIgnoringFinals(variableManager.thisId);
    return Result(value);
}

Struct::Struct(BMemory* mem) : Data(STRUCT), memory(mem) {}
Struct::~Struct() {delete memory;}
BMemory* Struct::getMemory() const {return memory;}
void Struct::removeFromOwner() {
    int counter = --referenceCounter;
    if(counter<=1) delete this;
}


Result Struct::push(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structPush, scopeMemory, other); }
Result Struct::pop(BMemory* scopeMemory) { return simpleImplement(variableManager.structPop, scopeMemory); }
Result Struct::next(BMemory* scopeMemory) { return simpleImplement(variableManager.structNext, scopeMemory); }
Result Struct::at(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.structAt, scopeMemory, other); }

Result Struct::put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value) {
    bberror("Not implemented: neq(" + std::string(datatypeName[getType()]) + ", " + position.torepr() + ", " + value.torepr() + ")");
}

void Struct::clear(BMemory* scopeMemory) {
    DataPtr implementation;
    unsigned int depth;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getMemory()->getOrNullShallow(variableManager.structClear);
        depth = scopeMemory->getDepth();
    }
    if(implementation.islitorexists()) {
        Result res = simpleImplement(variableManager.structClear, scopeMemory);
    }
    else {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        BMemory* prevMemory = memory;
        memory = new BMemory(depth, nullptr, 1);
        delete prevMemory;
    }
}

int64_t Struct::len(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structLen, scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isint(), "Struct implemented `len` but did not return an int");
    return ret.unsafe_toint();
}

Result Struct::move(BMemory* scopeMemory) {
    DataPtr implementation;
    unsigned int depth;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getMemory()->getOrNullShallow(variableManager.structMove);
        depth = scopeMemory->getDepth();
    }
    if(implementation.islitorexists()) return simpleImplement(variableManager.structMove, scopeMemory);

    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    BMemory* mem = memory;
    memory = new BMemory(depth, nullptr, 1);
    memory->unsafeSet(variableManager.thisId, this);
    DataPtr ret = new Struct(mem);
    mem->unsafeSet(variableManager.thisId, ret);
    return RESMOVE(Result(ret));
}

Result Struct::iter(BMemory* scopeMemory) { return simpleImplement(variableManager.structIter, scopeMemory); }

double Struct::toFloat(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structFloat, scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isfloat(), "Struct implemented `float` and used it to overload a float cast, but the method did not actually return a float");
    return ret.unsafe_tofloat();
}

std::string Struct::toString(BMemory* calledMemory) {
    Result res = simpleImplement(variableManager.structStr, calledMemory);
    const auto& ret = res.get();
    bbassert(ret.existsAndTypeEquals(STRING), "Struct implemented `str` and used it to overload a string cast, but the method did not actually return a string");
    return ret.get()->toString(nullptr);
}

bool Struct::toBool(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structBool, scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isbool(), "Struct implemented `bool` and used it to overload a bool cast, but the method did not actually return a bool");
    return ret.unsafe_tobool();
}

int64_t Struct::toInt(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.structInt, scopeMemory);
    const auto& ret = res.get();
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
