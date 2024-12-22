#include "data/List.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/Iterator.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"
#include "data/BHashMap.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <mutex>

extern BError* OUT_OF_RANGE;

// BList constructor
BList::BList() : Data(LIST) {}
BList::BList(int reserve) : Data(LIST)  {
    contents.reserve(reserve);
}
BList::~BList() {
    for(Data* dat : contents)
        if(dat)
            dat->removeFromOwner();
}

std::string BList::toString(){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "[";
    for (const auto& element : contents) {
        if (result.size()!=1) 
            result += ", ";
        if(element)
            result += element->toString();
        else
            result += " ";
    }
    return result + "]";
}

Data* BList::at(int index) const {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (index < 0 || index >= contents.size()) 
        bberror("List index " + std::to_string(index) + " out of range [0," + std::to_string(contents.size()) + ")");
    auto res = contents.at(index);
    return res;
}

Result BList::implement(const OperationType operation, BuiltinArgs* args) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    
    if (args->size == 1) {
        switch (operation) {
            case LEN: return std::move(Result(new Integer(contents.size())));
            case TOITER: return std::move(Result(new AccessIterator(args->arg0)));
            case NEXT: {
                if (contents.empty()) 
                    return std::move(Result(OUT_OF_RANGE));
                auto ret = Result(contents.front());
                contents.erase(contents.begin());
                ret.get()->removeFromOwner();
                return std::move(ret);
            }
            case POP: {
                if (contents.empty()) 
                    return std::move(Result(OUT_OF_RANGE));
                auto ret = Result(contents.back());
                contents.pop_back();
                //ret.get()->removeFromOwner();
                return std::move(ret);
            }
            case TOMAP: {
                auto map = new BHashMap();
                int n = contents.size();
                for (int i = 0; i < n; ++i) {
                    if(contents[i]->getType()!=LIST)
                        bberror("Can only create a map from a list of 2-element lists");
                    BList* list = (BList*)contents[i];
                    if (!list || list->contents.size() != 2) 
                        bberror("Can only create a map from a list of 2-element lists");
                    map->put(list->contents[0], list->contents[1]);
                }
                return std::move(Result(map));
            }
            case TOVECTOR: {
                int n = contents.size();
                auto vec = new Vector(n, false);
                double* rawret = vec->data;
                BuiltinArgs args;
                //args.preallocResult = new BFloat(0);
                args.size = 1;
                try {
                    for (int i = 0; i < n; ++i) {
                        args.arg0 = contents.at(i);
                        Result tempValue = args.arg0->implement(TOBB_FLOAT, &args);
                        Data* temp = tempValue.get();
                        auto type = temp->getType();
                        if (type == BB_INT) 
                            rawret[i] = static_cast<Integer*>(temp)->getValue();
                        else if (type == BB_FLOAT) 
                            rawret[i] = static_cast<BFloat*>(temp)->getValue();
                        else 
                            bberror("Non-numeric value in list during conversion to vector");
                    }
                } 
                catch (BBError& e) {
                    throw e;
                }
                return std::move(Result(vec));
            }
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2) {
        if (args->arg1->getType() == BB_INT) {
            int index = static_cast<Integer*>(args->arg1)->getValue();
            // Manual implementation of BList::at() to avoid deadlocks with its own lock
            if (index < 0 || index >= contents.size()) 
                return std::move(Result(OUT_OF_RANGE));
            Data* res = contents.at(index);
            return std::move(Result(res));
        } 
        else if (args->arg1->getType() == STRUCT || args->arg1->getType() == LIST || args->arg1->getType() == ITERATOR) {
            BuiltinArgs implargs;
            implargs.size = 1;
            implargs.arg0 = args->arg1;

            Result iter = args->arg1->implement(TOITER, &implargs);
            Data* iterator = iter.get();
            bbassert(iterator && iterator->getType() == ITERATOR, "Can only find list indexes based on an iterable object, but a non-iterable struct was provided.");

            Iterator* iterPtr = static_cast<Iterator*>(iterator);

            // Handle contiguous iterators efficiently
            if (iterPtr->isContiguous()) {
                int start = iterPtr->getStart();
                int end = iterPtr->getEnd();
                if (start < 0 || start >= contents.size() || end < 0 || end > contents.size() || start > end) 
                    return std::move(Result(OUT_OF_RANGE));

                // Reserve size and copy elements
                BList* ret = new BList(end - start);
                for (int i = start; i < end; ++i) {
                    Data* element = contents.at(i);
                    element->addOwner();
                    ret->contents.push_back(element);
                }
                return std::move(Result(ret));
            } 
            else {
                // Handle non-contiguous iterators
                BList* ret = new BList(contents.size());  // Estimate size
                ret->contents.reserve(iterPtr->expectedSize());  // Use expectedSize() for better reservation

                while (true) {
                    implargs.size = 1;
                    Result next = iterator->implement(NEXT, &implargs);
                    Data* indexData = next.get();

                    if (!indexData) 
                        break;

                    bbassert(indexData->getType() == BB_INT, "Iterable list indexes can only contain integers.");
                    int id = static_cast<Integer*>(indexData)->getValue();

                    if (id < 0 || id >= contents.size())
                        return std::move(Result(OUT_OF_RANGE));

                    Data* element = contents.at(id);
                    element->addOwner();
                    ret->contents.push_back(element);
                }
                return std::move(Result(ret));
            }
        }
    }


    if (operation == PUSH && args->size == 2 && args->arg0 == this) {
        auto value = args->arg1;
        bbassert(value, "Cannot push a missing value to a list");
        value->leak();
        value->addOwner();
        contents.push_back((value));
        return std::move(Result(nullptr));
    }

    if (operation == PUT && args->size == 3 && args->arg1->getType() == BB_INT) {
        int index = static_cast<Integer*>(args->arg1)->getValue();
        if (index < 0 || index >= contents.size()) 
            bberror("List index " + std::to_string(index) + " out of range [0," + std::to_string(contents.size()) + ")");
        auto value = args->arg2;
        bbassert(value, "Cannot set a missing value on a list");
        Data* prev = contents[index];
        contents[index] = value;
        value->leak();
        value->addOwner();
        prev->removeFromOwner();
        return std::move(Result(nullptr));
    }
    
    if (operation == ADD && args->size == 2 && args->arg1->getType()==LIST) {
        BList* other = static_cast<BList*>(args->arg1);
        std::lock_guard<std::recursive_mutex> otherLock(other->memoryLock);
        BList* ret = new BList(contents.size()+other->contents.size());
        ret->contents.insert(ret->contents.end(), contents.begin(), contents.end());
        ret->contents.insert(ret->contents.end(), other->contents.begin(), other->contents.end());
        for(Data* dat : ret->contents)
            dat->addOwner();
        return std::move(Result(ret));
    }

    throw Unimplemented();
}
