// iterwwatwor.cpp
#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/Boolean.h"
#include "common.h"
#include <iostream>

// ListContents constructor and methods
IteratorContents::IteratorContents(Data* object_):object(object_) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for list read/write");
    }
    BuiltinArgs args;
    args.arg0 = object;
    args.size = 1;
    pos = new Integer(-1);
    size = ((Integer*)object->implement(LEN, &args))->getValue();
    locked = -1; // start from -1 so that first object getting this goes to 0
}

IteratorContents::~IteratorContents() {
    delete pos;
    //if(locked) // basically locked!=0 
    //    delete object;
}

void IteratorContents::lock() {
    //if(locked)
    //    pthread_mutex_lock(&memoryLock);
}

void IteratorContents::unlock() {
    //if(locked)
    //    pthread_mutex_unlock(&memoryLock);
}

void IteratorContents::unsafeUnlock() {
    //pthread_mutex_unlock(&memoryLock);
}


// List constructors

Iterator::Iterator(const std::shared_ptr<IteratorContents>& contents_) : contents(contents_) {
    contents->lock();
    bool shouldUnlock = contents->locked;
    contents->locked += 1;
    if(shouldUnlock)
        contents->unsafeUnlock();
}

Iterator::~Iterator() {
    contents->lock();
    bool shouldUnlock = contents->locked;
    contents->locked -= 1;
    if(shouldUnlock)
        contents->unsafeUnlock();
}

// Return the type ID
int Iterator::getType() const {
    return ITERATOR;
}

// Convert to string representation
std::string Iterator::toString() const {
    return "iterator";
}

// Create a shallow copy of this List
Data* Iterator::shallowCopy() const {
    bberror("Iterators cannot be returned outside of scope");
    /*
    contents->lock();
    std::shared_ptr<IteratorContents> newContents = std::make_shared<IteratorContents>(contents->object->shallowCopyIfNeeded());
    newContents->pos = contents->pos;
    newContents->size = contents->size;
    newContents->locked = -1;
    contents->unlock();
    return new Iterator(newContents);*/
}

// Implement the specified operation
Data* Iterator::implement(const OperationType operation, BuiltinArgs* args) {
    if(args->size==1 && operation==NEXT) {
        contents->lock();
        contents->pos->value += 1; // can get away with not passing a new integer to Data::run because overloaded operators are run in the same thread
        int pos = contents->pos->value;
        if(pos>=contents->size) {
            contents->unlock();
            return nullptr;
        }
        // keep the return type as-is
        args->arg0 = contents->object;
        args->arg1 = contents->pos;
        args->size = 2;
        contents->unlock();

        Data* ret = contents->object->implement(AT, args);//Data::run(AT, args); // run outside locks to prevent deadlocks
        return ret;
    }
    if(args->size==1 && operation==LEN)
        return new Integer(contents->size);
    if(args->size==1 && operation==TOCOPY)
        return shallowCopy();
    if(args->size==1 && operation==TOITER) 
        return shallowCopy();//std::make_shared<Iterator>(std::make_shared<IteratorContents>(contents->object));

    throw Unimplemented();
}
