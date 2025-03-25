/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <bitset>
#include <cstdint>
#include <limits>
#include <cmath>
#include "common.h"

// Helper function for formatted output similar to Python's float formatting
std::string __python_like_float_format(double number, const std::string& format) {
    if(format.find("%Y") != std::string::npos || 
        format.find("%y") != std::string::npos || 
        format.find("%m") != std::string::npos || 
        format.find("%d") != std::string::npos || 
        format.find("%D") != std::string::npos || 
        format.find("%H") != std::string::npos || 
        format.find("%M") != std::string::npos || 
        format.find("%S") != std::string::npos) {
        if(!std::isfinite(number)) bberror("Date formatting failed: cannot parse non-finite float.");
        bbassert(number >= (double)std::numeric_limits<std::time_t>::min() && number <= (double)std::numeric_limits<std::time_t>::max(), "Date formatting failed: numeric timestamp is out of range.");
        std::time_t t = static_cast<std::time_t>(number);
        std::tm* tm_ptr = std::localtime(&t);
        if(!tm_ptr) bberror("Date formatting failed: numeric timestamp is out of range.");
        char buf[128];
        if(std::strftime(buf, sizeof(buf), format.c_str(), tm_ptr)) return std::string(buf);
        bberror("Date formatting failed: result is over 128 characters long.");
    }

    std::ostringstream output;
    size_t precision_pos = format.find('.');
    size_t type_pos = format.find_last_of("fFeEgGdxXob");

    int precision = 6; // Default precision
    char type = 'f';   // Default to fixed-point notation

    if(precision_pos != std::string::npos && (type_pos == std::string::npos || precision_pos < type_pos)) 
        precision = std::stoi(format.substr(precision_pos + 1, type_pos - precision_pos - 1));

    if(type_pos != std::string::npos) 
        type = format[type_pos];

    bool is_integer_format = (type == 'd' || type == 'x' || type == 'X' || type == 'o' || type == 'b');
    if(is_integer_format) {
        int int_number = static_cast<int>(number);
        switch(type) {
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

    if(format.find("%Y") != std::string::npos || 
        format.find("%y") != std::string::npos || 
        format.find("%m") != std::string::npos || 
        format.find("%d") != std::string::npos || 
        format.find("%D") != std::string::npos || 
        format.find("%H") != std::string::npos || 
        format.find("%M") != std::string::npos || 
        format.find("%S") != std::string::npos) {
        double number = (double)int_number;
        bbassert(number >= (double)std::numeric_limits<std::time_t>::min() && number <= (double)std::numeric_limits<std::time_t>::max(), "Date formatting failed: numeric timestamp is out of range.");
        std::time_t t = static_cast<std::time_t>((double)int_number);
        std::tm* tm_ptr = std::localtime(&t);
        if(!tm_ptr) bberror("Date formatting failed: numeric timestamp is out of range.");
        char buf[128];
        if(std::strftime(buf, sizeof(buf), format.c_str(), tm_ptr)) return std::string(buf);
        bberror("Date formatting failed: result is over 128 characters long.");
    }


    std::ostringstream output;
    size_t precision_pos = format.find('.');
    size_t type_pos = format.find_last_of("fFeEgGdxXob");

    int precision = 6;
    char type = 'f';

    if(precision_pos != std::string::npos && (type_pos == std::string::npos || precision_pos < type_pos)) 
        precision = std::stoi(format.substr(precision_pos + 1, type_pos - precision_pos - 1));

    if(type_pos != std::string::npos) 
        type = format[type_pos];

    bool is_integer_format = (type == 'd' || type == 'x' || type == 'X' || type == 'o' || type == 'b');

    if(is_integer_format) {
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
