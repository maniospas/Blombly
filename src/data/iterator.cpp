#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <mutex>

extern BError* OUT_OF_RANGE;


Iterator::Iterator() : Data(ITERATOR) {
}

std::string Iterator::toString(){
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
            return std::move(Result(OUT_OF_RANGE));

        args->arg0 = object;
        args->arg1 = pos;
        args->size = 2;
        return std::move(object->implement(AT, args));
    }
    
    //if (args->size == 1 && operation == LEN) 
    //    return std::move(Result(new Integer(size)));
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}



IntRange::IntRange(int first, int last, int step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
}
IntRange::~IntRange() {
}
Result IntRange::implement(const OperationType operation, BuiltinArgs* args) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (args->size == 1 && operation == NEXT) {
        if (step>0 && first >= last) 
            return std::move(Result(OUT_OF_RANGE));
        if (step<0 && first <= last) 
            return std::move(Result(OUT_OF_RANGE));
        Result ret = Result(new Integer(first));
        first += step;
        return std::move(ret);
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}



FloatRange::FloatRange(double first, double last, double step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
}
FloatRange::~FloatRange() {
}
Result FloatRange::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1 && operation == NEXT) {
        if (step>0 && first >= last) 
            return std::move(Result(nullptr));
        if (step<0 && first <= last) 
            return std::move(Result(nullptr));
        Result ret = Result(new BFloat(first));
        first += step;
        return std::move(ret);
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}
