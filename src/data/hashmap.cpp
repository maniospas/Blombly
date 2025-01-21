#include "data/BHashMap.h"
#include "data/BString.h"
#include "data/List.h"
#include "data/Iterator.h"
#include "data/BError.h"
#include "common.h"

#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

extern BError* OUT_OF_RANGE;

class MapIterator : public Iterator {
private:
    BHashMap* map;
    std::unordered_map<size_t, std::vector<std::pair<DataPtr, DataPtr>>>::iterator bucketIt;
    int64_t elementIndex;

public:
    MapIterator(BHashMap* map_) : map(map_), elementIndex(-1) {
        std::lock_guard<std::recursive_mutex> lock(map->memoryLock);
        bucketIt = map->contents.begin();
    }

    ~MapIterator() override = default;

    Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override {
        if (operation == NEXT && args->size == 1) {
            std::lock_guard<std::recursive_mutex> lock(map->memoryLock);
            if (bucketIt == map->contents.end()) return Result(OUT_OF_RANGE);
            elementIndex++;
            while (bucketIt != map->contents.end() && elementIndex >= static_cast<int64_t>(bucketIt->second.size())) {
                ++bucketIt;
                elementIndex = 0;
            }
            if (bucketIt == map->contents.end()) return Result(OUT_OF_RANGE);
            auto& kvPair = bucketIt->second[elementIndex];

            BList* item = new BList(2);
            item->contents.push_back(kvPair.first);
            item->contents.push_back(kvPair.second);
            kvPair.first->addOwner();
            kvPair.second->addOwner();
            return Result(item);
        }

        if (operation == TOITER && args->size == 1) return Result(this);
        throw Unimplemented();
    }
};


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

void BHashMap::put(DataPtr from, DataPtr to) {
    bbassert(from.exists(), "Missing key");
    bbassert(to.exists(), "Missing map value");
    bbassert(from->getType()!=ERRORTYPE, "Cannot have an error as a map key");
    bbassert(to->getType()!=ERRORTYPE, "Cannot have an error as a map value");

    size_t keyHash = from.islit()?from.unsafe_toint():from->toHash();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    auto& entryList = contents[keyHash];

    for (auto& kvPair : entryList) {
        if (kvPair.first->isSame(from)) {
            DataPtr prevValue = kvPair.second;
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
    if (args->size == 1) {
        switch (operation) {
            case LEN: {
                std::lock_guard<std::recursive_mutex> lock(memoryLock);
                int64_t count = 0;
                for (const auto& bucket : contents) count += bucket.second.size();
                return Result(count);
            }
            case TOITER: {
                std::lock_guard<std::recursive_mutex> lock(memoryLock);
                return Result(new MapIterator(this));
            }
            default: throw Unimplemented();
        }
    }

    if (operation == AT && args->size == 2) {
        DataPtr keyData = args->arg1;

        // Handle regular single-key access
        {
            std::lock_guard<std::recursive_mutex> lock(memoryLock);
            size_t keyHash = keyData.islit()?keyData.unsafe_toint():keyData->toHash();
            auto it = contents.find(keyHash);
            if (it != contents.end()) 
                for (const auto& kvPair : it->second) 
                    if (kvPair.first->isSame(keyData)) 
                        return Result(kvPair.second);
        }

        // Attempt to convert to an iterator if the single key is not found
        BuiltinArgs implArgs;
        implArgs.size = 1;
        implArgs.arg0 = keyData;
        Result iterResult = keyData->implement(TOITER, &implArgs, memory);
        DataPtr iterator = iterResult.get();

        if (iterator.existsAndTypeEquals(ITERATOR)) {
            Iterator* iter = static_cast<Iterator*>(iterator.get());
            std::lock_guard<std::recursive_mutex> lock(memoryLock);

            BList* resultList = new BList();
            bool allKeysFound = true;

            try {
                while (true) {
                    // Retrieve the next key from the iterator
                    Result nextKeyResult = iter->implement(NEXT, &implArgs, memory);
                    DataPtr nextKey = nextKeyResult.get();

                    if (!nextKey.islitorexists()) break; // Iterator exhausted

                    // Find the value corresponding to the key
                    size_t keyHash = nextKey.islit()?nextKey.unsafe_toint():nextKey->toHash();
                    auto it = contents.find(keyHash);
                    bool keyFound = false;

                    if (it != contents.end()) for (const auto& kvPair : it->second) if (kvPair.first->isSame(nextKey)) {
                        resultList->contents.push_back(kvPair.second);
                        kvPair.second->addOwner();
                        keyFound = true;
                        break;
                    }

                    if (!keyFound) {
                        allKeysFound = false;
                        break;
                    }
                }
            } catch (...) {
                for (DataPtr item : resultList->contents) if (item.exists()) item->removeFromOwner();
                delete resultList;
                throw;
            }

            if (!allKeysFound) {
                for (DataPtr item : resultList->contents)  if (item.exists()) item->removeFromOwner();
                delete resultList;
                return Result(OUT_OF_RANGE);
            }

            return Result(resultList);
        }

        return Result(OUT_OF_RANGE);
    }

    if (operation == PUT && args->size == 3) {
        DataPtr keyData   = args->arg1;
        DataPtr valueData = args->arg2;
        put(keyData, valueData);
        return Result(nullptr);
    }
    
    if(operation==MOVE && args->size==1) {
        BHashMap* ret = new BHashMap();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        ret->contents = std::move(contents);
        contents.clear();
        return std::move(Result(ret));
    }

    throw Unimplemented();
}
