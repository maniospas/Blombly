#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    BMemory* memory;
    Result simpleImplement(int implementationCode, BMemory* scopeMemory);
    Result simpleImplement(int implementationCode, BMemory* scopeMemory, const DataPtr& other);

public:
    mutable std::recursive_mutex memoryLock; 
    explicit Struct(BMemory* mem);
    ~Struct();
    std::string toString(BMemory* scopeMemory) override;
    BMemory* getMemory() const;    
    void removeFromOwner() override;
    Result implement(const OperationType operation_, BuiltinArgs* args_, BMemory* scopeMemory) override;

    Result push(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("push"), scopeMemory, other);}
    Result pop(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("pop"), scopeMemory);}
    Result next(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("next"), scopeMemory);}
    Result at(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("at"), scopeMemory, other);}
    Result put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value) {bberror("Not implemented: neq("+std::string(datatypeName[getType()])+", "+position.torepr()+", "+value.torepr()+")");}

    void clear(BMemory* scopeMemory) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        BMemory* prevMemory = memory;
        memory = new BMemory(nullptr, 1);
        delete prevMemory;
    }

    int64_t len(BMemory* scopeMemory) {
        Result res = simpleImplement(variableManager.getId("len"), scopeMemory);
        const auto& ret = res.get();
        bbassert(ret.isint(), "Struct implemented `len` but did not return an int");
        return ret.unsafe_toint();
    }

    Result move(BMemory* scopeMemory) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        BMemory* mem = memory;
        memory = new BMemory(nullptr, 1);
        memory->unsafeSet(variableManager.thisId, this);
        DataPtr ret = new Struct(mem);
        mem->unsafeSet(variableManager.thisId, ret);
        return std::move(Result(ret));
    }

    Result iter(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("iter"), scopeMemory);}
    double toFloat(BMemory* scopeMemory) {
        Result res = simpleImplement(variableManager.getId("float"), scopeMemory);
        const auto& ret = res.get();
        bbassert(ret.isfloat(), "Struct implemented `float` and used it to overload a float cast, but the method did not actually return a float");
        return ret.unsafe_tofloat();
    }
    bool toBool(BMemory* scopeMemory)  {
        Result res = simpleImplement(variableManager.getId("bool"), scopeMemory);
        const auto& ret = res.get();
        bbassert(ret.isbool(), "Struct implemented `bool` and used it to overload a bool cast, but the method did not actually return a bool");
        return ret.unsafe_tobool();
    }
    int64_t toInt(BMemory* scopeMemory) {
        Result res = simpleImplement(variableManager.getId("int"), scopeMemory);
        const auto& ret = res.get();
        bbassert(ret.isint(), "Struct implemented `int` and used it to overload an int cast, but the method did not actually return an int");
        return ret.unsafe_toint();
    }
    Result add(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("add"), scopeMemory, other);}
    Result sub(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("sub"), scopeMemory, other);}
    Result mul(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("mul"), scopeMemory, other);}
    Result div(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("div"), scopeMemory, other);}
    Result pow(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("pow"), scopeMemory, other);}
    Result mmul(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("mmul"), scopeMemory, other);}
    Result mod(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("mod"), scopeMemory, other);}
    Result lt(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("lt"), scopeMemory, other);}
    Result le(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("le"), scopeMemory, other);}
    Result gt(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("gt"), scopeMemory, other);}
    Result ge(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("ge"), scopeMemory, other);}
    Result eq(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("eq"), scopeMemory, other);}
    Result neq(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("neq"), scopeMemory, other);}
    Result opand(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("and"), scopeMemory, other);}
    Result opor(BMemory* scopeMemory, const DataPtr& other) {return simpleImplement(variableManager.getId("or"), scopeMemory, other);}
    Result min(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("min"), scopeMemory);}
    Result max(BMemory* scopeMemory){return simpleImplement(variableManager.getId("max"), scopeMemory);}
    Result logarithm(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("log"), scopeMemory);}
    Result sum(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("sum"), scopeMemory);}
    Result opnot(BMemory* scopeMemory) {return simpleImplement(variableManager.getId("not"), scopeMemory);}
};

#endif // STRUCT_H
