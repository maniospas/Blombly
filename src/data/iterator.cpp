#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/Boolean.h"
#include "common.h"
#include <iostream>
#include <mutex>

Iterator::Iterator(const std::shared_ptr<Data>& object_) : object(object_), pos(std::make_shared<Integer>(-1)) {
    BuiltinArgs args;
    args.arg0 = object;
    args.size = 1;
    size = std::static_pointer_cast<Integer>(object->implement(LEN, &args))->getValue();
}

Iterator::~Iterator() = default;  // No manual mutex destruction required with std::mutex

int Iterator::getType() const {
    return ITERATOR;
}

std::string Iterator::toString() const {
    return "iterator";
}

std::shared_ptr<Data> Iterator::shallowCopy() const {
    bberror("Iterators cannot be returned outside of scope");
}

std::shared_ptr<Data> Iterator::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1 && operation == NEXT) {
        std::lock_guard<std::mutex> lock(memoryLock);
        pos->value += 1; 
        int currentPos = pos->value;
        if (currentPos >= size) 
            return nullptr;

        args->arg0 = object;
        args->arg1 = pos;
        args->size = 2;
        return object->implement(AT, args);
    }
    
    if (args->size == 1 && operation == LEN) 
        return std::make_shared<Integer>(size);
    if (args->size == 1 && operation == TOCOPY) 
        return shallowCopy();
    if (args->size == 1 && operation == TOITER) 
        return shallowCopy();
    throw Unimplemented();
}
