// List.cpp
#include "data/List.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/Iterator.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"
#include "data/BHashMap.h"
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

Data* BList::at(int index) const {
    contents->lock();
    if(index < 0 || index>=contents->contents.size()) {
        int endcontents = contents->contents.size();
        contents->unlock();
        bberror("List index "+std::to_string(index)+" out of range [0,"+std::to_string(endcontents)+")");
        return nullptr;
    }
    Data* ret = contents->contents[index];
    if(ret)
        ret->shallowCopyIfNeeded();
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
            case TOMAP : {
                BHashMap* map = new BHashMap();
                contents->lock();
                int n = contents->contents.size();
                int i = 0;
                try {
                    for(;i<n;++i) {
                        Data* el = contents->contents[i];
                        bbassert(el->getType()==LIST, "Can only create a map from a list of lists");
                        BList* list = ((BList*)el);
                        list->contents->lock();
                        if(list->contents->contents.size()!=2) {
                            list->contents->unlock();
                            bberror("Can only create a map from a list of 2-element lists");
                        }
                        std::cout << list->contents->contents[0]->toString()<<"\n";
                        map->put(list->contents->contents[0], list->contents->contents[1]);
                        list->contents->unlock();
                    }
                }
                catch(BBError error) {
                    contents->unlock();
                    delete map;
                    throw error;
                }
                contents->unlock();
                return map;
            }
            case TOVECTOR: {
                contents->lock();
                int n = contents->contents.size();
                Vector* ret = new Vector(n, false); // do not fill with zeros, as we will fill with whatever values we obtain
                int i = 0;
        		double* rawret = ret->value->data;
        		BuiltinArgs* args = new BuiltinArgs();
        		args->preallocResult = new BFloat(0);  // preallocate this intermediate result to speed things up in case a lot of floats are moved around (this is still slower for ints)
        		args->size = 1;
        		try {
	                for(;i<n;++i) {
        		 		args->arg0 = contents->contents[i];
					bbassert(args->arg0, "Convertion of a list element to float encountered a missing value");
					Data* temp = args->arg0->implement(TOFLOAT, args);
					auto type = temp->getType();
					bbassert(type==FLOAT || type==INT, "Convertion of a list element to float returned a non-float and non-int value");
					double value = type==INT?((Integer*)temp)->value:((BFloat*)temp)->value;
					rawret[i] = value;
	                }
        		}
        		catch(BBError e) {
        		 	delete ret;
        		 	delete args->preallocResult;
        		 	delete args;
                contents->unlock();
        		 	throw e;
        		}
        		delete args->preallocResult;
        		delete args;
                contents->unlock();
                return ret;
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
        if(ret) {
            auto type = ret->getType();
            Data* res = args->preallocResult;
            if(res && res->getType()==type) {
                if(type==INT) {
                    ((Integer*)args->preallocResult)->value = ((Integer*)res)->value;
                    ret = res;
                }
                else if(type==FLOAT) {
                    ((BFloat*)args->preallocResult)->value = ((BFloat*)res)->value;
                    ret = res;
                }
                else if(type==BOOL) {
                    ((Boolean*)args->preallocResult)->value = ((Boolean*)res)->value;
                    ret = res;
                }
                else if(type==STRING) {
                    ((BString*)args->preallocResult)->value = ((BString*)res)->value;
                    ret = res;
                }
            }
            else 
                ret = ret->shallowCopy();
        }
        contents->unlock();
        return ret;
    }

    if(operation==PUSH && args->size==2 && args->arg0==this) {
        contents->lock();
        if(args->arg1)
            contents->contents.push_back(args->arg1->shallowCopyIfNeeded());
        else
            contents->contents.push_back(nullptr);
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
