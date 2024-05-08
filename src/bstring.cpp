// BString.cpp
#include "BString.h"
#include "Boolean.h"
#include "Integer.h"
#include "Float.h"
#include "common.h"

// Constructor
BString::BString(const std::string& val) : value(val) {}

// Return the type ID
int BString::getType() const {
    return STRING;
}

// Convert to string representation
std::string BString::toString() const {
    return value;
}

// Create a shallow copy of this BString
std::shared_ptr<Data> BString::shallowCopy() const {
    return std::make_shared<BString>(value);
}

// Implement the specified operation
std::shared_ptr<Data> BString::implement(const OperationType operation, const BuiltinArgs* args)  {
    if (args->size == 1) {
        switch(operation) {
            case TOCOPY: 
            case TOSTR: return std::make_shared<BString>(value);
            case TOINT: return std::make_shared<Integer>(std::atoi(value.c_str()));
            case TOFLOAT: return std::make_shared<Float>(std::atof(value.c_str()));
            case TOBOOL: return std::make_shared<Boolean>(value == "true");
        }
        throw Unimplemented();
    }

    // Two-argument operations involving strings
    if (args->size == 2 && args->arg0->getType() == STRING && args->arg1->getType() == STRING) {
        std::string v1 = ((BString*)args->arg0)->value;
        std::string v2 = ((BString*)args->arg1)->value;
        switch(operation) {
            case EQ: return std::make_shared<Boolean>(v1 == v2);
            case NEQ: return std::make_shared<Boolean>(v1 != v2);
            case ADD: return std::make_shared<BString>(v1 + v2);
        }
    }

    // Unimplemented operation
    throw Unimplemented();
}
