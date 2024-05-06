// Boolean.h
#ifndef BOOLEAN_H
#define BOOLEAN_H

#include <memory>
#include <string>
#include "Data.h"

// Boolean class representing a boolean data type
class Boolean : public Data {
private:
    bool value;

public:
    explicit Boolean(bool val);
    
    int getType() const override;
    std::string toString() const override;
    bool getValue() const;

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, const BuiltinArgs& args) override;
};

#endif // BOOLEAN_H
