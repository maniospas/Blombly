// Integer.cpp
#include "Integer.h"
#include "common.h"
#include "Boolean.h"
#include "Float.h"
#include "Vector.h"
#include "BString.h"

// Constructor
Integer::Integer(int val) : value(val) {}

// Return the type ID
int Integer::getType() const {
    return INT;
}

// Convert to string representation
std::string Integer::toString() const {
    return std::to_string(value);
}

// Return the integer value
int Integer::getValue() const {
    return value;
}

// Create a shallow copy of this Integer
std::shared_ptr<Data> Integer::shallowCopy() const {
    return std::make_shared<Integer>(value);
}

// Implement the specified operation
std::shared_ptr<Data> Integer::implement(const OperationType operation, const BuiltinArgs& args) {
    // Single argument operations
    if (args.size == 1 && args.arg0->getType() == INT) {
        if (operation == TOCOPY || operation == TOINT) {
            return std::make_shared<Integer>(value);
        }
        if (operation == TOFLOAT) {
            return std::make_shared<Float>(value);
        }
        if (operation == TOSTR) {
            return std::make_shared<BString>(std::to_string(value));
        }
        if (operation == TOVECTOR) {
            return std::make_shared<Vector>(value, true);
        }
    }

    // Two-argument operations
    if (args.size == 2 && args.arg0->getType() == INT && args.arg1->getType() == INT) {
        int v1 = std::static_pointer_cast<Integer>(args.arg0)->getValue();
        int v2 = std::static_pointer_cast<Integer>(args.arg1)->getValue();
        int result = 0;

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
            return std::make_shared<Float>(v1 / static_cast<float>(v2));
        } else if (operation == MOD) {
            result = v1 % v2;
        } else if (operation == POW) {
            result = static_cast<int>(std::pow(v1, v2));
        } else {
            throw Unimplemented();
        }

        return std::make_shared<Integer>(result);
    }

    // Unimplemented operation
    throw Unimplemented();
}
