// Integer.h
#ifndef INTEGER_H
#define INTEGER_H

#include <memory>
#include <string>
#include <cmath> 
#include "Data.h"

// Integer class representing an integer data type
class Integer : public Data {
private:
    int value;

public:
    explicit Integer(int val);

    int getType() const override;
    std::string toString() const override;
    int getValue() const;

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Iterator;
    friend class Float;
    friend class Boolean;
};

#endif // INTEGER_H
