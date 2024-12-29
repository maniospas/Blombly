#include "data/BHashMap.h"
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BString.h"
#include "data/BFloat.h"
#include "data/Iterator.h"
#include "data/BError.h"
#include "common.h"

#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

extern BError* OUT_OF_RANGE;

BHashMap::BHashMap() : Data(MAP) {}
BHashMap::~BHashMap() {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    for (auto& bucket : contents) {
        for (auto& kvPair : bucket.second) {
            kvPair.first->removeFromOwner();
            kvPair.second->removeFromOwner();
        }
    }
    contents.clear();
}

std::string BHashMap::toString(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "{";
    bool firstEntry = true;
    for (const auto& bucket : contents) {
        for (const auto& kvPair : bucket.second) {
            if (!firstEntry) result += ", ";
            result += kvPair.first->toString(memory) + ": " + kvPair.second->toString(memory);
            firstEntry = false;
        }
    }
    result += "}";
    return result;
}

void BHashMap::put(Data* from, Data* to) {
    bbassert(from, "Missing key");
    bbassert(to, "Missing value");
    bbassert(from->getType()!=ERRORTYPE, "Cannot have an error as a map key");
    bbassert(to->getType()!=ERRORTYPE, "Cannot have an error as a map value");

    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    size_t keyHash = from->toHash();
    auto& entryList = contents[keyHash];

    for (auto& kvPair : entryList) {
        if (kvPair.first->isSame(from)) {
            Data* prevValue = kvPair.second;
            kvPair.second = to;            
            to->addOwner();
            prevValue->removeFromOwner();
            return;
        }
    }

    // If not found, we add a new key-value pair
    entryList.emplace_back(from, to);
    to->addOwner();
    from->addOwner();
}

Result BHashMap::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    // Typically, a method to handle 1-argument operations
    if (args->size == 1) {
        switch (operation) {
            case LEN: 
            {
                std::lock_guard<std::recursive_mutex> lock(memoryLock);
                int64_t count = 0;
                for (const auto& bucket : contents) {
                    count += bucket.second.size();
                }
                return Result(new Integer(count));
            }
            case TOITER:
                return Result(new AccessIterator(args->arg0));
            default:
                throw Unimplemented();
        }
    }

    if (operation == AT && args->size == 2) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Data* keyData = args->arg1;
        size_t keyHash = keyData->toHash();
        auto it = contents.find(keyHash);
        if (it == contents.end()) return Result(OUT_OF_RANGE);
        for (const auto& kvPair : it->second) if (kvPair.first->isSame(keyData)) return Result(kvPair.second);
        return Result(OUT_OF_RANGE);
    }

    if (operation == PUT && args->size == 3) {
        Data* keyData   = args->arg1;
        Data* valueData = args->arg2;
        put(keyData, valueData);
        return Result(nullptr);
    }

    throw Unimplemented();
}
