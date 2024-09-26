#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    std::shared_ptr<BMemory> memory;
    mutable std::recursive_mutex memoryLock;

public:

    int getType() const override;
    std::string toString() const override;
    virtual std::shared_ptr<BMemory> getMemory() const = 0;
    std::shared_ptr<Data> implement(const OperationType operation_, BuiltinArgs* args_) override;
    virtual std::shared_ptr<Struct> modifyBeforeAttachingToMemory(std::shared_ptr<Struct> selfPtr, BMemory* owner) = 0; // the contract here is that while the method is running there will always be a valid owner class instance
};

class StrongStruct : public Struct {
private:
    std::shared_ptr<BMemory> memory;
public:
    explicit StrongStruct(const std::shared_ptr<BMemory>& mem);
    ~StrongStruct();
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<BMemory> getMemory() const override;
    std::shared_ptr<Struct> modifyBeforeAttachingToMemory(std::shared_ptr<Struct> selfPtr, BMemory* owner) override;
};

class WeakStruct : public Struct {
private:
    std::weak_ptr<BMemory> memory;
public:
    explicit WeakStruct(const std::shared_ptr<BMemory>& mem);
    explicit WeakStruct(const std::weak_ptr<BMemory>& mem);
    ~WeakStruct();
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<BMemory> getMemory() const override;
    std::shared_ptr<Struct> modifyBeforeAttachingToMemory(std::shared_ptr<Struct> selfPtr, BMemory* owner) override;
};

#endif // STRUCT_H
