// List.cpp
#include "List.h"
#include "Integer.h"
#include "Boolean.h"
#include "common.h"
#include <iostream>

// ListContents constructor and methods
ListContents::ListContents() {
    if (pthread_mutex_init(&memoryLock, nullptr) != 0) {
        std::cerr << "Failed to create a mutex for list read/write" << std::endl;
        exit(1);
    }
}

void ListContents::lock() {
    pthread_mutex_lock(&memoryLock);
}

void ListContents::unlock() {
    pthread_mutex_unlock(&memoryLock);
}

// List constructors
List::List() : contents(std::make_shared<ListContents>()) {}

List::List(std::shared_ptr<ListContents> cont) : contents(cont) {}

// Return the type ID
int List::getType() const {
    return LIST;
}

// Convert to string representation
std::string List::toString() const {
    contents->lock();
    std::string result = "[";
    for (const auto& element : contents->contents) {
        if (result.size() > 1) {
            result += ", ";
        }
        result += element->toString();
    }
    contents->unlock();
    return result + "]";
}

// Create a shallow copy of this List
std::shared_ptr<Data> List::shallowCopy() const {
    return std::make_shared<List>(contents);
}

// Implement the specified operation
std::shared_ptr<Data> List::implement(const OperationType operation, const BuiltinArgs& args) {
    if(args.size==1 && args.arg0->getType()==LIST && operation==TOCOPY)
        return std::make_shared<List>(contents);
    if(args.size==1 && args.arg0->getType()==LIST && operation==LEN)
        return std::make_shared<Integer>(contents->contents.size());
    if(args.size==2 && args.arg0->getType()==LIST && args.arg1->getType()==INT && operation==AT) {
        contents->lock();
        int index = std::static_pointer_cast<Integer>(args.arg1)->getValue();
        if(index < 0 || index>=contents->contents.size()) {
            std::cerr << "List index "<<index<<" out of range [0,"<<contents->contents.size()<<")\n";
            contents->unlock();
            exit(1);
            return nullptr;
        }
        std::shared_ptr<Data> ret = contents->contents[index];
        contents->unlock();
        return ret;
    }
    if(args.size==2 && args.arg0->getType()==LIST && operation==PUSH) {
        contents->lock();
        contents->contents.push_back(args.arg1);
        contents->unlock();
        return args.arg0;
    }
    if(args.size==2 && args.arg1->getType()==LIST && operation==POP) {
        contents->lock();
        contents->contents.push_back(args.arg0);
        contents->unlock();
        return args.arg1;
    }
    if(args.size==1 && args.arg0->getType()==LIST && operation==POP) {
        contents->lock();
        std::shared_ptr<Data> ret = contents->contents.size()?contents->contents[contents->contents.size()-1]:nullptr;
        if(contents->contents.size())
            contents->contents.pop_back();
        contents->unlock();
        return ret;
    }
    if(args.size==1 && args.arg0->getType()==LIST && operation==POLL) {
        contents->lock();
        std::shared_ptr<Data> ret = contents->contents.size()?contents->contents[0]:nullptr;
        if(contents->contents.size())
            contents->contents.erase( contents->contents.begin());
        contents->unlock();
        return ret;
    }
    if(args.size==1 && args.arg0->getType()==LIST && operation==POP) {
        contents->lock();
        std::shared_ptr<Data> ret = contents->contents.size()?contents->contents[contents->contents.size()-1]:nullptr;
        if(contents->contents.size())
            contents->contents.pop_back();
        contents->unlock();
        return ret;
    }
    if(args.size==3 && args.arg0->getType()==LIST && args.arg1->getType()==INT && operation==PUT) {
        contents->lock();
        int index = std::static_pointer_cast<Integer>(args.arg1)->getValue();
        int diff = index-contents->contents.size();
        for(int i=0;i<=diff;i++)
            contents->contents.push_back(nullptr);
        contents->contents[index] = args.arg2;
        contents->unlock();
        return nullptr;
    }
    throw Unimplemented();
}
