// data/BString.h
#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "data/Data.h"

// BString class representing a string data type
class BString : public Data {
private:
    std::string value;

public:
    explicit BString(const std::string& val);

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Boolean;
    friend class BFloat;
    friend class Integer;
    friend class Vector;
    friend class BList;
    friend class BList;
    friend class BFile;
    friend class BError;
};

#endif // BSTRING_H
