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

BHashMap::BHashMap() {}

BHashMap::~BHashMap() {}

int BHashMap::getType() const {
    return MAP;
}

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

std::shared_ptr<Data> BHashMap::shallowCopy() const {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    auto copy = std::make_shared<BHashMap>();
    copy->contents = contents;
    return copy;
}

void BHashMap::put(const std::shared_ptr<Data>& from, const std::shared_ptr<Data>& to) {
    bbassert(from, "Missing key value");

    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    size_t key = from->toHash();
    auto& existing = contents[key];
    if (existing && existing->isDestroyable) 
        existing.reset();  // Safely destroy the previous object
    contents[key] = INLINE_SCOPY(to);
}

std::shared_ptr<Data> BHashMap::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY: return shallowCopy();
            case LEN: return std::make_shared<Integer>(contents.size());
            case TOITER: return std::make_shared<Iterator>(args->arg0);
        }
        throw Unimplemented();
    }
    if (operation == AT && args->size == 2) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        size_t key = args->arg1->toHash();
        auto it = contents.find(key);
        if (it == contents.end()) 
            return nullptr;
        auto res = it->second;
        SCOPY(res);
        return res;
    }

    if (operation == PUT && args->size == 3) {
        put(args->arg1, args->arg2);
        return nullptr;
    }

    throw Unimplemented();
}
