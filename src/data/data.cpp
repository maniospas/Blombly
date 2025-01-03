#include "data/Data.h"

Data::Data(Datatype type) : type(type), referenceCounter(0) {}
void Data::addOwner() {++referenceCounter;}
void Data::removeFromOwner() {if((--referenceCounter)==0) delete this;}
void Data::leak() {}
bool Data::isSame(Data* other) {return other==this;}
size_t Data::toHash() const {return (size_t)this;}
Result Data::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {throw Unimplemented();}

Result Data::run(const OperationType operation, BuiltinArgs *args, BMemory* memory) {
    try {return std::move(args->arg0->implement(operation, args, memory));}
    catch (Unimplemented&) {}

    int size = args->size;
    --size;
    if (size) {
        try {return args->arg1->implement(operation, args, memory);}
        catch (Unimplemented&) {}
        --size;
        if (size) {
            try {return args->arg2->implement(operation, args, memory);}
            catch (Unimplemented&) {}
        }
    }
    std::string err = "No valid builtin implementation for this method: " + getOperationTypeName(operation) + "(";
    int i = 0;
    if (args->size > 0) {
        auto arg = args->arg0;
        if (i++) err += ",";
        if (arg) err += datatypeName[arg->getType()];
    }
    if (args->size > 1) {
        auto arg = args->arg1;
        if (i++) err += ",";
        if (arg) err += datatypeName[arg->getType()];
    }
    if (args->size > 2) {
        auto arg = args->arg2;
        if (i++) err += ",";
        if (arg) err += datatypeName[arg->getType()];
    }
    err += ")";
    bberror(err);
    return std::move(Result(nullptr));
}