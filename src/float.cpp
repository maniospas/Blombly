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
std::shared_ptr<Data> Float::implement(const OperationType operation, BuiltinArgs* args)  {
    // Single argument operations
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOFLOAT: return std::make_shared<Float>(value);
            case TOINT: return std::make_shared<Integer>(static_cast<int>(value));
            case TOSTR: return std::make_shared<BString>(std::to_string(value));
        }
        throw Unimplemented();
    }

    // Two-argument operations
    if (args->size == 2 
        && (args->arg0->getType() == FLOAT || args->arg0->getType() == INT) 
        && (args->arg1->getType() == FLOAT || args->arg1->getType() == INT)) {
        double v1 = args->arg0->getType() == INT ? static_cast<double>(((Integer*)args->arg0)->getValue()) : ((Float*)args->arg0)->getValue();
        double v2 = args->arg1->getType() == INT ? static_cast<double>(((Integer*)args->arg1)->getValue()) : ((Float*)args->arg1)->getValue();
        switch(operation) {
            case EQ: 
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1==v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 == v2);
            case NEQ: 
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1!=v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 != v2);
            case LT: 
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1<v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 < v2);
            case LE:
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1<=v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 <= v2);
            case GT: 
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1>v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 > v2);
            case GE: 
                if(args->preallocResult && args->preallocResult->getType()==BOOL) {
                    ((Boolean*)args->preallocResult.get())->value = v1>=v2;
                    return args->preallocResult;
                }
                return std::make_shared<Boolean>(v1 >= v2);
            case ADD: 
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Float*)args->preallocResult.get())->value = v1+v2;
                    return args->preallocResult;
                }
                return std::make_shared<Float>(v1+v2);
            case SUB: 
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Float*)args->preallocResult.get())->value = v1-v2;
                    return args->preallocResult;
                }
                return std::make_shared<Float>(v1-v2);
            case MUL: 
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Float*)args->preallocResult.get())->value = v1*v2;
                    return args->preallocResult;
                }
                return std::make_shared<Float>(v1*v2);
            case POW: 
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Float*)args->preallocResult.get())->value = std::pow(v1, v2);
                    return args->preallocResult;
                }
                return std::make_shared<Float>(std::pow(v1, v2));
            case DIV: 
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Float*)args->preallocResult.get())->value = v1/v2;
                    return args->preallocResult;
                }
                return std::make_shared<Float>(v1 / v2);
        }
    }

    // Unimplemented operation
    throw Unimplemented();
}
