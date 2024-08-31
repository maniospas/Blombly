// Integer.cpp
#include "data/Integer.h"
#include "common.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>
#include "data/Boolean.h"
#include "data/BFloat.h"
#include "data/Vector.h"
#include "data/BString.h"


std::string __python_like_int_format(int int_number, const std::string& format) {
    std::ostringstream output;

    // Parse the format string
    size_t precision_pos = format.find('.');
    size_t type_pos = format.find_last_of("fFeEgGdxXob");

    // Default settings
    int precision = 6; // Default precision for floating-point numbers
    char type = 'f';   // Default to fixed-point notation for floating-point numbers

    // Handle precision if specified
    if (precision_pos != std::string::npos) {
        if (type_pos == std::string::npos || precision_pos < type_pos) {
            precision = std::stoi(format.substr(precision_pos + 1, type_pos - precision_pos - 1));
        }
    }

    // Handle type if specified
    if (type_pos != std::string::npos) {
        type = format[type_pos];
    }

    // Check if the format is for an integer type
    bool is_integer_format = (type == 'd' || type == 'x' || type == 'X' || type == 'o' || type == 'b');

    if (is_integer_format) {
        // Format the integer according to the specified format
        switch (type) {
            case 'd': // Decimal format
                output << int_number;
                break;
            case 'x': // Hexadecimal format (lowercase)
                output << std::hex << int_number;
                break;
            case 'X': // Hexadecimal format (uppercase)
                output << std::uppercase << std::hex << int_number;
                break;
            case 'o': // Octal format
                output << std::oct << int_number;
                break;
            case 'b': { // Binary format
                output << std::bitset<sizeof(int) * 8>(int_number);
                break;
            }
            default: // Unsupported integer format
                throw std::invalid_argument("Unsupported integer format specifier.");
        }
    } else {
        double number = static_cast<double>(int_number);

        static_cast<int>(number);
        // Apply floating-point formatting
        output << std::setprecision(precision);

        switch (type) {
            case 'f': // Fixed-point notation (lowercase)
            case 'F': // Fixed-point notation (uppercase)
                output << std::fixed << number;
                break;
            case 'e': // Exponential notation (lowercase)
                output << std::scientific << number;
                break;
            case 'E': // Exponential notation (uppercase)
                output << std::scientific << std::uppercase << number;
                break;
            case 'g': // General format (lowercase)
            case 'G': // General format (uppercase)
                output << std::defaultfloat << number;
                if (type == 'G') output << std::uppercase;
                break;
            default: // Default case: Treat as fixed-point
                output << std::fixed << number;
        }
    }

    return output.str();
}

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
                case DIV: FLOAT_RESULT(v1/static_cast<float>(v2));
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
        
        if(operation==AT && type1==STRING) 
            STRING_RESULT(std::move(__python_like_int_format(value, args->arg1->toString())));
        throw Unimplemented();
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
