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
#include <mutex>

// BList constructor
BList::BList()  {}
BList::BList(int reserve)  {
    contents.reserve(reserve);
}
BList::~BList() {}

int BList::getType() const {
    return LIST;
}

std::string BList::toString() const {
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

Data* BList::implement(const OperationType operation, BuiltinArgs* args) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    
    if (args->size == 1) {
        switch (operation) {
            case LEN: return new Integer(contents.size());
            case TOITER: return new Iterator(args->arg0);
            case NEXT: {
                if (contents.empty()) return nullptr;
                auto ret = (contents.front());
                contents.erase(contents.begin());
                return ret;
            }
            case POP: {
                if (contents.empty()) return nullptr;
                auto ret = (contents.back());
                contents.pop_back();
                return (ret);
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
                return map;
            }
            case TOVECTOR: {
                int n = contents.size();
                auto vec = new Vector(n, false);
                double* rawret = vec->data;
                BuiltinArgs args;
                args.preallocResult = new BFloat(0);
                args.size = 1;
                try {
                    for (int i = 0; i < n; ++i) {
                        args.arg0 = contents.at(i);
                        auto temp = args.arg0->implement(TOBB_FLOAT, &args);
                        auto type = temp->getType();
                        if (type == BB_INT) {
                            rawret[i] = static_cast<Integer*>(temp)->getValue();
                        } 
                        else if (type == BB_FLOAT) {
                            rawret[i] = static_cast<BFloat*>(temp)->getValue();
                        } 
                        else {
                            bberror("Non-numeric value in list during conversion to vector");
                        }
                    }
                } 
                catch (BBError& e) {
                    throw e;
                }
                return vec;
            }
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        int index = static_cast<Integer*>(args->arg1)->getValue();
        // manual implementation of at to avoid deadlocks with its own lock
        if (index < 0 || index >= contents.size()) 
            bberror("List index " + std::to_string(index) + " out of range [0," + std::to_string(contents.size()) + ")");
        auto res = contents.at(index);
        return res;
    }   

    if (operation == PUSH && args->size == 2 && args->arg0 == this) {
        auto value = args->arg1;
        contents.push_back((value));
        return nullptr;
    }

    if (operation == PUT && args->size == 3 && args->arg1->getType() == BB_INT) {
        int index = static_cast<Integer*>(args->arg1)->getValue();
        if (index >= contents.size()) 
            contents.resize(index + 1);
        auto value = args->arg2;
        contents[index] = value;
        return nullptr;
    }

    throw Unimplemented();
}
