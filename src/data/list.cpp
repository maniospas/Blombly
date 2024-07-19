// List.cpp
#include "data/List.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/Iterator.h"
#include "common.h"
#include <iostream>

// ListContents constructor and methods
ListContents::ListContents(): lockable(0) {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        bberror("Failed to create a mutex for list read/write");
    }
}

ListContents::~ListContents() {
    for(const Data* element : contents)
        if(element && element->isDestroyable)
            delete element;
}

void ListContents::lock() {
    if(lockable)
        pthread_mutex_lock(&memoryLock);
}

void ListContents::unlock() {
    if(lockable)
        pthread_mutex_unlock(&memoryLock);
}

void ListContents::unsafeUnlock() {
    pthread_mutex_unlock(&memoryLock);
}

// List constructors
BList::BList() : contents(std::make_shared<ListContents>()) {}

BList::BList(const std::shared_ptr<ListContents>& cont) : contents(cont) {}

BList::~BList() {
    contents->lock();
    bool shouldUnlock = contents->lockable;
    contents->lockable -= 1;
    if(shouldUnlock)
        contents->unsafeUnlock();
}

// Return the type ID
int BList::getType() const {
    return LIST;
}

// Convert to string representation
std::string BList::toString() const {
    contents->lock();
    std::string result = "[";
    for (Data* element : contents->contents) {
        if (result.size() > 1) {
            result += ", ";
        }
        result += element->toString();
    }
    contents->unlock();
    return result + "]";
}

// Create a shallow copy of this List
Data* BList::shallowCopy() const {
    contents->lock();
    Data* ret = new BList(contents);
    bool shouldUnlock = contents->lockable;
    contents->lockable += 1;
    if(shouldUnlock)
        contents->unlock();
    return ret;
}

// Implement the specified operation
Data* BList::implement(const OperationType operation, BuiltinArgs* args) {
    if(args->size==1) {
        switch(operation) {
            case TOCOPY: return shallowCopyIfNeeded();
            case LEN: return new Integer(contents->contents.size());
            case TOITER: return new Iterator(std::make_shared<IteratorContents>(this));
            case NEXT:{
                contents->lock();
                bool hasElements = contents->contents.size();
                Data* ret = hasElements?contents->contents.front():nullptr;
                if(hasElements)
                    contents->contents.erase(contents->contents.begin());
                contents->unlock();
                return ret; // do not create shallow copy as the value does not remain in the list
            }
            case POP: {
                contents->lock();
                bool hasElements = contents->contents.size();
                Data* ret = hasElements?contents->contents.back():nullptr;
                if(hasElements)
                    contents->contents.pop_back();
                contents->unlock();
                return ret; // do not create shallow copy as the value does not remain in the list
            }
        }
        throw Unimplemented();
    }
    if(operation==AT && args->size==2 && args->arg1->getType()==INT) {
        contents->lock();
        int index = ((Integer*)args->arg1)->getValue();
        if(index < 0 || index>=contents->contents.size()) {
            int endcontents = contents->contents.size();
            contents->unlock();
            bberror("List index "+std::to_string(index)+" out of range [0,"+std::to_string(endcontents)+")");
            return nullptr;
        }
        Data* ret = contents->contents[index];
        contents->unlock();
        if(ret)
            ret = ret->shallowCopy();
        return ret;
    }

    if(operation==PUSH && args->size==2 && args->arg0==this) {
        contents->lock();
        contents->contents.push_back(args->arg1->shallowCopyIfNeeded());
        contents->unlock();
        return nullptr;
    }

    if(operation==PUT && args->size==3 && args->arg1->getType()==INT) {
        contents->lock();
        int index = ((Integer*)args->arg1)->getValue();
        int diff = index-contents->contents.size();
        for(int i=0;i<=diff;i++)
            contents->contents.push_back(nullptr);
        Data* prev = contents->contents[index];
        if(prev && prev->isDestroyable) // TODO: do not delete if primitives are going to be replaced
            delete prev;
        contents->contents[index] = args->arg2->shallowCopyIfNeeded();
        contents->unlock();
        return nullptr;
    }

    throw Unimplemented();
}
