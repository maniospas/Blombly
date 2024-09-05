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
    if (!implementation)// || args_->arg0!=memory->getOrNullShallow(variableManager.thisId))
        throw Unimplemented();
    BList* args = new BList();  // will be destroyed alongside the memory
    args->contents->contents.reserve(args_->size - 1);
    bbassert(args_->arg0==this, "\\Must define "+operation + " for the first operand");
    if (args_->size > 1)
        args->contents->contents.push_back(args_->arg1->shallowCopyIfNeeded());
    if (args_->size > 2)
        args->contents->contents.push_back(args_->arg2->shallowCopyIfNeeded());
    bbassert(implementation->getType() == CODE, "\\"+operation + " is not a method");
    Code* code = (Code*)implementation;
    BMemory newMemory(memory, LOCAL_EXPACTATION_FROM_CODE(code));
    newMemory.unsafeSet(variableManager.argsId, args, nullptr);
    Data* value = executeBlock(code, &newMemory, nullptr, nullptr);
    if(value)
        value = value->shallowCopy(); // TODO: check this
    return value;
}


Data* GlobalStruct::shallowCopy() const {
    return new GlobalStruct(memory);
}


GlobalStruct::GlobalStruct(BMemory*  mem) : memory(mem) {
    memory->countDependencies.fetch_add(1, std::memory_order_relaxed);
}
GlobalStruct::~GlobalStruct() {
    int deps = memory->countDependencies.fetch_sub(-1, std::memory_order_relaxed);
    if(deps==1) {
        isDestroyable = false;  // ignore the destruction caused by the parent memory deletion
        delete memory;
    }
}
BMemory* GlobalStruct::getMemory() const {return memory;}
void GlobalStruct::lock() const {memory->lock();}
void GlobalStruct::unlock() const {memory->unlock();}
