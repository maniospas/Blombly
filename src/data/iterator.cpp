#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "common.h"
#include <iostream>
#include <mutex>


Iterator::Iterator() : Data(ITERATOR) {
}

std::string Iterator::toString() const {
    return "iterator";
}


AccessIterator::AccessIterator(Data* object_) : object(object_), pos(new Integer(-1)), Iterator() {
    BuiltinArgs args;
    args.arg0 = object;
    args.size = 1;
    Result lenValue = object->implement(LEN, &args);
    Data* len = lenValue.get();
    bbassert(len && len->getType()==BB_INT, "`len` failed to return an integer");
    size = static_cast<Integer*>(len)->getValue();
}

AccessIterator::~AccessIterator() {
    delete pos;
}
Result AccessIterator::implement(const OperationType operation, BuiltinArgs* args) {
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
    
    //if (args->size == 1 && operation == LEN) 
    //    return std::move(Result(new Integer(size)));
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}



IntRange::IntRange(int first, int last, int step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
    bbassert(step>0 == first<last || first==last, "A range iterator that never terminates was constructed (step should not have the opposite sign from last-first).");
}
Result IntRange::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1 && operation == NEXT) {
        if (first >= last) 
            return std::move(Result(nullptr));
        Result res = Result(new Integer(first));
        first += step;
        return std::move(res);
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}



FloatRange::FloatRange(double first, double last, double step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
    bbassert(step>0 == first<last || first==last, "A range iterator that never terminates was constructed (step should not have the opposite sign from last-first).");
}
Result FloatRange::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1 && operation == NEXT) {
        if (first >= last) 
            return std::move(Result(nullptr));
        Result res = Result(new BFloat(first));
        first += step;
        return std::move(res);
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}
