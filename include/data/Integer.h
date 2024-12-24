#ifndef BB_INTEGER_H
#define BB_INTEGER_H

#include <memory>
#include <string>
#include <cmath>
#include "data/Data.h"

class Integer : public Data {
private:
    int value;

public:
    explicit Integer(int val);

    std::string toString()override;
    int getValue() const;
    void setValue(int val);

    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) override;
    virtual size_t toHash() const override;

    friend class AccessIterator;
    friend class BFloat;
    friend class Boolean;
    friend class BString;
    friend class Vector;
    friend class BList;
    friend class BFile;
    friend class BHashMap;
};

#endif // BB_INTEGER_H
