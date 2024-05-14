#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
public:
    int getType() const override;
    std::string toString() const override;
    virtual std::shared_ptr<BMemory> getMemory() const = 0;
    virtual void lock() const = 0;
    virtual void unlock() const = 0;
    Data* implement(const OperationType operation_, BuiltinArgs* args_) override;
    Data* shallowCopy() const override;
};

class LocalStruct : public Struct {
private:
    std::weak_ptr<BMemory> memory;
public:
    LocalStruct(const std::shared_ptr<BMemory>& mem);
    std::shared_ptr<BMemory> getMemory() const override;
    void lock() const override;
    void unlock() const override;
};

class GlobalStruct : public Struct {
private:
    std::shared_ptr<BMemory> memory;
public:
    GlobalStruct(const std::shared_ptr<BMemory>& mem);
    std::shared_ptr<BMemory> getMemory() const override;
    void lock() const override;
    void unlock() const override;
};

#endif // STRUCT_H
