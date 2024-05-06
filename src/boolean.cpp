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
    if (args.size == 1 && args.arg0->getType() == BOOL) {
        if (operation == TOCOPY || operation == TOBOOL) {
            return std::make_shared<Boolean>(value);
        } 
        if(operation==NOT) {
            // Negation operation
            return std::make_shared<Boolean>(!value);
        }
    }

    // Two Boolean argument operations
    if (args.size == 2 && args.arg0->getType() == BOOL && args.arg1->getType() == BOOL) {
        bool v1 = std::static_pointer_cast<Boolean>(args.arg0)->getValue();
        bool v2 = std::static_pointer_cast<Boolean>(args.arg1)->getValue();
        bool res = false;

        if (operation == AND) {
            res = v1 && v2;
        } else if (operation == OR) {
            res = v1 || v2;
        } else if (operation == EQ) {
            res = v1 == v2;
        } else if (operation == NEQ) {
            res = v1 != v2;
        } else {
            throw Unimplemented();
        }

        return std::make_shared<Boolean>(res);
    }

    // Unimplemented operation
    throw Unimplemented();
}
