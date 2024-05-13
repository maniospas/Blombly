// data/Integer.h
#ifndef INTEGER_H
#define INTEGER_H

#include <memory>
#include <string>
#include <cmath> 
#include "data/Data.h"

// Integer class representing an integer data type
class Integer : public Data {
private:
    int value;

public:
    explicit Integer(int val);

    int getType() const override;
    std::string toString() const override;
    int getValue() const;

    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Iterator;
    friend class BFloat;
    friend class Boolean;
    friend class BString;
    friend class Vector;
    friend class BList;
};

#endif // INTEGER_H
