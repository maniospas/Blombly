// Float.cpp
#include "BFloat.h"
#include "common.h"
#include "Integer.h"
#include "Boolean.h"
#include "BString.h"

// Constructor
BFloat::BFloat(double val) : value(val) {}

// Return the type ID
int BFloat::getType() const {
    return FLOAT;
}

// Convert to string representation
std::string BFloat::toString() const {
    return std::to_string(value);
}

// Return the floating-point value
double BFloat::getValue() const {
    return value;
}

// Create a shallow copy of this Float
Data* BFloat::shallowCopy() const {
    return new BFloat(value);
}

// Implement the specified operation
Data* BFloat::implement(const OperationType operation, BuiltinArgs* args)  {
    // Two-argument operations
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();
        if ((type0 == FLOAT || type0 == INT) && (type1 == FLOAT || type1 == INT)) {
            double v1 = type0 == INT ? ((Integer*)args->arg0)->getValue() : ((BFloat*)args->arg0)->getValue();
            double v2 = type1 == INT ? ((Integer*)args->arg1)->getValue() : ((BFloat*)args->arg1)->getValue();
            Data* preallocResult;
            switch(operation) {
                case EQ: BOOLEAN_RESULT(v1==v2);
                case NEQ: BOOLEAN_RESULT(v1!=v2);
                case LT: BOOLEAN_RESULT(v1<v2);
                case LE: BOOLEAN_RESULT(v1<=v2);
                case GT: BOOLEAN_RESULT(v1>v2);
                case GE: BOOLEAN_RESULT(v1>=v2);
                case ADD: FLOAT_RESULT(v1+v2);
                case SUB: FLOAT_RESULT(v1-v2);
                case MUL: FLOAT_RESULT(v1*v2);
                case POW: FLOAT_RESULT(std::pow(v1, v2));
                case DIV: FLOAT_RESULT(v1/v2);
            }
        }
    }
    
    // Single argument operations (implement last because they are heavier either way)
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOFLOAT: FLOAT_RESULT(value);;
            case TOINT: INT_RESULT(static_cast<int>(value));
            case TOSTR: STRING_RESULT(std::to_string(value));
        }
        throw Unimplemented();
    }

    // Unimplemented operation
    throw Unimplemented();
}
