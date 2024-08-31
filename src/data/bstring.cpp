// BString.cpp
#include "data/BString.h"
#include "data/Boolean.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BFile.h"
#include "data/Iterator.h"
#include "common.h"

// Constructor
BString::BString(const std::string& val) : value(val) {}

// Return the type ID
int BString::getType() const {
    return STRING;
}

// Convert to string representation
std::string BString::toString() const {
    return value;
}

// Create a shallow copy of this BString
Data* BString::shallowCopy() const {
    return new BString(value);
}

// Implement the specified operation
Data* BString::implement(const OperationType operation, BuiltinArgs* args)  {

    // Two-argument operations involving strings
    if (args->size == 2 && args->arg0->getType() == STRING && args->arg1->getType() == STRING) {
        std::string v1 = ((BString*)args->arg0)->value;
        std::string v2 = ((BString*)args->arg1)->value;
        switch(operation) {
            case EQ: BOOLEAN_RESULT(v1 == v2);
            case NEQ: BOOLEAN_RESULT(v1 != v2);
            case ADD: STRING_RESULT(v1 + v2);
        }
    }

    if (args->size == 1) {
        switch(operation) {
            case TOCOPY: 
            case TOSTR: STRING_RESULT(value);
            case TOINT: {
                char* endptr = nullptr;
                int ret = std::strtol(value.c_str(), &endptr, 10);
                if (endptr == value.c_str() || *endptr != '\0')
                    return nullptr;
                INT_RESULT(ret);
            }
            case LEN: INT_RESULT(value.size());
            case TOFLOAT: {
                char* endptr = nullptr;
                double ret = std::strtod(value.c_str(), &endptr);
                if (endptr == value.c_str() || *endptr != '\0')
                    return nullptr;
                FLOAT_RESULT(ret);
            }
            case TOBOOL: BOOLEAN_RESULT(value == "true");
            case TOITER: return new Iterator(std::make_shared<IteratorContents>(this));
            case TOFILE: return new BFile(value);
        }
        throw Unimplemented();
    }

    if(operation==AT && args->size==2 && args->arg1->getType()==INT) {
        int index = ((Integer*)args->arg1)->getValue();
        if(index < 0 || index>=value.size()) {
            bberror("String index "+std::to_string(index)+" out of range [0,"+std::to_string(value.size())+")");
            return nullptr;
        }
        return new BString(std::string(1, value[index]));
    }

    // Unimplemented operation
    throw Unimplemented();
}
