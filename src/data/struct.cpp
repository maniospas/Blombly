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
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        mem = getMemory();
        implementation = mem->getOrNullShallow(implementationCode);
    }

    bbassert(implementation.exists(), "Must define `" + variableManager.getSymbol(implementationCode) + "` for the struct to overload the corresponding operation");
    bbassert(implementation->getType() == CODE, "Struct field `"+variableManager.getSymbol(implementationCode) + "` is not a method and therefore the corresponding operation is not overloaded (even callable structs are not allowed)");

    BList* args = new BList(0);

    Code* code = static_cast<Code*>(implementation.get());
    BMemory newMemory(calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(code, &newMemory, true);
    Result value = executor.run(code);
    newMemory.setToNullIgnoringFinals(variableManager.thisId);
    return Result(value);
}

Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory, const DataPtr& other) {
    BMemory* mem;
    DataPtr implementation;
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
    BMemory newMemory(calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(code, &newMemory, true);
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


Result Struct::push(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("push"), scopeMemory, other); }
Result Struct::pop(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("pop"), scopeMemory); }
Result Struct::next(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("next"), scopeMemory); }
Result Struct::at(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("at"), scopeMemory, other); }

Result Struct::put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value) {
    bberror("Not implemented: neq(" + std::string(datatypeName[getType()]) + ", " + position.torepr() + ", " + value.torepr() + ")");
}

void Struct::clear(BMemory* scopeMemory) {
    DataPtr implementation;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getMemory()->getOrNullShallow(variableManager.getId("clear"));
    }
    if(implementation.islitorexists()) {
        Result res = simpleImplement(variableManager.getId("clear"), scopeMemory);
        const auto& ret = res.get();
        if(ret.islitorexists()) {
            std::lock_guard<std::recursive_mutex> lock(memoryLock);
            BMemory* prevMemory = memory;
            memory = new BMemory(nullptr, 1);
            delete prevMemory;
            bberror("Struct returned from its `clear` method.");
        }
    }
    else {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        BMemory* prevMemory = memory;
        memory = new BMemory(nullptr, 1);
        delete prevMemory;
    }
}

int64_t Struct::len(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.getId("len"), scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isint(), "Struct implemented `len` but did not return an int");
    return ret.unsafe_toint();
}

Result Struct::move(BMemory* scopeMemory) {
    DataPtr implementation;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        implementation = getMemory()->getOrNullShallow(variableManager.getId("clear"));
    }
    if(implementation.islitorexists()) return simpleImplement(variableManager.getId("clear"), scopeMemory);

    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    BMemory* mem = memory;
    memory = new BMemory(nullptr, 1);
    memory->unsafeSet(variableManager.thisId, this);
    DataPtr ret = new Struct(mem);
    mem->unsafeSet(variableManager.thisId, ret);
    return RESMOVE(Result(ret));
}

Result Struct::iter(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("iter"), scopeMemory); }

double Struct::toFloat(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.getId("float"), scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isfloat(), "Struct implemented `float` and used it to overload a float cast, but the method did not actually return a float");
    return ret.unsafe_tofloat();
}

std::string Struct::toString(BMemory* calledMemory) {
    Result res = simpleImplement(variableManager.getId("str"), calledMemory);
    const auto& ret = res.get();
    bbassert(ret.existsAndTypeEquals(STRING), "Struct implemented `str` and used it to overload a string cast, but the method did not actually return a string");
    return ret.get()->toString(nullptr);
}

bool Struct::toBool(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.getId("bool"), scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isbool(), "Struct implemented `bool` and used it to overload a bool cast, but the method did not actually return a bool");
    return ret.unsafe_tobool();
}

int64_t Struct::toInt(BMemory* scopeMemory) {
    Result res = simpleImplement(variableManager.getId("int"), scopeMemory);
    const auto& ret = res.get();
    bbassert(ret.isint(), "Struct implemented `int` and used it to overload an int cast, but the method did not actually return an int");
    return ret.unsafe_toint();
}

Result Struct::add(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("add"), scopeMemory, other); }
Result Struct::sub(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("sub"), scopeMemory, other); }
Result Struct::mul(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("mul"), scopeMemory, other); }
Result Struct::div(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("div"), scopeMemory, other); }
Result Struct::pow(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("pow"), scopeMemory, other); }
Result Struct::mmul(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("mmul"), scopeMemory, other); }
Result Struct::mod(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("mod"), scopeMemory, other); }
Result Struct::lt(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("lt"), scopeMemory, other); }
Result Struct::le(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("le"), scopeMemory, other); }
Result Struct::gt(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("gt"), scopeMemory, other); }
Result Struct::ge(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("ge"), scopeMemory, other); }
Result Struct::eq(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("eq"), scopeMemory, other); }
Result Struct::neq(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("neq"), scopeMemory, other); }
Result Struct::opand(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("and"), scopeMemory, other); }
Result Struct::opor(BMemory* scopeMemory, const DataPtr& other) { return simpleImplement(variableManager.getId("or"), scopeMemory, other); }
Result Struct::min(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("min"), scopeMemory); }
Result Struct::max(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("max"), scopeMemory); }
Result Struct::logarithm(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("log"), scopeMemory); }
Result Struct::sum(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("sum"), scopeMemory); }
Result Struct::opnot(BMemory* scopeMemory) { return simpleImplement(variableManager.getId("not"), scopeMemory); }
