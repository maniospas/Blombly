#include "data/BFloat.h"
#include "common.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>
#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BString.h"
#include "data/Iterator.h"


// Helper function for formatted output similar to Python's float formatting
std::string __python_like_float_format(double number, const std::string& format) {
    std::ostringstream output;
    size_t precision_pos = format.find('.');
    size_t type_pos = format.find_last_of("fFeEgGdxXob");

    int precision = 6; // Default precision
    char type = 'f';   // Default to fixed-point notation

    if (precision_pos != std::string::npos && (type_pos == std::string::npos || precision_pos < type_pos)) 
        precision = std::stoi(format.substr(precision_pos + 1, type_pos - precision_pos - 1));

    if (type_pos != std::string::npos) 
        type = format[type_pos];

    bool is_integer_format = (type == 'd' || type == 'x' || type == 'X' || type == 'o' || type == 'b');
    if (is_integer_format) {
        int int_number = static_cast<int>(number);
        switch (type) {
            case 'd': output << int_number; break;
            case 'x': output << std::hex << int_number; break;
            case 'X': output << std::uppercase << std::hex << int_number; break;
            case 'o': output << std::oct << int_number; break;
            case 'b': output << std::bitset<sizeof(int) * 8>(int_number); break;
            default: throw std::invalid_argument("Unsupported integer format specifier.");
        }
    } 
    else {
        output << std::setprecision(precision);
        switch (type) {
            case 'f': case 'F': output << std::fixed << number; break;
            case 'e': output << std::scientific << number; break;
            case 'E': output << std::scientific << std::uppercase << number; break;
            case 'g': case 'G': output << std::defaultfloat << number; break;
            default: output << std::fixed << number;
        }
    }

    return output.str();
}

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

bool BFloat::isSame(DataPtr other) {
    if(other->getType()!=BB_FLOAT)
        return false;
    return static_cast<BFloat*>(other)->value==value;
}

size_t BFloat::toHash() const {
    return std::hash<double>{}(value); 
}

Result BFloat::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();

        double v1 = (type0 == BB_INT) ? static_cast<double>(static_cast<Integer*>(args->arg0)->getValue()) : static_cast<BFloat*>(args->arg0)->getValue();
        double v2 = (type1 == BB_INT) ? static_cast<double>(static_cast<Integer*>(args->arg1)->getValue()) : static_cast<BFloat*>(args->arg1)->getValue();

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
        if (operation == AT && type1 == STRING) 
            STRING_RESULT(__python_like_float_format(value, args->arg1->toString(memory)));
        
        throw Unimplemented();
    }

    // Single argument operations
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOBB_FLOAT: BB_FLOAT_RESULT(value);
            case TOBB_INT: BB_INT_RESULT(static_cast<int>(value));
            case TOSTR: STRING_RESULT(std::to_string(value));
            case TOBB_BOOL: BB_BOOLEAN_RESULT(value);
        }
        throw Unimplemented();
    }

    if (args->size == 3 && operation == TORANGE) {
        double v0 = args->arg0->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg0)->getValue()) : static_cast<BFloat*>(args->arg0)->getValue();
        double v1 = args->arg1->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg1)->getValue()) : static_cast<BFloat*>(args->arg1)->getValue();
        double v2 = args->arg2->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg2)->getValue()) : static_cast<BFloat*>(args->arg2)->getValue();
        return std::move(Result(new FloatRange(v0, v1, v2)));
    }

    if(operation == TORANGE) bberror("Ranges containing floats should have exactly three arguments `range(first, end, step)` where end is non-inclusive.");
    throw Unimplemented();
}
