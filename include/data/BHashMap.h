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

#ifndef BHASHMAP_H
#define BHASHMAP_H

#include "Data.h"
#include "Iterator.h"
#include <memory>
#include <mutex>
#include "tsl/hopscotch_map.h"

class BHashMap : public Data {
public:
    BHashMap();
    virtual ~BHashMap();
    std::string toString(BMemory* memory) override;
    Result put(BMemory* memory, const DataPtr& from, const DataPtr& to) override;
    Result at(BMemory* memory, const DataPtr& other) override;
    void clear(BMemory* memory) override;
    int64_t len(BMemory* memory) override;
    Result move(BMemory* memory) override;
    Result iter(BMemory* memory) override;
    void fastUnsafePut(const DataPtr& from, const DataPtr& to);
    //Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;

private:
    mutable std::recursive_mutex memoryLock;
    std::unordered_map<size_t, std::vector<std::pair<DataPtr, DataPtr>>> contents;
    friend class MapIterator; 
};


class MapIterator : public Iterator {
private:
    BHashMap* map;
    std::unordered_map<size_t, std::vector<std::pair<DataPtr, DataPtr>>>::iterator bucketIt;
    int64_t elementIndex;

public:
    MapIterator(BHashMap* map_);
    ~MapIterator() override = default;
    Result next(BMemory* memory) override;
    Result iter(BMemory* memory) override;
};

#endif // BHASHMAP_H
