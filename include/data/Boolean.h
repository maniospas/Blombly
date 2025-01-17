#ifndef BB_BOOLEAN_H
#define BB_BOOLEAN_H

#include <memory>
#include <string>
#include "data/Data.h"

class Boolean : public Data {
private:
    explicit Boolean(bool val);
    ~Boolean();
public:
    bool value;
    static Boolean* valueTrue;
    static Boolean* valueFalse;
    
    std::string toString(BMemory* memory)override;
    bool getValue() const;
    void setValue(bool val);
    virtual void addOwner() override {}
    virtual void removeFromOwner() override {}

    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
    virtual bool isSame(DataPtr other) override;

    friend class Integer; 
    friend class BFloat; 
    friend class BString; 
    friend class BList;
    friend class Vector;
    friend class BError;
    friend class BHashMap;
};

#endif // BB_BOOLEAN_H
