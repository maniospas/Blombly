// BString.cpp
#include "data/BError.h"
#include "data/BString.h"
#include "common.h"

// Constructor
BError::BError(const std::string& val) : value(val) {}

// Return the type ID
int BError::getType() const {
    return ERRORTYPE;
}

// Convert to string representation
std::string BError::toString() const {
    return value;
}

// Create a shallow copy of this BString
Data* BError::shallowCopy() const {
    return new BError(value);
}

// Implement the specified operation
Data* BError::implement(const OperationType operation, BuiltinArgs* args)  {
    if (args->size == 1) {
        switch(operation) {
            //case TOCOPY: return shallowCopy();
            case TOSTR: STRING_RESULT(value);
        }
        throw Unimplemented();
    }
    // Unimplemented operation
    throw Unimplemented();
}
