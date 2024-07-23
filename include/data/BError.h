// data/BError.h
#ifndef BERROR_H
#define BERROR_H

#include <string>
#include <memory>
#include "data/Data.h"

// BString class representing a string data type
class BError : public Data {
private:
    std::string value;

public:
    explicit BError(const std::string& val);

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // BERROR_H
