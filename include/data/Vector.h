#ifndef VECTOR_H
#define VECTOR_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"

class Vector : public Data {
private:
    double* data;
    uint64_t size;
    mutable std::recursive_mutex memoryLock;

public:
    explicit Vector(uint64_t size);
    explicit Vector(uint64_t size, bool setToZero);
    ~Vector();

    std::string toString(BMemory* memory)override;

    Result at(BMemory* memory, const DataPtr& other) override;
    Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) override;
    int64_t len(BMemory* memory) override;
    Result iter(BMemory* memory) override;
    Result add(BMemory* memory, const DataPtr& other) override;
    Result sub(BMemory* memory, const DataPtr& other) override;
    Result rsub(BMemory* memory, const DataPtr& other) override;
    Result mul(BMemory* memory, const DataPtr& other) override;
    Result div(BMemory* memory, const DataPtr& other) override;
    Result rdiv(BMemory* memory, const DataPtr& other) override;
    Result pow(BMemory* memory, const DataPtr& other) override;
    Result rpow(BMemory* memory, const DataPtr& other) override;
    //Result mmul(BMemory* memory, const DataPtr& other) override;
    Result lt(BMemory* memory, const DataPtr& other) override;
    Result le(BMemory* memory, const DataPtr& other) override;
    Result gt(BMemory* memory, const DataPtr& other) override;
    Result ge(BMemory* memory, const DataPtr& other) override;
    Result eq(BMemory* memory, const DataPtr& other) override;
    Result neq(BMemory* memory, const DataPtr& other) override;
    Result min(BMemory* memory) override;
    Result max(BMemory* memory) override;
    Result logarithm(BMemory* memory) override;
    Result sum(BMemory* memory) override;

    friend class BList;
};

#endif // VECTOR_H
