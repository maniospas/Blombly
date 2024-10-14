#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/Boolean.h"
#include "common.h"
#include <iostream>
#include <mutex>

Iterator::Iterator(Data* object_) : object(object_), pos(new Integer(-1)), Data(ITERATOR) {
    BuiltinArgs args;
    args.arg0 = object;
    args.size = 1;
    Result lenValue = object->implement(LEN, &args);
    Data* len = lenValue.get();
    bbassert(len && len->getType()==BB_INT, "`len` failed to return an integer");
    size = static_cast<Integer*>(len)->getValue();
}

Iterator::~Iterator() {
    delete pos;
}

std::string Iterator::toString() const {
    return "iterator";
}

Result Iterator::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1 && operation == NEXT) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        pos->value += 1; 
        int currentPos = pos->value;
        if (currentPos >= size) 
            return std::move(Result(nullptr));

        args->arg0 = object;
        args->arg1 = pos;
        args->size = 2;
        return object->implement(AT, args);
    }
    
    if (args->size == 1 && operation == LEN) 
        return std::move(Result(new Integer(size)));
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}
