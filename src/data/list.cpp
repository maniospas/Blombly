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
BList::BList(const std::vector<std::shared_ptr<Data>>& contents) : contents(contents) {}
BList::BList(const std::shared_ptr<BList>& list) : contents(list->contents) {}
BList::~BList() {}

int BList::getType() const {
    return LIST;
}

std::string BList::toString() const {
    std::lock_guard<std::mutex> lock(memoryLock);
    std::string result = "[";
    for (const auto& element : contents) {
        if (!result.empty()) {
            result += ", ";
        }
        result += element->toString();
    }
    return result + "]";
}

std::shared_ptr<Data> BList::shallowCopy() const {
    std::lock_guard<std::mutex> lock(memoryLock);
    auto copy = std::make_shared<BList>(contents);
    return copy;
}

std::shared_ptr<Data> BList::at(int index) const {
    std::lock_guard<std::mutex> lock(memoryLock);
    if (index < 0 || index >= contents.size()) {
        bberror("List index " + std::to_string(index) + " out of range [0," + std::to_string(contents.size()) + ")");
        return nullptr;
    }
    return contents[index] ? contents[index]->shallowCopy() : nullptr;
}

std::shared_ptr<Data> BList::implement(const OperationType operation, BuiltinArgs* args) {
    std::lock_guard<std::mutex> lock(memoryLock);
    
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY: return shallowCopy();
            case LEN: return std::make_shared<Integer>(contents.size());
            case TOITER: return std::make_shared<Iterator>(args->arg0);
            case NEXT: {
                if (contents.empty()) return nullptr;
                auto ret = contents.front();
                contents.erase(contents.begin());
                return ret;
            }
            case POP: {
                if (contents.empty()) return nullptr;
                auto ret = contents.back();
                contents.pop_back();
                return ret;
            }
            case TOMAP: {
                auto map = std::make_shared<BHashMap>();
                int n = contents.size();
                for (int i = 0; i < n; ++i) {
                    auto list = std::dynamic_pointer_cast<BList>(contents[i]);
                    if (!list || list->contents.size() != 2) {
                        bberror("Can only create a map from a list of 2-element lists");
                    }
                    map->put(list->contents[0], list->contents[1]);
                }
                return map;
            }
            case TOVECTOR: {
                int n = contents.size();
                auto vec = std::make_shared<Vector>(n, false);
                double* rawret = vec->data.get();
                BuiltinArgs* args = new BuiltinArgs();
                args->preallocResult = std::make_shared<BFloat>(0);
                args->size = 1;
                try {
                    for (int i = 0; i < n; ++i) {
                        args->arg0 = contents[i];
                        auto temp = args->arg0->implement(TOFLOAT, args);
                        auto type = temp->getType();
                        if (type == INT) {
                            rawret[i] = static_cast<Integer*>(temp.get())->getValue();
                        } else if (type == FLOAT) {
                            rawret[i] = static_cast<BFloat*>(temp.get())->getValue();
                        } else {
                            bberror("Non-numeric value in list during conversion to vector");
                        }
                    }
                } catch (BBError& e) {
                    delete args;
                    throw e;
                }
                delete args;
                return vec;
            }
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2 && args->arg1->getType() == INT) {
        int index = static_cast<Integer*>(args->arg1.get())->getValue();
        return at(index);
    }

    if (operation == PUSH && args->size == 2 && args->arg0.get() == this) {
        if (args->arg1) 
            contents.push_back(args->arg1->shallowCopy());
        else 
            contents.push_back(nullptr);
        return nullptr;
    }

    if (operation == PUT && args->size == 3 && args->arg1->getType() == INT) {
        int index = static_cast<Integer*>(args->arg1.get())->getValue();
        if (index >= contents.size()) 
            contents.resize(index + 1);
        contents[index] = args->arg2 ? args->arg2->shallowCopy() : nullptr;
        return nullptr;
    }

    throw Unimplemented();
}
