#ifndef BFLOAT_H
#define BFLOAT_H

#include <memory>
#include <string>
#include "data/Data.h"


class BFloat : public Data {
private:
    double value;

public:
    explicit BFloat(double val);

    int getType() const override;
    std::string toString() const override;
    double getValue() const;

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    size_t toHash() const override {
        return std::hash<double>{}(value); 
    }

    friend class Integer;
    friend class Boolean;
    friend class Vector;
    friend class BString;
    friend class BList;
    friend class BHashMap;
};

#endif // BFLOAT_H
