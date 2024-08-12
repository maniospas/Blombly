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
    virtual BMemory* getMemory() const = 0;
    virtual void lock() const = 0;
    virtual void unlock() const = 0;
    Data* implement(const OperationType operation_, BuiltinArgs* args_) override;
};

class GlobalStruct : public Struct {
private:
    BMemory* memory;
public:
    GlobalStruct(BMemory* mem);
    ~GlobalStruct();
    BMemory* getMemory() const override;
    void lock() const override;
    void unlock() const override;
    Data* shallowCopy() const override;
};

#endif // STRUCT_H
