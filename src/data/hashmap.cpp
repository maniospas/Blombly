#include "data/BHashMap.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BString.h"
#include "data/BFloat.h"
#include "common.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "data/Iterator.h"
#include "tsl/hopscotch_map.h"

BHashMap::BHashMap() : Data(MAP) {}

BHashMap::~BHashMap() {}

std::string BHashMap::toString() const {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "{";
    for (const auto& pair : contents) {
        if (result.size() > 1) 
            result += ", ";
        result += pair.second->toString();
    }
    return result + "}";
}

void BHashMap::put(Data* from, Data* to) {
    bbassert(from, "Missing key value");
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    size_t key = from->toHash();
    auto& existing = contents[key];
    contents[key] = to;
}

Result BHashMap::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1) {
        switch (operation) {
            case LEN: return Result(new Integer(contents.size()));
            case TOITER: return Result(new Iterator(args->arg0));
        }
        throw Unimplemented();
    }
    if (operation == AT && args->size == 2) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        size_t key = args->arg1->toHash();
        auto it = contents.find(key);
        if (it == contents.end()) 
            return Result(nullptr);
        return Result(it->second);
    }

    if (operation == PUT && args->size == 3) {
        put(args->arg1, args->arg2);
        return Result(nullptr);
    }

    throw Unimplemented();
}
