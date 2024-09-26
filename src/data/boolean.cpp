#include "data/Boolean.h"
#include "common.h"

Boolean::Boolean(bool val) : value(val) {}

int Boolean::getType() const {
    return BOOL;
}

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

std::shared_ptr<Data> Boolean::shallowCopy() const {
    return std::make_shared<Boolean>(value);
}

std::shared_ptr<Data> Boolean::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 2 && args->arg0->getType() == BOOL && args->arg1->getType() == BOOL) {
        bool v1 = static_cast<Boolean*>(args->arg0.get())->getValue();
        bool v2 = static_cast<Boolean*>(args->arg1.get())->getValue();
        switch (operation) {
            case AND: BOOLEAN_RESULT(v1 && v2);
            case OR: BOOLEAN_RESULT(v1 || v2);
            case EQ: BOOLEAN_RESULT(v1 == v2);
            case NEQ: BOOLEAN_RESULT(v1 != v2);
        }
    }

    if (args->size == 1) {
        switch (operation) {
            case TOCOPY:
            case TOBOOL: BOOLEAN_RESULT(value);
            case NOT: BOOLEAN_RESULT(!value);
        }
        throw Unimplemented();
    }

    throw Unimplemented();
}
