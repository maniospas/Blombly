#include "data/Integer.h"
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"
#include "data/Iterator.h"
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

Integer::Integer(int64_t val) : value(val), Data(BB_INT) {}

std::string Integer::toString(){
    return std::to_string(value);
}

int64_t Integer::getValue() const {
    return value;
}

void Integer::setValue(int64_t val) {
    value = val;
}

bool Integer::isSame(Data* other) {
    if(other->getType()!=BB_INT)
        return false;
    return static_cast<Integer*>(other)->value==value;
}


size_t Integer::toHash() const {
    return std::hash<int>{}(value);
}

Result Integer::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 2) {
        int64_t type0 = args->arg0->getType();
        int64_t type1 = args->arg1->getType();

        if (type0 == BB_INT && type1 == BB_INT) {
            int64_t v1 = static_cast<Integer*>(args->arg0)->getValue();
            int64_t v2 = static_cast<Integer*>(args->arg1)->getValue();

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
                case TORANGE: return Result(new IntRange(v1, v2, 1));
            }
        } 
        else if ((type0 == BB_FLOAT || type0 == BB_INT) && (type1 == BB_FLOAT || type1 == BB_INT)) {
            double v1 = type0 == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg0)->getValue())
                                     : static_cast<BFloat*>(args->arg0)->getValue();
            double v2 = type1 == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg1)->getValue())
                                     : static_cast<BFloat*>(args->arg1)->getValue();

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
            STRING_RESULT((__python_like_int_format(value, args->arg1->toString())));
        }
        throw Unimplemented();
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOBB_INT: BB_INT_RESULT(value);
            case TOBB_FLOAT: BB_FLOAT_RESULT(value);
            case TOSTR: STRING_RESULT(std::to_string(value));
            case TOVECTOR: return Result(new Vector(value, true));
            case TORANGE: return Result(new IntRange(0, value, 1));
            case TOBB_BOOL: BB_BOOLEAN_RESULT(value);
        }
        throw Unimplemented();
    }

    
    if (args->size == 3 && operation == TORANGE) {
        if(args->arg0->getType()==BB_INT && args->arg1->getType()==BB_INT && args->arg2->getType()==BB_INT) {
            int64_t v0 = static_cast<Integer*>(args->arg0)->getValue();
            int64_t v1 = static_cast<Integer*>(args->arg1)->getValue();
            int64_t v2 = static_cast<Integer*>(args->arg2)->getValue();
            return std::move(Result(new IntRange(v0, v1, v2)));
        }
        else {
            double v0 = args->arg0->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg0)->getValue()) : static_cast<BFloat*>(args->arg0)->getValue();
            double v1 = args->arg1->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg1)->getValue()) : static_cast<BFloat*>(args->arg1)->getValue();
            double v2 = args->arg2->getType() == BB_INT ? static_cast<double>(static_cast<Integer*>(args->arg2)->getValue()) : static_cast<BFloat*>(args->arg2)->getValue();
            return std::move(Result(new FloatRange(v0, v1, v2)));
        }
    }

    throw Unimplemented();
}
