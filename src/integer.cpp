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
    // Single argument operations
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

    // Two-argument operations
    if (args->size == 2 && args->arg0->getType() == INT && args->arg1->getType() == INT) {
        int v1 = ((Integer*)args->arg0)->getValue();
        int v2 = ((Integer*)args->arg1)->getValue();

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
                if(args->preallocResult && args->preallocResult->getType()==INT) {
                    ((Integer*)args->preallocResult.get())->value = v1+v2;
                    return args->preallocResult;
                }
                return std::make_shared<Integer>(v1+v2);
            case SUB: 
                if(args->preallocResult && args->preallocResult->getType()==INT) {
                    ((Integer*)args->preallocResult.get())->value = v1-v2;
                    return args->preallocResult;
                }
                return std::make_shared<Integer>(v1-v2);
            case MUL: 
                if(args->preallocResult && args->preallocResult->getType()==INT) {
                    ((Integer*)args->preallocResult.get())->value = v1*v2;
                    return args->preallocResult;
                }
                return std::make_shared<Integer>(v1*v2);
            case MOD: 
                if(args->preallocResult && args->preallocResult->getType()==INT) {
                    ((Integer*)args->preallocResult.get())->value = v1%v2;
                    return args->preallocResult;
                }
                return std::make_shared<Integer>(v1%v2);
            case POW: 
                if(args->preallocResult && args->preallocResult->getType()==INT) {
                    ((Integer*)args->preallocResult.get())->value = static_cast<int>(std::pow(v1, v2));
                    return args->preallocResult;
                }
                return std::make_shared<Integer>(static_cast<int>(std::pow(v1, v2)));
            case DIV:
                if(args->preallocResult && args->preallocResult->getType()==FLOAT) {
                    ((Integer*)args->preallocResult.get())->value = v1 / static_cast<float>(v2);
                    return args->preallocResult;
                }
                return std::make_shared<Float>(v1 / static_cast<float>(v2));
        }
    }
    // Unimplemented operation
    throw Unimplemented();
}
