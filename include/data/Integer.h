#ifndef INTEGER_H
#define INTEGER_H

#include <memory>
#include <string>
#include <cmath>
#include "data/Data.h"


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

    size_t toHash() const override {
        return std::hash<int>{}(value);
    }

    friend class Iterator;
    friend class BFloat;
    friend class Boolean;
    friend class BString;
    friend class Vector;
    friend class BList;
    friend class BFile;
    friend class BHashMap;
};

#endif // INTEGER_H
