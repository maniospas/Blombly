#ifndef BBB_FLOAT_H
#define BBB_FLOAT_H

#include <memory>
#include <string>
#include "data/Data.h"


class BFloat : public Data {
private:
    double value;

public:
    explicit BFloat(double val);

    std::string toString() const override;
    double getValue() const;

    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) const override;
    virtual size_t toHash() const override;

    friend class Integer;
    friend class Boolean;
    friend class Vector;
    friend class BString;
    friend class BList;
    friend class BHashMap;
};

#endif // BBB_FLOAT_H
