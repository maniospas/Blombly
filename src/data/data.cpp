#include "data/Data.h"

int Data::numObjects = 0;

Data::Data(int type) : type(type), isContained(true), referenceCounter(0) {
    //++numObjects;
}

Data::~Data() {
    //--numObjects;
}

void Data::addOwner() {
    /*if(isContained) {
        int& ref = *reinterpret_cast<int*>(&referenceCounter);
        ++ref;
    }
    else*/
        ++referenceCounter;
    //std::cout << "added "<<(referenceCounter)<<" "<<this<<" "<<toString()<<"\n";
}

void Data::removeFromOwner() {
    //std::cout << "removed "<<(referenceCounter-1)<<" "<<this<<"\n";
   /* if(isContained) {
        int& ref = *reinterpret_cast<int*>(&referenceCounter);
        if((--ref)==0) 
            delete this;
    }
    else*/ if((--referenceCounter)==0) 
        delete this;
}

int Data::countObjects() {
    return numObjects;
}

void Data::leak() {
    //isContained = false;
}

Result Data::run(const OperationType operation, BuiltinArgs *args) {
    try {
        return std::move(args->arg0->implement(operation, args));
    }
    catch (Unimplemented&) {
        // Handle unimplemented methods
    }

    int size = args->size;
    --size;
    if (size) {
        try {
            return args->arg1->implement(operation, args);
        }
        catch (Unimplemented&) {
            // Handle unimplemented methods
        }
        --size;
        if (size) {
            try {
                return args->arg2->implement(operation, args);
            }
            catch (Unimplemented&) {
                // Handle unimplemented methods
            }
        }
    }

    std::string err = "No valid builtin implementation for this method: " + getOperationTypeName(operation) + "(";
    int i = 0;
    if (args->size > 0) {
        auto arg = args->arg0;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    if (args->size > 1) {
        auto arg = args->arg1;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    if (args->size > 2) {
        auto arg = args->arg2;
        if (i++) err += ",";
        if (arg != nullptr) err += datatypeName[arg->getType()];
    }
    err += ")";
    bberror(err);
    return std::move(Result(nullptr));
}

Result Data::implement(const OperationType operation, BuiltinArgs* args) {
    throw Unimplemented();
}

size_t Data::toHash() const {
    return std::hash<std::string>{}(toString());
}
