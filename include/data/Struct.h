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
    mutable std::mutex memoryLock;

public:
    explicit Struct(const std::shared_ptr<BMemory>& mem);
    ~Struct();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<BMemory> getMemory() const;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation_, BuiltinArgs* args_) override;
};

#endif // STRUCT_H
