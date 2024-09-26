#include "data/Struct.h"
#include "data/Boolean.h"
#include "data/List.h"
#include "data/Code.h"
#include "BMemory.h"
#include <iostream>
#include <stdexcept>
#include "common.h"


Struct::Struct(const std::shared_ptr<BMemory>& mem) : memory(mem) {
}

Struct::~Struct() {
    memory.reset();  
}

int Struct::getType() const {
    return STRUCT;
}

std::string Struct::toString() const {
    try {
        BuiltinArgs args;
        args.size = 1;
        args.arg0 = shallowCopy();
        std::shared_ptr<Data> repr = args.arg0->implement(TOSTR, &args);
        return repr->toString();
    } catch (Unimplemented&) {
        return "struct";
    }
}

std::shared_ptr<BMemory> Struct::getMemory() const {
    return memory;
}

std::shared_ptr<Data> Struct::shallowCopy() const {
    return std::make_shared<Struct>(memory);
}

std::shared_ptr<Data> Struct::implement(const OperationType operation_, BuiltinArgs* args_) {
    if (args_->size == 1 && operation_ == TOCOPY) {
        return shallowCopy();
    }

    std::string operation = getOperationTypeName(operation_);
    auto mem = getMemory();
    auto implementation = mem->getOrNull(variableManager.getId("\\" + operation), true);

    if (!implementation) {
        throw Unimplemented();
    }

    auto args = std::make_shared<BList>(args_->size - 1);  // will be destroyed alongside the memory
    bbassert(args_->arg0.get() == this, "Must define \\" + operation + " for the first operand");

    if (args_->size > 1) {
        args->contents->push_back(args_->arg1->shallowCopy());
    }

    if (args_->size > 2) {
        args->contents->push_back(args_->arg2->shallowCopy());
    }

    bbassert(implementation->getType() == CODE, "\\" + operation + " is not a method");
    auto code = std::static_pointer_cast<Code>(implementation);

    std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(mem, LOCAL_EXPECTATION_FROM_CODE(code));
    newMemory->unsafeSet(variableManager.argsId, args, nullptr);

    BuiltinArgs builtinArgs;
    bool hasReturned(false);
    std::shared_ptr<Data> value = executeBlock(code, newMemory, hasReturned, builtinArgs);
    bbassert(hasReturned, "Implementation for \\" + operation + " did not return anything");
    newMemory.reset();
    args.reset();
    return value ? value->shallowCopy() : nullptr;
}
