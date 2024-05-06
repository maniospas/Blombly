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
std::shared_ptr<Data> BString::implement(const OperationType operation, const BuiltinArgs& args)  {
    if (args.size == 1 && args.arg0->getType() == STRING) {
        if (operation == TOCOPY) {
            return std::make_shared<BString>(value);
        }

        // Conversion to Integer
        if (operation == TOINT) {
            return std::make_shared<Integer>(std::atoi(value.c_str()));
        }

        // Conversion to Float
        if (operation == TOFLOAT) {
            return std::make_shared<Float>(std::atof(value.c_str()));
        }

        // Conversion to Boolean
        if (operation == TOBOOL) {
            return std::make_shared<Boolean>(value == "true");
        }
    }

    // Two-argument operations involving strings
    if (args.size == 2 && args.arg0->getType() == STRING && args.arg1->getType() == STRING) {
        std::string v1 = std::static_pointer_cast<BString>(args.arg0)->value;
        std::string v2 = std::static_pointer_cast<BString>(args.arg1)->value;

        if (operation == EQ) {
            return std::make_shared<Boolean>(v1 == v2);
        }
        if (operation == NEQ) {
            return std::make_shared<Boolean>(v1 != v2);
        }
        if (operation == ADD) {
            return std::make_shared<BString>(v1 + v2);
        }
    }

    // Unimplemented operation
    throw Unimplemented();
}
