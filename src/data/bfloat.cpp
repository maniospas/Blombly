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

BFloat::BFloat(double val) : value(val) {}

int BFloat::getType() const {
    return FLOAT;
}

std::string BFloat::toString() const {
    return std::to_string(value);
}

double BFloat::getValue() const {
    return value;
}

std::shared_ptr<Data> BFloat::shallowCopy() const {
    return std::make_shared<BFloat>(value);
}

std::shared_ptr<Data> BFloat::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();

        double v1 = (type0 == INT) ? static_cast<double>(static_cast<Integer*>(args->arg0.get())->getValue()) 
                                   : static_cast<BFloat*>(args->arg0.get())->getValue();
        double v2 = (type1 == INT) ? static_cast<double>(static_cast<Integer*>(args->arg1.get())->getValue()) 
                                   : static_cast<BFloat*>(args->arg1.get())->getValue();

        switch (operation) {
            case EQ: BOOLEAN_RESULT(v1 == v2);
            case NEQ: BOOLEAN_RESULT(v1 != v2);
            case LT: BOOLEAN_RESULT(v1 < v2);
            case LE: BOOLEAN_RESULT(v1 <= v2);
            case GT: BOOLEAN_RESULT(v1 > v2);
            case GE: BOOLEAN_RESULT(v1 >= v2);
            case ADD: FLOAT_RESULT(v1 + v2);
            case SUB: FLOAT_RESULT(v1 - v2);
            case MUL: FLOAT_RESULT(v1 * v2);
            case POW: FLOAT_RESULT(std::pow(v1, v2));
            case DIV: FLOAT_RESULT(v1 / v2);
        }

        // Handle formatted string conversion
        if (operation == AT && type1 == STRING) {
            STRING_RESULT(__python_like_float_format(value, args->arg1->toString()));
        }
        
        throw Unimplemented();
    }

    // Single argument operations
    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOFLOAT: FLOAT_RESULT(value);
            case TOINT: INT_RESULT(static_cast<int>(value));
            case TOSTR: STRING_RESULT(std::to_string(value));
        }
        throw Unimplemented();
    }

    throw Unimplemented();
}
