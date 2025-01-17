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

std::string Iterator::toString(BMemory* memory){
    return "iterator";
}


AccessIterator::AccessIterator(DataPtr object_) : object(object_), pos(new Integer(-1)), Iterator(), size(-1) {
}

AccessIterator::~AccessIterator() {
    delete pos;
}

Result AccessIterator::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if(size==-1) {
        BuiltinArgs args;
        args.arg0 = object;
        args.size = 1;
        Result lenValue = object->implement(LEN, &args, memory);
        DataPtr len = lenValue.get();
        bbassert(len.exists() && len->getType()==BB_INT, "`len` failed to return an integer");
        size = static_cast<Integer*>(len.get())->getValue();
    }

    if (args->size == 1 && operation == NEXT) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        pos->value += 1; 
        int64_t currentPos = pos->value;
        if (currentPos >= size) 
            return std::move(Result(OUT_OF_RANGE));

        args->arg0 = object;
        args->arg1 = pos;
        args->size = 2;
        return std::move(object->implement(AT, args, memory));
    }
    
    //if (args->size == 1 && operation == LEN) 
    //    return std::move(Result(new Integer(size)));
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}



IntRange::IntRange(int64_t first, int64_t last, int64_t step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
    //cache = new Integer(first);
    //cache->addOwner();
}
IntRange::~IntRange() {
}
Result IntRange::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (args->size == 1 && operation == NEXT) {
        if (step>0 && first >= last) 
            return std::move(Result(OUT_OF_RANGE));
        if (step<0 && first <= last) 
            return std::move(Result(OUT_OF_RANGE));
        Integer* cache = new Integer(first);
        /*if(cache->referenceCounter<=3) { // this, the temporary variable holding the value, and the named variable with the value assigned
            cache->setValue(first);
        }
        else {
            cache->removeFromOwner();
            cache = new Integer(first);
            cache->addOwner();
        }*/
        first += step;
        return std::move(Result(cache));
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}

DataPtr IntRange::fastNext() {
    if (step>0 && first >= last) 
        return OUT_OF_RANGE;
    if (step<0 && first <= last) 
        return OUT_OF_RANGE;
    Integer* cache = new Integer(first);
    first += step;
    return cache;
}


FloatRange::FloatRange(double first, double last, double step) : first(first), last(last), step(step), Iterator() {
    bbassert(step, "A range iterator with zero step never ends.");
    //cache = new BFloat(first);
    //cache->addOwner();
}
FloatRange::~FloatRange() {
}
Result FloatRange::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (args->size == 1 && operation == NEXT) {
        if (step>0 && first >= last) 
            return std::move(Result(OUT_OF_RANGE));
        if (step<0 && first <= last) 
            return std::move(Result(OUT_OF_RANGE));
        BFloat* cache = new BFloat(first);
        /*if(cache->referenceCounter<=3) { // this, the temporary variable holding the value, and the named variable with the value assigned
            cache->setValue(first);
        }
        else {
            cache->removeFromOwner();
            cache = new BFloat(first);
            cache->addOwner();
        }*/
        first += step;
        return std::move(Result(cache));
    }
    if (args->size == 1 && operation == TOITER) 
        return std::move(Result(this));
    throw Unimplemented();
}

DataPtr FloatRange::fastNext() {
    if (step>0 && first >= last) 
        return OUT_OF_RANGE;
    if (step<0 && first <= last) 
        return OUT_OF_RANGE;
    BFloat* cache = new BFloat(first);
    first += step;
    return cache;
}