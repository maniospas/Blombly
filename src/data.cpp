#include "Data.h"

std::shared_ptr<Data> Data::run(const OperationType operation, const BuiltinArgs* args) {
    if (args->size>0) {
        try {
            return args->arg0->implement(operation, args);
        }
        catch (Unimplemented) {
            // Handle unimplemented methods
        }
    }
    if (args->size>1) {
        try {
            return args->arg1->implement(operation, args);
        }
        catch (Unimplemented) {
            // Handle unimplemented methods
        }
    }
    if (args->size>2) {
        try {
            return args->arg2->implement(operation, args);
        }
        catch (Unimplemented) {
            // Handle unimplemented methods
        }
    }

    std::string err = "No valid builtin implementation for this method: " + getOperationTypeName(operation) + "(";
    int i = 0;
    if(args->size>0) {
        Data* arg = args->arg0;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    if(args->size>1) {
        Data* arg = args->arg1;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    if(args->size>2) {
        Data*arg = args->arg2;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    err += ")";
    std::cerr << err << std::endl;
    exit(1);
    return nullptr;
}
