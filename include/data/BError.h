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

#ifndef BERROR_H
#define BERROR_H

#include <string>
#include <memory>
#include "common.h"
#include "data/Data.h"

// BString class representing a string data type
class BError : public Data {
private:
    std::string value;
    bool consumed;

public:
    explicit BError(const std::string& val);
    BError* consume();
    bool isConsumed() const;
    std::string toString(BMemory* memory) override;

    Result push(BMemory* memory, const DataPtr& other) override {bbcascade1(this->value);}
    Result pop(BMemory* memory) override {bbcascade1(this->value);}
    Result next(BMemory* memory) override {bbcascade1(this->value);}
    Result at(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) override {bbcascade1(this->value);}
    void clear(BMemory* memory) override {bbcascade1(this->value);}
    int64_t len(BMemory* memory) override {throw BBError(this->value);}
    Result move(BMemory* memory) override {bbcascade1(this->value);}
    Result iter(BMemory* memory) override {throw BBError(this->value);}
    double toFloat(BMemory* memory) override {throw BBError(this->value);}
    bool toBool(BMemory* memory) override {throw BBError(this->value);}
    int64_t toInt(BMemory* memory) override {throw BBError(this->value);}
    Result add(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result sub(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result mul(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result div(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result pow(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result mmul(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result mod(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result lt(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result le(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result gt(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result ge(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result eq(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result neq(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result opand(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result opor(BMemory* memory, const DataPtr& other) override {throw BBError(this->value);}
    Result min(BMemory* memory) override {throw BBError(this->value);}
    Result max(BMemory* memory) override {throw BBError(this->value);}
    Result logarithm(BMemory* memory) override {throw BBError(this->value);}
    Result sum(BMemory* memory) override {throw BBError(this->value);}
    Result opnot(BMemory* memory) override {throw BBError(this->value);}
};

#endif // BERROR_H
