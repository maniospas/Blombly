// Boolean.cpp
#include "Boolean.h"
#include "common.h"


// Constructor
Boolean::Boolean(bool val) : value(val) {}

// Return the type ID
int Boolean::getType() const {
    return BOOL;
}

// Convert to string representation
std::string Boolean::toString() const {
    return value ? "true" : "false";
}

// Return the boolean value
bool Boolean::getValue() const {
    return value;
}

// Create a shallow copy of this Boolean
Data* Boolean::shallowCopy() const {
    return new Boolean(value);
}

// Implement the specified operation
Data* Boolean::implement(const OperationType operation, BuiltinArgs* args) {
    // Two Boolean argument operations
    if (args->size == 2 && args->arg0->getType() == BOOL && args->arg1->getType() == BOOL) {
        bool v1 = ((Boolean*)args->arg0)->getValue();
        bool v2 = ((Boolean*)args->arg1)->getValue();
        switch(operation) {
            case AND: BOOLEAN_RESULT(v1 && v2);
            case OR: BOOLEAN_RESULT(v1 || v2);
            case EQ: BOOLEAN_RESULT(v1 == v2);
            case NEQ: BOOLEAN_RESULT(v1 != v2);
        }
    }

    // Single Boolean argument operations
    if (args->size == 1) {
        switch(operation) {
            case TOCOPY:
            case TOBOOL: BOOLEAN_RESULT(value);
            case NOT: BOOLEAN_RESULT(!value);
        }
        throw Unimplemented();
    }


    // Unimplemented operation
    throw Unimplemented();
}
