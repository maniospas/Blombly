#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"
#include "common.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>


std::string __python_like_int_format(int int_number, const std::string& format) {
    std::ostringstream output;
    size_t precision_pos = format.find('.');
    size_t type_pos = format.find_last_of("fFeEgGdxXob");

    int precision = 6;
    char type = 'f';

    if (precision_pos != std::string::npos && (type_pos == std::string::npos || precision_pos < type_pos)) 
        precision = std::stoi(format.substr(precision_pos + 1, type_pos - precision_pos - 1));

    if (type_pos != std::string::npos) 
        type = format[type_pos];

    bool is_integer_format = (type == 'd' || type == 'x' || type == 'X' || type == 'o' || type == 'b');

    if (is_integer_format) {
        switch (type) {
            case 'd': output << int_number; break;
            case 'x': output << std::hex << int_number; break;
            case 'X': output << std::uppercase << std::hex << int_number; break;
            case 'o': output << std::oct << int_number; break;
            case 'b': output << std::bitset<sizeof(int) * 8>(int_number); break;
            default: throw std::invalid_argument("Unsupported integer format specifier.");
        }
    } else {
        double number = static_cast<double>(int_number);
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

Integer::Integer(int val) : value(val) {}

int Integer::getType() const {
    return BB_INT;
}

std::string Integer::toString() const {
    return std::to_string(value);
}

int Integer::getValue() const {
    return value;
}

std::shared_ptr<Data> Integer::shallowCopy() const {
    return std::make_shared<Integer>(value);
}

std::shared_ptr<Data> Integer::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 2) {
        int type0 = args->arg0->getType();
        int type1 = args->arg1->getType();

        if (type0 == BB_INT && type1 == BB_INT) {
            int v1 = static_cast<Integer*>(args->arg0.get())->getValue();
            int v2 = static_cast<Integer*>(args->arg1.get())->getValue();

            switch (operation) {
                case EQ: BB_BOOLEAN_RESULT(v1 == v2);
                case NEQ: BB_BOOLEAN_RESULT(v1 != v2);
                case LT: BB_BOOLEAN_RESULT(v1 < v2);
                case LE: BB_BOOLEAN_RESULT(v1 <= v2);
                case GT: BB_BOOLEAN_RESULT(v1 > v2);
                case GE: BB_BOOLEAN_RESULT(v1 >= v2);
                case ADD: BB_INT_RESULT(v1 + v2);
                case SUB: BB_INT_RESULT(v1 - v2);
                case MUL: BB_INT_RESULT(v1 * v2);
                case MOD: BB_INT_RESULT(v1 % v2);
                case POW: BB_INT_RESULT(static_cast<int>(std::pow(v1, v2)));
                case DIV: BB_FLOAT_RESULT(v1 / static_cast<float>(v2));
            }
        } else if ((type0 == BB_FLOAT || type0 == BB_INT) && (type1 == BB_FLOAT || type1 == BB_INT)) {
            double v1 = type0 == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg0.get())->getValue())
                                     : static_cast<BFloat*>(args->arg0.get())->getValue();
            double v2 = type1 == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg1.get())->getValue())
                                     : static_cast<BFloat*>(args->arg1.get())->getValue();

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
        }

        if (operation == AT && type1 == STRING) {
            STRING_RESULT(std::move(__python_like_int_format(value, args->arg1->toString())));
        }
        throw Unimplemented();
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOBB_INT: BB_INT_RESULT(value);
            case TOBB_FLOAT: BB_FLOAT_RESULT(value);
            case TOSTR: STRING_RESULT(std::to_string(value));
            case TOVECTOR: return std::make_shared<Vector>(value, true);
        }
        throw Unimplemented();
    }

    throw Unimplemented();
}
