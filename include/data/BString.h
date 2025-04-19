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

#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "data/Data.h"
#include <mutex>


class BString : public Data {
private:
    std::string contents;
    explicit BString();
    std::string& toString();
public:
    explicit BString(const std::string& val);
    bool isSame(const DataPtr& other) override;
    Result eq(BMemory *memory, const DataPtr& other) override;
    Result neq(BMemory *memory, const DataPtr& other) override;
    Result at(BMemory *memory, const DataPtr& other) override;
    size_t toHash() const override;
    std::string toString(BMemory* memory) override;
    int64_t toInt(BMemory *memory) override;
    int64_t len(BMemory *memory) override;
    double toFloat(BMemory *memory) override;
    bool toBool(BMemory *memory) override;
    Result iter(BMemory *memory) override;
    Result add(BMemory *memory, const DataPtr& other) override;
    Result lt(BMemory* memory, const DataPtr& other) override;
    Result le(BMemory* memory, const DataPtr& other) override;
    Result gt(BMemory* memory, const DataPtr& other) override;
    Result ge(BMemory* memory, const DataPtr& other) override;
};

#endif // BSTRING_H
