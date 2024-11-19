#ifndef BSTRING_H
#define BSTRING_H

#include <string>
#include <memory>
#include "data/Data.h"

class BString : public Data {
private:
    std::string value;

public:
    explicit BString(const std::string& val);

    std::string toString() const override;
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) const override;
    virtual size_t toHash() const override;

    friend class Boolean;
    friend class BFloat;
    friend class Integer;
    friend class Vector;
    friend class BList;
    friend class BFile;
    friend class BError;
    friend class BHashMap;
};

#endif // BSTRING_H
