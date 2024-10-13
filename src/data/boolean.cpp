#include "data/Boolean.h"
#include "common.h"

Boolean::Boolean(bool val) : value(val), Data(BB_BOOL) {}

bool Boolean::isTrue() const {
    return value;
}

std::string Boolean::toString() const {
    return value ? "true" : "false";
}

bool Boolean::getValue() const {
    return value;
}

void Boolean::setValue(bool val) {
    value = val;
}

Result Boolean::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 2 && args->arg0->getType() == BB_BOOL && args->arg1->getType() == BB_BOOL) {
        bool v1 = static_cast<Boolean*>(args->arg0)->getValue();
        bool v2 = static_cast<Boolean*>(args->arg1)->getValue();
        switch (operation) {
            case AND: BB_BOOLEAN_RESULT(v1 && v2);
            case OR: BB_BOOLEAN_RESULT(v1 || v2);
            case EQ: BB_BOOLEAN_RESULT(v1 == v2);
            case NEQ: BB_BOOLEAN_RESULT(v1 != v2);
        }
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOBB_BOOL: BB_BOOLEAN_RESULT(value);
            case NOT: BB_BOOLEAN_RESULT(!value);
        }
        throw Unimplemented();
    }

    throw Unimplemented();
}
