/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
#include <deque>

extern BError* OUT_OF_RANGE;

MapIterator::MapIterator(BHashMap* map_): map(map_), elementIndex(-1) {
    std::lock_guard<std::recursive_mutex> lock(map->memoryLock);
    bucketIt = map->contents.begin();
}

Result MapIterator::next(BMemory* memory) {
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
    kvPair.first.existsAddOwner();
    kvPair.second.existsAddOwner();
    return Result(item);
}
Result MapIterator::iter(BMemory* memory) {return RESMOVE(Result(this));}


BHashMap::BHashMap() : Data(MAP) {}
BHashMap::~BHashMap() {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    for (auto& bucket : contents) for (auto& kvPair : bucket.second) {
        kvPair.first.existsRemoveFromOwner();
        kvPair.second.existsRemoveFromOwner();
    }
    contents.clear();
}

std::string BHashMap::toString(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result = "{";
    bool firstEntry = true;
    for (const auto& bucket : contents) for (const auto& kvPair : bucket.second) {
        if (!firstEntry) result += ", ";
        result += (kvPair.first.exists()?kvPair.first->toString(memory):kvPair.first.torepr()) + ": " + (kvPair.second.exists()?kvPair.second->toString(memory):kvPair.second.torepr());
        firstEntry = false;
    }
    result += "}";
    return result;
}

void BHashMap::fastUnsafePut(const DataPtr& from, const DataPtr& to) {
    size_t keyHash = from.islit()?from.unsafe_toint():from->toHash();
    auto& entryList = contents[keyHash];

    for (auto& kvPair : entryList) if (kvPair.first->isSame(from)) {
        DataPtr prevValue = kvPair.second;
        kvPair.second = to;            
        to.existsAddOwner();
        prevValue.existsRemoveFromOwner();
        return;
    }

    // If not found, we add a new key-value pair
    entryList.emplace_back(from, to);
    to.existsAddOwner();
    from.existsAddOwner();
}

Result BHashMap::put(BMemory* memory, const DataPtr& from, const DataPtr& to) {
    bbassert(from.islitorexists(), "Missing key");
    bbassert(to.islitorexists(), "Missing map value");
    if(from.existsAndTypeEquals(ERRORTYPE)) bberror(from->toString(nullptr));
    if(to.existsAndTypeEquals(ERRORTYPE)) bberror(from->toString(nullptr));

    size_t keyHash = from.islit()?from.unsafe_toint():from->toHash();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    auto& entryList = contents[keyHash];

    for (auto& kvPair : entryList) if (kvPair.first.isSame(from)) {
        DataPtr prevValue = kvPair.second;
        kvPair.second = to;            
        to.existsAddOwner();
        prevValue.existsRemoveFromOwner();
        return RESMOVE(Result(DataPtr::NULLP));
    }

    // If not found, we add a new key-value pair
    entryList.emplace_back(from, to);
    to.existsAddOwner();
    from.existsAddOwner();
    return RESMOVE(Result(DataPtr::NULLP));
}


Result BHashMap::at(BMemory* memory, const DataPtr& keyData) {
    if(keyData==DataPtr::NULLP) bberror("Cannot have a missing value as key");
    {
        size_t keyHash = keyData.islit()?keyData.unsafe_toint():keyData->toHash();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        auto it = contents.find(keyHash);
        if (it != contents.end()) for (const auto& kvPair : it->second) if (kvPair.first->isSame(keyData)) 
            return Result(kvPair.second);
    }
    if(keyData.islit()) return RESMOVE(Result(OUT_OF_RANGE));
    Result iterResult = keyData->iter(memory);
    DataPtr iterator = iterResult.get();

    if (iterator.existsAndTypeEquals(ITERATOR)) {
        Iterator* iter = static_cast<Iterator*>(iterator.get());
        std::lock_guard<std::recursive_mutex> lock(memoryLock);

        BList* resultList = new BList();
        bool allKeysFound = true;

        try {
            while (true) {
                Result nextKeyResult = iter->next(memory);
                DataPtr nextKey = nextKeyResult.get();
                if (nextKey==OUT_OF_RANGE) break;
                size_t keyHash = nextKey.islit()?nextKey.unsafe_toint():nextKey->toHash();
                auto it = contents.find(keyHash);
                bool keyFound = false;
                if (it != contents.end()) for (const auto& kvPair : it->second) if (kvPair.first->isSame(nextKey)) {
                    resultList->contents.push_back(kvPair.second);
                    kvPair.second.existsAddOwner();
                    keyFound = true;
                    break;
                }
                if (!keyFound) {
                    allKeysFound = false;
                    break;
                }
            }
        } catch (...) {
            for (DataPtr item : resultList->contents) if (item.exists()) item.existsRemoveFromOwner();
            delete resultList;
            throw;
        }
        if (!allKeysFound) {
            for (DataPtr item : resultList->contents)  if (item.exists()) item.existsRemoveFromOwner();
            delete resultList;
            return Result(OUT_OF_RANGE);
        }
        return Result(resultList);
    }
    return Result(OUT_OF_RANGE);
}
void BHashMap::clear(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    for (auto& bucket : contents) for (auto& kvPair : bucket.second) {
        kvPair.first.existsRemoveFromOwner();
        kvPair.second.existsRemoveFromOwner();
    }
    contents.clear();
}
Result BHashMap::move(BMemory* memory) {
    BHashMap* ret = new BHashMap();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    ret->contents = std::move(contents);
    contents.clear();
    return RESMOVE(Result(ret));
}
int64_t BHashMap::len(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    int64_t count = 0;
    for (const auto& bucket : contents) count += bucket.second.size();
    return count;
}
Result BHashMap::iter(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    return Result(new MapIterator(this));
}