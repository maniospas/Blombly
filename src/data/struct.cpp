#include "data/Struct.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include "interpreter/functional.h"
#include <iostream>
#include <stdexcept>
#include "common.h"


std::string Struct::toString(BMemory* memory){
    try {
        BuiltinArgs args;
        args.size = 1;
        args.arg0 = (DataPtr)this;
        Result reprValue = args.arg0->implement(TOSTR, &args, memory);
        DataPtr repr = reprValue.get();
        return repr->toString(memory);
    } catch (Unimplemented&) {
        return "struct";
    }
}

inline Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory) {
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

inline Result Struct::simpleImplement(int implementationCode, BMemory* calledMemory, const DataPtr& other) {
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

Result Struct::implement(const OperationType operation_, BuiltinArgs* args_, BMemory* calledMemory) {
    //if (args_->size == 1 && operation_ == TOCOPY) {
    //    bberror("Cannot copy structs");
    //}

    std::string operation = getOperationTypeName(operation_);
    BMemory* mem;
    DataPtr implementation;
    {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        mem = getMemory();
        implementation = mem->getOrNullShallow(variableManager.getId(operation));

        if(!implementation.exists()) {
            if(operation_==CLEAR) {
                delete memory;
                memory = new BMemory(nullptr, 0);
                return std::move(Result(nullptr));
            }
            if(operation_==MOVE) {
                memory = new BMemory(nullptr, 1);
                memory->unsafeSet(variableManager.thisId, this);
                DataPtr ret = new Struct(mem);
                mem->unsafeSet(variableManager.thisId, ret);
                return std::move(Result(ret));
            }
        }
    }

    bbassert(implementation.exists(), "Must define " + operation + " for the struct");
    bbassert(implementation->getType() == CODE, operation + " is not a method");

    if (!implementation.exists()) {
        //return std::move(Result(nullptr));
        throw Unimplemented();
    }

    bbassert(args_->arg0 == this, "The first argument must be the struct itself");
    BList* args = new BList(args_->size - 1);  // will be destroyed alongside the memory
    if (args_->size > 1) {
        args_->arg1->addOwner();
        args->contents.push_back(args_->arg1);
    }
    if (args_->size > 2) {
        args_->arg2->addOwner();
        args->contents.push_back(args_->arg2);
    }

    Code* code = static_cast<Code*>(implementation.get());
    BMemory newMemory(calledMemory->getParentWithFinals(), LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.thisId, this);
    newMemory.unsafeSet(variableManager.argsId, args);

    ExecutionInstance executor(code, &newMemory, true);
    Result value = executor.run(code);
    newMemory.setToNullIgnoringFinals(variableManager.thisId);
    bbassert(executor.hasReturned() || operation_==PUT || operation_==PUSH || operation_==CLEAR, "Implementation for `" + operation + "` did not return anything");
    return Result(value);
}

Struct::Struct(BMemory* mem) : memory(mem), Data(STRUCT) {}
Struct::~Struct() {
    delete memory;
}
BMemory* Struct::getMemory() const {
    return memory;
}
void Struct::removeFromOwner() {
    int counter = --referenceCounter;
    //std::cout << "removing "<<this<<" from "<<data<<" with counter "<<referenceCounter<<"\n";
    if(counter<=1) {// its held memory will always hold this, in which case we do want a destruction
        //std::cout << "destroying "<<toString()<<" "<<this<<"\n";
        //std::lock_guard<std::recursive_mutex> lock(memoryLock);
        //memory->data[variableManager.thisId] = nullptr;
        //memory->release();

        //std::lock_guard<std::recursive_mutex> lock(memoryLock);
        //memory->data[variableManager.thisId] = nullptr;
        //memory->release();
        delete this;
    }
    //else 
    //    std::cout << "refusing to destroy "<<this<<toString()<<referenceCounter<<"\n";
}