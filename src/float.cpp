// Float.cpp
#include "Float.h"
#include "common.h"
#include "Integer.h"
#include "Boolean.h"
#include "BString.h"

// Constructor
Float::Float(double val) : value(val) {}

// Return the type ID
int Float::getType() const {
    return FLOAT;
}

// Convert to string representation
std::string Float::toString() const {
    return std::to_string(value);
}

// Return the floating-point value
double Float::getValue() const {
    return value;
}

// Create a shallow copy of this Float
std::shared_ptr<Data> Float::shallowCopy() const {
    return std::make_shared<Float>(value);
}

// Implement the specified operation
std::shared_ptr<Data> Float::implement(const OperationType operation, const BuiltinArgs& args)  {
    // Single argument operations
    if (args.size == 1 && args.arg0->getType() == FLOAT) {
        if (operation == TOCOPY || operation == TOFLOAT) {
            return std::make_shared<Float>(value);
        }
        if (operation == TOINT) {
            return std::make_shared<Integer>(static_cast<int>(value));
        }
        if (operation == TOSTR) {
            return std::make_shared<BString>(std::to_string(value));
        }
    }

    // Two-argument operations
    if (args.size == 2 && (args.arg0->getType() == FLOAT || args.arg0->getType() == INT) && (args.arg1->getType() == FLOAT || args.arg1->getType() == INT)) {
        double v1 = args.arg0->getType() == INT ? static_cast<double>(std::static_pointer_cast<Integer>(args.arg0)->getValue()) : std::static_pointer_cast<Float>(args.arg0)->getValue();
        double v2 = args.arg1->getType() == INT ? static_cast<double>(std::static_pointer_cast<Integer>(args.arg1)->getValue()) : std::static_pointer_cast<Float>(args.arg1)->getValue();
        double result = 0;

        if (operation == EQ) {
            return std::make_shared<Boolean>(v1 == v2);
        }
        if (operation == NEQ) {
            return std::make_shared<Boolean>(v1 != v2);
        }
        if (operation == LT) {
            return std::make_shared<Boolean>(v1 < v2);
        }
        if (operation == LE) {
            return std::make_shared<Boolean>(v1 <= v2);
        }
        if (operation == GT) {
            return std::make_shared<Boolean>(v1 > v2);
        }
        if (operation == GE) {
            return std::make_shared<Boolean>(v1 >= v2);
        }
        if (operation == ADD) {
            result = v1 + v2;
        } else if (operation == SUB) {
            result = v1 - v2;
        } else if (operation == MUL) {
            result = v1 * v2;
        } else if (operation == DIV) {
            result = v1 / v2;
        } else if (operation == POW) {
            result = std::pow(v1, v2);
        } else {
            throw Unimplemented();
        }

        return std::make_shared<Float>(result);
    }

    // Unimplemented operation
    throw Unimplemented();
}
