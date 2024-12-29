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

std::string BHashMap::toString(BMemory* memory){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "{";
    for (const auto& pair : contents) {
        if (result.size() > 1) 
            result += ", ";
        for (const auto& item : pair.second) {
            result += item.first->toString(memory) + ": " + item.second->toString(memory);
        }
    }
    return result + "}";
}

void BHashMap::put(Data* from, Data* to) {
    bbassert(from, "Missing key");
    bbassert(to, "Missing value");
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    size_t key = from->toHash();
    auto& entryList = contents[key];

    // Check if an equivalent key exists and overwrite its value if found
    for (auto& pair : entryList) {
        if (pair.first->isSame(from)) {
            Data* prev = pair.second; 
            pair.second = to;
            to->addOwner(); // don't add owner to "from" as we are keeping the old one
            prev->removeFromOwner();
            return;
        }
    }
    entryList.emplace_back(from, to);
    to->addOwner();
    from->addOwner();
}

Result BHashMap::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (args->size == 1) {
        switch (operation) {
            case LEN: return std::move(Result(new Integer(contents.size())));
            case TOITER: return std::move(Result(new AccessIterator(args->arg0)));
        }
        throw Unimplemented();
    }
    if (operation == AT && args->size == 2) {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        size_t key = args->arg1->toHash();
        auto it = contents.find(key);
        if (it == contents.end()) 
            return std::move(Result(nullptr));
        for (const auto& pair : it->second) {
            if (pair.first->isSame(args->arg1)) 
                return std::move(Result(pair.second));
        }
        return std::move(Result(nullptr));
    }

    if (operation == PUT && args->size == 3) {
        put(args->arg1, args->arg2);
        return std::move(Result(nullptr));
    }

    throw Unimplemented();
}