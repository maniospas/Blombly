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
std::shared_ptr<Data> Integer::implement(const OperationType operation, BuiltinArgs* args) {
    // Two-argument operations
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();
        if (type0 == INT && type1 == INT) {
            int v1 = ((Integer*)args->arg0)->getValue();
            int v2 = ((Integer*)args->arg1)->getValue();

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
                    if(preallocResult && preallocResult->getType()==INT) {
                        ((Integer*)preallocResult)->value = v1+v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Integer>(v1+v2);
                case SUB: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==INT) {
                        ((Integer*)preallocResult)->value = v1-v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Integer>(v1-v2);
                case MUL: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==INT) {
                        ((Integer*)preallocResult)->value = v1*v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Integer>(v1*v2);
                case MOD: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==INT) {
                        ((Integer*)preallocResult)->value = v1%v2;
                        return args->preallocResult;
                    }
                    return std::make_shared<Integer>(v1%v2);
                case POW: 
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==INT) {
                        ((Integer*)preallocResult)->value = static_cast<int>(std::pow(v1, v2));
                        return args->preallocResult;
                    }
                    return std::make_shared<Integer>(static_cast<int>(std::pow(v1, v2)));
                case DIV:
                    preallocResult = args->preallocResult.get();
                    if(preallocResult && preallocResult->getType()==FLOAT) {
                        ((Integer*)preallocResult)->value = v1 / static_cast<float>(v2);
                        return args->preallocResult;
                    }
                    return std::make_shared<Float>(v1 / static_cast<float>(v2));
            }
        }
        else if ((type0 == FLOAT || type0 == INT) && (type1 == FLOAT || type1 == INT)) {
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


    // Single argument operations (implement afterwards after)
    if (args->size == 1) {
        switch(operation) {
            case TOCOPY:
            case TOINT: return std::make_shared<Integer>(value);
            case TOFLOAT: return std::make_shared<Float>(value);
            case TOSTR: return std::make_shared<BString>(std::to_string(value));
            case TOVECTOR: return std::make_shared<Vector>(value, true);
        }
        throw Unimplemented();
    }

    // Unimplemented operation
    throw Unimplemented();
}
