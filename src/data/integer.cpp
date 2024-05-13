// Integer.cpp
#include "data/Integer.h"
#include "common.h"
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"

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
Data* Integer::shallowCopy() const {
    return new Integer(value);
}

// Implement the specified operation
Data* Integer::implement(const OperationType operation, BuiltinArgs* args) {
    // Twod-argument operations
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();
        if (type0 == INT && type1 == INT) {
            int v1 = ((Integer*)args->arg0)->getValue();
            int v2 = ((Integer*)args->arg1)->getValue();

            switch(operation) {
                case EQ: BOOLEAN_RESULT(v1==v2);
                case NEQ: BOOLEAN_RESULT(v1!=v2);
                case LT: BOOLEAN_RESULT(v1<v2);
                case LE: BOOLEAN_RESULT(v1<=v2);
                case GT: BOOLEAN_RESULT(v1>v2);
                case GE: BOOLEAN_RESULT(v1>=v2);
                case ADD: INT_RESULT(v1+v2);
                case SUB: INT_RESULT(v1-v2);
                case MUL: INT_RESULT(v1*v2);
                case MOD: INT_RESULT(v1%v2);
                case POW: INT_RESULT(static_cast<int>(std::pow(v1, v2)));
                case DIV: INT_RESULT(v1/static_cast<float>(v2));
            }
        }
        else if ((type0 == FLOAT || type0 == INT) && (type1 == FLOAT || type1 == INT)) {
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


    // Single argument operations (implement afterwards after)
    if (args->size == 1) {
        switch(operation) {
            case TOCOPY:
            case TOINT: INT_RESULT(value);
            case TOFLOAT: FLOAT_RESULT(value);
            case TOSTR: STRING_RESULT(std::to_string(value));
            case TOVECTOR: return new Vector(value, true);
        }
        throw Unimplemented();
    }

    // Unimplemented operation
    throw Unimplemented();
}
