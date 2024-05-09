// Float.h
#ifndef FLOAT_H
#define FLOAT_H

#include <memory>
#include <string>
#include "Data.h"

// Float class representing a floating-point data type
class Float : public Data {
private:
    double value;

public:
    explicit Float(double val);

    int getType() const override;
    std::string toString() const override;
    double getValue() const;

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Vector;
};

#endif // FLOAT_H
