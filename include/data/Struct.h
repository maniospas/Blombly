#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    BMemory* memory;
    mutable std::recursive_mutex memoryLock;

public:
    explicit Struct(BMemory* mem);
    ~Struct();
    int getType() const override;
    std::string toString() const override;
    virtual BMemory* getMemory() const;
    Data* implement(const OperationType operation_, BuiltinArgs* args_) override;    
};

#endif // STRUCT_H
