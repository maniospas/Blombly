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
std::shared_ptr<Data> Boolean::shallowCopy() const {
    return std::make_shared<Boolean>(value);
}

// Implement the specified operation
std::shared_ptr<Data> Boolean::implement(const OperationType operation, const BuiltinArgs& args) {
    // Single Boolean argument operations
    if (args.size == 1) {
        if (operation == TOCOPY || operation == TOBOOL) {
            return std::make_shared<Boolean>(value);
        } 
        if(operation==NOT) {
            return std::make_shared<Boolean>(!value);
        }
        throw Unimplemented();
    }

    // Two Boolean argument operations
    if (args.size == 2 && args.arg0->getType() == BOOL && args.arg1->getType() == BOOL) {
        bool v1 = ((Boolean*)args.arg0.get())->getValue();
        bool v2 = ((Boolean*)args.arg1.get())->getValue();
        switch(operation) {
            case AND: return std::make_shared<Boolean>(v1 && v2);
            case OR: return std::make_shared<Boolean>(v1 || v2);
            case EQ: return std::make_shared<Boolean>(v1 == v2);
            case NEQ: return std::make_shared<Boolean>(v1 != v2);
        }
    }

    // Unimplemented operation
    throw Unimplemented();
}
