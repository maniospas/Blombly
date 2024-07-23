// data/Boolean.h
#ifndef BOOLEAN_H
#define BOOLEAN_H

#include <memory>
#include <string>
#include "data/Data.h"

// Boolean class representing a boolean data type
class Boolean : public Data {
private:
    bool value;

public:
    explicit Boolean(bool val);
    
    int getType() const override;
    std::string toString() const override;
    bool getValue() const;
    bool isTrue() const override;

    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Integer; 
    friend class BFloat; 
    friend class BString; 
    friend class BList;
    friend class Vector;
};

#endif // BOOLEAN_H
