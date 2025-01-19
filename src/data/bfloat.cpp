#include "data/BFloat.h"
#include "common.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>
#include "data/Integer.h"
#include "data/BString.h"
#include "data/Iterator.h"



BFloat::BFloat(double val) : value(val), Data(BB_FLOAT) {}

std::string BFloat::toString(BMemory* memory){
    return std::to_string(value);
}

double BFloat::getValue() const {
    return value;
}

void BFloat::setValue(double val) {
    value = val;
}

bool BFloat::isSame(const DataPtr& other) {
    if(other->getType()!=BB_FLOAT)
        return false;
    return static_cast<BFloat*>(other.get())->value==value;
}

size_t BFloat::toHash() const {
    return std::hash<double>{}(value); 
}

Result BFloat::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();

        double v1 = (type0 == BB_INT) ? static_cast<double>(static_cast<Integer*>(args->arg0.get())->getValue()) : static_cast<BFloat*>(args->arg0.get())->getValue();
        double v2 = (type1 == BB_INT) ? static_cast<double>(static_cast<Integer*>(args->arg1.get())->getValue()) : static_cast<BFloat*>(args->arg1.get())->getValue();

        switch (operation) {
            case EQ: BB_BOOLEAN_RESULT(v1 == v2);
            case NEQ: BB_BOOLEAN_RESULT(v1 != v2);
            case LT: BB_BOOLEAN_RESULT(v1 < v2);
            case LE: BB_BOOLEAN_RESULT(v1 <= v2);
            case GT: BB_BOOLEAN_RESULT(v1 > v2);
            case GE: BB_BOOLEAN_RESULT(v1 >= v2);
            case ADD: BB_FLOAT_RESULT(v1 + v2);
            case SUB: BB_FLOAT_RESULT(v1 - v2);
            case MUL: BB_FLOAT_RESULT(v1 * v2);
            case POW: BB_FLOAT_RESULT(std::pow(v1, v2));
            case DIV: BB_FLOAT_RESULT(v1 / v2);
        }

        // Handle formatted string conversion
        if (operation == AT && type1 == STRING) {
            //STRING_RESULT(__python_like_float_format(value, args->arg1->toString(memory)));
        }
        
        throw Unimplemented();
    }

    // Single argument operations
    if (args->size == 1) {
        switch (operation) {
            case TOBB_FLOAT: BB_FLOAT_RESULT(value);
            case TOBB_INT: BB_INT_RESULT(static_cast<int>(value));
            case TOSTR: STRING_RESULT(std::to_string(value));
            case TOBB_BOOL: BB_BOOLEAN_RESULT(value);
        }
        throw Unimplemented();
    }

    if (args->size == 3 && operation == TORANGE) {
        double v0 = args->arg0->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg0.get())->getValue()) : static_cast<BFloat*>(args->arg0.get())->getValue();
        double v1 = args->arg1->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg1.get())->getValue()) : static_cast<BFloat*>(args->arg1.get())->getValue();
        double v2 = args->arg2->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg2.get())->getValue()) : static_cast<BFloat*>(args->arg2.get())->getValue();
        return std::move(Result(new FloatRange(v0, v1, v2)));
    }

    if(operation == TORANGE) bberror("Ranges containing floats should have exactly three arguments `range(first, end, step)` where end is non-inclusive.");
    throw Unimplemented();
}
