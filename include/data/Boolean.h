#ifndef BB_BOOLEAN_H
#define BB_BOOLEAN_H

#include <memory>
#include <string>
#include "data/Data.h"

class Boolean : public Data {
private:
    bool value;

public:
    explicit Boolean(bool val);
    
    std::string toString() const override;
    bool getValue() const;
    bool isTrue() const override;
    void setValue(bool val);

    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) const override;

    friend class Integer; 
    friend class BFloat; 
    friend class BString; 
    friend class BList;
    friend class Vector;
    friend class BError;
    friend class BHashMap;
};

#endif // BB_BOOLEAN_H
