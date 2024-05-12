// List.cpp
#include "List.h"
#include "Integer.h"
#include "Boolean.h"
#include "Iterator.h"
#include "common.h"
#include <iostream>

// ListContents constructor and methods
ListContents::ListContents() {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        std::cerr << "Failed to create a mutex for list read/write" << std::endl;
        exit(1);
    }
}

ListContents::~ListContents() {
    for(const Data* element : contents)
        if(element)
            delete element;
}

void ListContents::lock() {
    pthread_mutex_lock(&memoryLock);
}

void ListContents::unlock() {
    pthread_mutex_unlock(&memoryLock);
}

// List constructors
BList::BList() : contents(std::make_shared<ListContents>()) {}

BList::BList(const std::shared_ptr<ListContents>& cont) : contents(cont) {}

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
    return new BList(contents);
}

// Implement the specified operation
Data* BList::implement(const OperationType operation, BuiltinArgs* args) {
    if(args->size==1 && operation==TOCOPY)
        return new BList(contents);
    if(args->size==1 && operation==LEN)
        return new Integer(contents->contents.size());
    if(args->size==2 && args->arg0->getType()==LIST && args->arg1->getType()==INT && operation==AT) {
        contents->lock();
        int index = ((Integer*)args->arg1)->getValue();
        if(index < 0 || index>=contents->contents.size()) {
            std::cerr << "List index "<<index<<" out of range [0,"<<contents->contents.size()<<")\n";
            contents->unlock();
            exit(1);
            return nullptr;
        }
        Data* ret = contents->contents[index];
        contents->unlock();
        if(ret)
            ret = ret->shallowCopy();
        return ret;
    }
    if(args->size==1 && operation==TOITER) {
        contents->lock();
        Data* ret = new Iterator(std::make_shared<IteratorContents>(this));
        contents->unlock();
        return ret;
    }
    if(args->size==1 && operation==NEXT) {
        contents->lock();
        bool hasElements = contents->contents.size();
        Data* ret = hasElements?contents->contents.front():nullptr;
        if(hasElements)
            contents->contents.erase(contents->contents.begin());
        contents->unlock();
        if(ret)
            ret = ret->shallowCopy();
        return ret;
    }
    if(args->size==1 && operation==POP) {
        contents->lock();
        bool hasElements = contents->contents.size();
        Data* ret = hasElements?contents->contents.back():nullptr;
        if(hasElements)
            contents->contents.pop_back();
        contents->unlock();
        if(ret)
            ret = ret->shallowCopy();
        return ret;
    }

    if(args->size==2 && args->arg0->getType()==LIST && operation==PUSH) {
        contents->lock();
        contents->contents.push_back(args->arg1->shallowCopy());
        contents->unlock();
        return nullptr;
    }

    if(args->size==3 && args->arg0->getType()==LIST && args->arg1->getType()==INT && operation==PUT) {
        contents->lock();
        int index = ((Integer*)args->arg1)->getValue();
        int diff = index-contents->contents.size();
        for(int i=0;i<=diff;i++)
            contents->contents.push_back(nullptr);
        contents->contents[index] = args->arg2->shallowCopy();
        contents->unlock();
        return nullptr;
    }
    throw Unimplemented();
}
