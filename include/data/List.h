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

#ifndef LIST_H
#define LIST_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"

class Graphics;
class Vector;
class BHashMap;

class BList : public Data {
private:
    mutable std::recursive_mutex memoryLock; 
    int64_t front;
    void resizeContents();
public:
    std::vector<DataPtr> contents;
    
    explicit BList();
    explicit BList(int64_t reserve);
    ~BList();

    std::string toString(BMemory* memory)override;
    DataPtr at(int64_t index) const;
    Vector* toVector(BMemory* memory) const;
    BHashMap* toMap() const;

    Result add(BMemory* memory, const DataPtr& other) override;
    Result push(BMemory* memory, const DataPtr& other) override;
    Result pop(BMemory* memory) override;
    Result next(BMemory* memory) override;
    Result at(BMemory* memory, const DataPtr& other) override;
    Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) override;
    void clear(BMemory* memory) override;
    int64_t len(BMemory* memory) override;
    Result move(BMemory* memory) override;
    Result iter(BMemory* memory) override;
    Result min(BMemory* memory) override;
    Result max(BMemory* memory) override;

    friend class Graphics;
};

#endif // LIST_H
