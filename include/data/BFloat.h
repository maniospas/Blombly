#ifndef BBB_FLOAT_H
#define BBB_FLOAT_H

#include <memory>
#include <string>
#include "data/Data.h"


class BFloat : public Data {
public:
    double value;
    explicit BFloat(double val);

    std::string toString(BMemory* memory)override;
    double getValue() const;
    void setValue(double val);

    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
    virtual bool isSame(DataPtr other) override;
    virtual size_t toHash() const override;

    friend class Integer;
    friend class Boolean;
    friend class Vector;
    friend class BString;
    friend class BList;
    friend class BHashMap;
};

#endif // BBB_FLOAT_H
