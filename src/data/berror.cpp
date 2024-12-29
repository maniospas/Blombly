// BString.cpp
#include "data/BError.h"
#include "data/BString.h"
#include "data/Boolean.h"
#include "common.h"

// Constructor
BError::BError(const std::string& val) : value(val), consumed(false), Data(ERRORTYPE) {}

void BError::consume() {
    consumed = true;
}

bool BError::isConsumed() const {
    return consumed; 
}

std::string BError::toString(BMemory* memory){
    return value;
}

// Implement the specified operation
Result BError::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory)  {
    if (args->size == 1) {
        switch(operation) {
            case TOSTR: consumed=true;STRING_RESULT(value);
            case TOBB_BOOL: consumed=true;BB_BOOLEAN_RESULT(true);
        }
        throw Unimplemented();
    }
    // Unimplemented operation
    throw Unimplemented();
}
