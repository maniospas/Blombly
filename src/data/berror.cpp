// BString.cpp
#include "data/BError.h"
#include "data/BString.h"
#include "data/Boolean.h"
#include "common.h"

// Constructor
BError::BError(const std::string& val) : value(val), consumed(false) {}

void BError::consume() {
    consumed = true;
}

bool BError::isConsumed() const {
    return consumed;
}

// Return the type ID
int BError::getType() const {
    return ERRORTYPE;
}

// Convert to string representation
std::string BError::toString() const {
    return value;
}

// Create a shallow copy of this BString
std::shared_ptr<Data> BError::shallowCopy() const {
    return std::make_shared<BError>(value);
    //bberror("Cannot copy error messages.");
}

// Implement the specified operation
std::shared_ptr<Data> BError::implement(const OperationType operation, BuiltinArgs* args)  {
    if (args->size == 1) {
        switch(operation) {
            case TOCOPY: return shallowCopy();
            case TOSTR: consumed=true;STRING_RESULT(value);
            case TOBOOL: consumed=true;BOOLEAN_RESULT(true);
        }
        throw Unimplemented();
    }
    // Unimplemented operation
    throw Unimplemented();
}
