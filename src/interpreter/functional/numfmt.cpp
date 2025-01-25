#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>
#include <cstdint>

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


std::string __python_like_int_format(int64_t int_number, const std::string& format) {
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
