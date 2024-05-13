#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    std::shared_ptr<BMemory> memory;
public:
    Struct(const std::shared_ptr<BMemory>& mem);
    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<BMemory> getMemory();
    void lock();
    void unlock();
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation_, BuiltinArgs* args_) override;
};

#endif // STRUCT_H
