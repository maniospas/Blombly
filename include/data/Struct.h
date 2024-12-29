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

public:
    explicit Struct(BMemory* mem);
    ~Struct();
    std::string toString(BMemory* memory)override;
    virtual BMemory* getMemory() const;
    Result implement(const OperationType operation_, BuiltinArgs* args_, BMemory* memory) override;    
    void removeFromOwner() override;
};

#endif // STRUCT_H
