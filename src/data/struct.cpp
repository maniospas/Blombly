#include "data/Struct.h"
#include "data/Boolean.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include <iostream>
#include <stdexcept>
#include "common.h"


std::string Struct::toString() const {
    try {
        BuiltinArgs args;
        args.size = 1;
        args.arg0 = (Data*)this;
        Data* repr = args.arg0->implement(TOSTR, &args);
        return repr->toString();
    } catch (Unimplemented&) {
        return "struct";
    }
}

Data* Struct::implement(const OperationType operation_, BuiltinArgs* args_) {
    if (args_->size == 1 && operation_ == TOCOPY) {
        bberror("Cannot copy structs");
    }

    std::string operation = getOperationTypeName(operation_);
    auto mem = getMemory();
    auto implementation = mem->getOrNull(variableManager.getId("\\" + operation), true);

    if (!implementation) {
        throw Unimplemented();
    }

    auto args = new BList(args_->size - 1);  // will be destroyed alongside the memory
    bbassert(args_->arg0 == this, "Must define \\" + operation + " for the first operand");

    if (args_->size > 1) 
        args->contents.push_back(args_->arg1);

    if (args_->size > 2) 
        args->contents.push_back(args_->arg2);

    bbassert(implementation->getType() == CODE, "\\" + operation + " is not a method");
    Code* code = (Code*)implementation;

    BMemory* newMemory = new BMemory(mem, LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory->unsafeSet(variableManager.argsId, args, nullptr);

    bool hasReturned(false);
    Data* value = executeBlock(code, newMemory, hasReturned);
    bbassert(hasReturned, "Implementation for \\" + operation + " did not return anything");
    return value;
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
    if(counter==1 || counter==0) {// its held memory will always hold this, in which case we do want a destruction
        //std::cout << "destroying "<<toString()<<" "<<this<<"\n";
        delete this;
    }
    //else 
    //    std::cout << "refusing to destroy "<<this<<toString()<<referenceCounter<<"\n";
}