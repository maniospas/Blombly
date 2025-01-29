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

#ifndef ITERATOR_H
#define ITERATOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"


class Iterator : public Data {
public:
    explicit Iterator();
    virtual ~Iterator() = default;
    std::string toString(BMemory* memory) override;
    virtual int64_t expectedSize() const {return 0;} // this is not a guarantee at all
    virtual bool isContiguous() const {return false;}
    virtual int64_t getStart() const {bberror("Internal error: thechosen iterator type does not implement `getStart`, which means that `isContiguous` was not checked first.");}
    virtual int64_t getEnd() const {bberror("Internal error: the chosen iterator type does not implement `getEnd`, which means that `isContiguous` was not checked first.");}
    virtual DataPtr fastNext() {return (Data*)nullptr;} // nullptr signifies to JIT that it needs to fallback to calling next();
    Result iter(BMemory* memory) override {return RESMOVE(Result(this));} // not virtual to not be overriden
};


class AccessIterator : public Iterator {
private:
    mutable std::recursive_mutex memoryLock;
    int64_t size;
    Data* object;
    int64_t pos;
public:
    explicit AccessIterator(DataPtr object_, int64_t size); // size should be equal to object_->len(memory) for the memory in which the iterator is being created
    ~AccessIterator();
    Result next(BMemory* memory) override;
};


class IntRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    int64_t first, last, step;
public:
    explicit IntRange(int64_t first, int64_t last, int64_t step);
    ~IntRange();
    Result next(BMemory* memory) override;
    DataPtr fastNext() override;
    
    int64_t expectedSize() const override {std::lock_guard<std::recursive_mutex> lock(memoryLock);return (last-first)/step;}
    bool isContiguous() const override {return step==1;}
    int64_t getStart() const override {std::lock_guard<std::recursive_mutex> lock(memoryLock);return first;}
    int64_t getEnd() const override {return last;}
};

class FloatRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    double first, last, step;
public:
    explicit FloatRange(double first, double last, double step);
    ~FloatRange();
    DataPtr fastNext() override;
    Result next(BMemory* memory) override;
};

#endif // ITERATOR_H
