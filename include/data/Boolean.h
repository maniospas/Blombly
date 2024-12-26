#ifndef BB_BOOLEAN_H
#define BB_BOOLEAN_H

#include <memory>
#include <string>
#include "data/Data.h"

class Boolean : public Data {
private:
    bool value;
    explicit Boolean(bool val);
public:
    static Boolean* valueTrue;
    static Boolean* valueFalse;
    
    std::string toString()override;
    bool getValue() const;
    void setValue(bool val);
    virtual void addOwner() const{}
    virtual void removeFromOwner() const{}

    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    virtual bool isSame(Data* other) override;

    friend class Integer; 
    friend class BFloat; 
    friend class BString; 
    friend class BList;
    friend class Vector;
    friend class BError;
    friend class BHashMap;
};

#endif // BB_BOOLEAN_H
