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
    // Two-argument operations
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();
        if ((type0 == FLOAT || type0 == INT) && (type1 == FLOAT || type1 == INT)) {
            double v1 = type0 == INT ? ((Integer*)args->arg0)->getValue() : ((Float*)args->arg0)->getValue();
            double v2 = type1 == INT ? ((Integer*)args->arg1)->getValue() : ((Float*)args->arg1)->getValue();
            Data* preallocResult;
            switch(operation) {
                case EQ: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1==v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 == v2);
                case NEQ: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1!=v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 != v2);
                case LT: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1<v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 < v2);
                case LE:
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1<=v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 <= v2);
                case GT: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1>v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 > v2);
                case GE: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==BOOL) {
                        ((Boolean*)preallocResult)->value = v1>=v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Boolean>(v1 >= v2);
                case ADD: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Float*)preallocResult)->value = v1+v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(v1+v2);
                case SUB: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Float*)preallocResult)->value = v1-v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(v1-v2);
                case MUL: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Float*)preallocResult)->value = v1*v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(v1*v2);
                case POW: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Float*)preallocResult)->value = std::pow(v1, v2);
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(std::pow(v1, v2));
                case DIV: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Float*)preallocResult)->value = v1/v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(v1 / v2);
            }
        }
    }
    
    // Single argument operations (implement last because they are heavier either way)
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOFLOAT: return std::make_shared<Float>(value);
            case TOINT: return std::make_shared<Integer>(static_cast<int>(value));
            case TOSTR: return std::make_shared<BString>(std::to_string(value));
        }
        throw Unimplemented();
    }

    // Unimplemented operation
    throw Unimplemented();
}
