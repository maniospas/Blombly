// data/BError.h
#ifndef BERROR_H
#define BERROR_H

#include <string>
#include <memory>
#include "common.h"
#include "data/Data.h"

// BString class representing a string data type
class BError : public Data {
private:
    std::string value;
    bool consumed;

public:
    explicit BError(const std::string& val);
    void consume();
    bool isConsumed() const;

    int getType() const override;
    std::string toString() const override;
    virtual std::shared_ptr<Data> shallowCopy() const;
    virtual std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args);
};

#endif // BERROR_H
