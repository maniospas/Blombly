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

#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    tsl::hopscotch_map<int, DataPtr> data;
    Result simpleImplement(int implementationCode, BMemory* scopeMemory);
    Result simpleImplement(int implementationCode, BMemory* scopeMemory, const DataPtr& other);
    void releaseMemory();
public:
    mutable std::recursive_mutex memoryLock;
    explicit Struct();
    explicit Struct(int defaultSize);
    ~Struct();
    DataPtr get(int id) const;
    DataPtr getOrNull(int id) const;
    void set(int id, const DataPtr& other);
    void transferNoChecks(int id, const DataPtr& other);

    Result push(BMemory* scopeMemory, const DataPtr& other) override;
    Result pop(BMemory* scopeMemory) override;
    Result next(BMemory* scopeMemory) override;
    Result at(BMemory* scopeMemory, const DataPtr& other);
    Result put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value);
    void clear(BMemory* scopeMemory) override;
    int64_t len(BMemory* scopeMemory) override;
    Result move(BMemory* scopeMemory) override;
    Result iter(BMemory* scopeMemory) override;
    double toFloat(BMemory* scopeMemory) override;
    std::string toString(BMemory* calledMemory) override;
    bool toBool(BMemory* scopeMemory) override;
    int64_t toInt(BMemory* scopeMemory) override;
    Result add(BMemory* scopeMemory, const DataPtr& other) override;
    Result sub(BMemory* scopeMemory, const DataPtr& other) override;
    Result rsub(BMemory* scopeMemory, const DataPtr& other) override;
    Result mul(BMemory* scopeMemory, const DataPtr& other) override;
    Result div(BMemory* scopeMemory, const DataPtr& other) override;
    Result rdiv(BMemory* scopeMemory, const DataPtr& other) override;
    Result pow(BMemory* scopeMemory, const DataPtr& other) override;
    Result rpow(BMemory* scopeMemory, const DataPtr& other) override;
    Result mmul(BMemory* scopeMemory, const DataPtr& other) override;
    Result mod(BMemory* scopeMemory, const DataPtr& other) override;
    Result rmod(BMemory* scopeMemory, const DataPtr& other) override;
    Result lt(BMemory* scopeMemory, const DataPtr& other) override;
    Result le(BMemory* scopeMemory, const DataPtr& other) override;
    Result gt(BMemory* scopeMemory, const DataPtr& other) override;
    Result ge(BMemory* scopeMemory, const DataPtr& other) override;
    Result eq(BMemory* scopeMemory, const DataPtr& other) override;
    Result neq(BMemory* scopeMemory, const DataPtr& other) override;
    Result opand(BMemory* scopeMemory, const DataPtr& other) override ;
    Result opor(BMemory* scopeMemory, const DataPtr& other) override;
    Result min(BMemory* scopeMemory) override;
    Result max(BMemory* scopeMemory) override;
    Result logarithm(BMemory* scopeMemory) override;
    Result sum(BMemory* scopeMemory) override;
    Result opnot(BMemory* scopeMemory) override;
};

#endif // STRUCT_H
