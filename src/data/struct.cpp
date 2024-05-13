#include "data/Struct.h"
#include "data/Boolean.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include <iostream>
#include <stdexcept>

Struct::Struct(const std::shared_ptr<BMemory>& mem) : memory(mem) {}

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

std::shared_ptr<BMemory> Struct::getMemory() {
    return memory;
}

void Struct::lock() {
    memory->lock();
}

void Struct::unlock() {
    memory->unlock();
}

Data* Struct::shallowCopy() const {
    return new Struct(memory);
}

Data* Struct::implement(const OperationType operation_, BuiltinArgs* args_) {
    if(args_->size == 1 && args_->arg0->getType() == STRUCT && operation_ == TOCOPY)
        return new Struct(memory);
    std::string operation = getOperationTypeName(operation_);
    std::shared_ptr<BMemory> memory = this->memory;
    Data* implementation = memory->getOrNull(variableManager.getId(operation), true);
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
    args->isMutable = false;
    if (implementation->getType() == CODE) {
        Code* code = (Code*)implementation;
        std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPACTATION_FROM_CODE(code));
        newMemory->set(variableManager.argsId, args);
        std::vector<Command*>* program = (std::vector<Command*>*)code->getProgram();
        Data* value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
        return value;
    } else {
        std::cerr << operation << " is not a method\n";
        exit(1);
        delete args;  // Note: This line will never be executed due to exit.
    }
    throw Unimplemented();
}
