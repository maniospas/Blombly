#include "data/Struct.h"
#include "data/Boolean.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include <iostream>
#include <stdexcept>


int Struct::getType() const {
    return STRUCT;
}

std::string Struct::toString() const {
    try {
        BuiltinArgs args;
        args.size = 1;
        args.arg0 = (Data*)this;
        Data* repr = args.arg0->implement(TOSTR, &args);
        return repr->toString();
    } catch(Unimplemented) {
        return "struct";
    }
}


Data* Struct::implement(const OperationType operation_, BuiltinArgs* args_) {
    if(args_->size == 1 && operation_ == TOCOPY)
        return shallowCopy();
    std::string operation = getOperationTypeName(operation_);
    BMemory* memory = this->getMemory();
    Data* implementation = memory->getOrNull(variableManager.getId("\\"+operation), true);
    if (!implementation)
        throw Unimplemented();
    BList* args = new BList();
    args->contents->contents.reserve(args_->size - 1);
    if (args_->size)
        args->contents->contents.push_back(args_->arg0->shallowCopy());
    if (args_->size > 1)
        args->contents->contents.push_back(args_->arg1->shallowCopy());
    if (args_->size > 2)
        args->contents->contents.push_back(args_->arg2->shallowCopy());
    if (implementation->getType() == CODE) {
        Code* code = (Code*)implementation;
        BMemory* newMemory = new BMemory(memory, LOCAL_EXPACTATION_FROM_CODE(code));
        newMemory->unsafeSet(variableManager.argsId, args, nullptr);
        Data* value = executeBlock(code, newMemory, nullptr, nullptr);
        return value;
    } else {
        bberror("\\"+operation + " is not a method");
        delete args;  // Note: This line will never be executed due to exit.
    }
    throw Unimplemented();
}


Data* GlobalStruct::shallowCopy() const {
    return new GlobalStruct(memory);
}


GlobalStruct::GlobalStruct(BMemory*  mem) : memory(mem) {
    memory->countDependencies.fetch_add(1, std::memory_order_relaxed);
}
GlobalStruct::~GlobalStruct() {
    int deps = memory->countDependencies.fetch_sub(-1, std::memory_order_relaxed);
    if(deps==1)
        delete memory;
}
BMemory* GlobalStruct::getMemory() const {return memory;}
void GlobalStruct::lock() const {memory->lock();}
void GlobalStruct::unlock() const {memory->unlock();}
