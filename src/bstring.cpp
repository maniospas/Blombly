// BString.cpp
#include "BString.h"
#include "Boolean.h"
#include "Integer.h"
#include "BFloat.h"
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
Data* BString::shallowCopy() const {
    return new BString(value);
}

// Implement the specified operation
Data* BString::implement(const OperationType operation, BuiltinArgs* args)  {

    // Two-argument operations involving strings
    if (args->size == 2 && args->arg0->getType() == STRING && args->arg1->getType() == STRING) {
        std::string v1 = ((BString*)args->arg0)->value;
        std::string v2 = ((BString*)args->arg1)->value;
        switch(operation) {
            case EQ: BOOLEAN_RESULT(v1 == v2);
            case NEQ: BOOLEAN_RESULT(v1 != v2);
            case ADD: STRING_RESULT(v1 + v2);
        }
    }

    if (args->size == 1) {
        switch(operation) {
            case TOCOPY: 
            case TOSTR: STRING_RESULT(value);
            case TOINT: INT_RESULT(std::atoi(value.c_str()));
            case TOFLOAT: FLOAT_RESULT(std::atof(value.c_str()));
            case TOBOOL: BOOLEAN_RESULT(value == "true");
        }
        throw Unimplemented();
    }

    // Unimplemented operation
    throw Unimplemented();
}
