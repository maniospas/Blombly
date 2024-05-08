// BString.h
#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "Data.h"

// BString class representing a string data type
class BString : public Data {
private:
    std::string value;

public:
    explicit BString(const std::string& val);

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, const BuiltinArgs* args) override;
};

#endif // BSTRING_H
