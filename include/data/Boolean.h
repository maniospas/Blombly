#ifndef BOOLEAN_H
#define BOOLEAN_H

#include <memory>
#include <string>
#include "data/Data.h"

class Boolean : public Data {
private:
    bool value;

public:
    explicit Boolean(bool val);
    
    int getType() const override;
    std::string toString() const override;
    bool getValue() const;
    bool isTrue() const override;
    void setValue(bool val);

    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;

    friend class Integer; 
    friend class BFloat; 
    friend class BString; 
    friend class BList;
    friend class Vector;
    friend class BError;
    friend class BHashMap;
};

#endif // BOOLEAN_H
