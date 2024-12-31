#include "data/Code.h"
#include "common.h"
#include "BMemory.h"
#include "data/Jitable.h"

Metadata::Metadata() {}

Metadata::~Metadata() {
}

Code::Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(new Metadata()), scheduleForParallelExecution(false), Data(CODE), jitable(nullptr) {}

Code::Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory, Metadata* metadata)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(metadata), scheduleForParallelExecution(false), Data(CODE), jitable(nullptr) {}

void Code::setMetadata(int id, Data* data) {
    if(data)
        data->addOwner();
    if (metadata->metadata[id] && metadata->metadata[id] != data) {
        bberror(toString(nullptr) + " already has the specification entry: " + variableManager.getSymbol(id));
    }
    metadata->metadata[id] = data;
}

Metadata* Code::getAllMetadata() const {
    bberror("Code secifications are temporarily unavailable.");
    //return metadata;
}

Data* Code::getMetadata(int id) const {
    bberror("Code secifications are temporarily unavailable.");
    /*if (metadata->metadata.empty())
        bberror("Codehas no declared specification");
    Data* ret = metadata->metadata[id];
    if (!ret)
        bberror(toString() + " has no specification entry: " + variableManager.getSymbol(id));
    return ret;*/
}

bool Code::getMetadataBool(int id, bool def) const {
    bberror("Code secifications are temporarily unavailable.");
    /*
    if (metadata->metadata.empty())
        return def;
    Data* ret = metadata->metadata[id];
    if (!ret)
        return def;
    return ret->isTrue();*/
}

std::string Code::toString(BMemory* memory){
    if(jitable) return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end) + " with "+jitable->toString();
    return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end);
}

int Code::getStart() const {
    return start;
}

int Code::getEnd() const {
    return end;
}

std::vector<Command*>* Code::getProgram() const {
    return program;
}

BMemory* Code::getDeclarationMemory() const {
    return declarationMemory;
}

void Code::setDeclarationMemory(BMemory* newMemory) {
    declarationMemory = newMemory;
}

Result Code::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    //if (args->size == 1)
    //    return this;
    
    throw Unimplemented();
}
