#include "data/BString.h"
#include "data/Boolean.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BFile.h"
#include "data/Iterator.h"
#include "common.h"

BString::BString(const std::string& val) : value(val) {}

int BString::getType() const {
    return STRING;
}

std::string BString::toString() const {
    return value;
}

std::shared_ptr<Data> BString::shallowCopy() const {
    return std::make_shared<BString>(value);
}

std::shared_ptr<Data> BString::implement(const OperationType operation, BuiltinArgs* args) {

    if (args->size == 2 && args->arg0->getType() == STRING && args->arg1->getType() == STRING) {
        std::string v1 = static_cast<BString*>(args->arg0.get())->value;
        std::string v2 = static_cast<BString*>(args->arg1.get())->value;
        switch (operation) {
            case EQ: BB_BOOLEAN_RESULT(v1 == v2);
            case NEQ: BB_BOOLEAN_RESULT(v1 != v2);
            case ADD: STRING_RESULT(v1 + v2);
        }
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY: 
            case TOSTR: STRING_RESULT(value);
            case TOBB_INT: {
                char* endptr = nullptr;
                int ret = std::strtol(value.c_str(), &endptr, 10);
                if (endptr == value.c_str() || *endptr != '\0') {
                    return nullptr;
                }
                BB_INT_RESULT(ret);
            }
            case LEN: BB_INT_RESULT(value.size());
            case TOBB_FLOAT: {
                char* endptr = nullptr;
                double ret = std::strtod(value.c_str(), &endptr);
                if (endptr == value.c_str() || *endptr != '\0') {
                    return nullptr;
                }
                BB_FLOAT_RESULT(ret);
            }
            case TOBB_BOOL: BB_BOOLEAN_RESULT(value == "true");
            case TOITER: return std::make_shared<Iterator>(args->arg0);
            case TOFILE: return std::make_shared<BFile>(value);
        }
        throw Unimplemented();
    }

    if (operation == AT && args->size == 2 && args->arg1->getType() == BB_INT) {
        int index = static_cast<Integer*>(args->arg1.get())->getValue();
        if (index < 0 || index >= value.size()) {
            bberror("String index " + std::to_string(index) + " out of range [0," + std::to_string(value.size()) + ")");
            return nullptr;
        }
        return std::make_shared<BString>(std::string(1, value[index]));
    }

    throw Unimplemented();
}
