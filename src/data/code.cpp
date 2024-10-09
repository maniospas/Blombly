#include "data/Code.h"
#include "common.h"
#include "BMemory.h"

Metadata::Metadata() {}

Metadata::~Metadata() {
    for (const auto& element : metadata) {
        if (element.second && element.second->isDestroyable) {
            element.second->isDestroyable = false;
        }
    }
}

Code::Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(new Metadata()), scheduleForParallelExecution(false) {}

Code::Code(std::vector<Command*>* programAt, int startAt, int endAt, BMemory* declMemory, Metadata* metadata)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(metadata), scheduleForParallelExecution(false) {}

void Code::setMetadata(int id, Data* data) {
    if (metadata->metadata[id] && metadata->metadata[id] != data) {
        bberror(toString() + " already has the specification entry: " + variableManager.getSymbol(id));
    }
    metadata->metadata[id] = data;
}

Metadata* Code::getAllMetadata() const {
    return metadata;
}

Data* Code::getMetadata(int id) const {
    if (metadata->metadata.empty())
        bberror(toString() + " has no declared specification");
    Data* ret = metadata->metadata[id];
    if (!ret)
        bberror(toString() + " has no specification entry: " + variableManager.getSymbol(id));
    return ret;
}

bool Code::getMetadataBool(int id, bool def) const {
    if (metadata->metadata.empty())
        return def;
    Data* ret = metadata->metadata[id];
    if (!ret)
        return def;
    return ret->isTrue();
}

int Code::getType() const {
    return CODE;
}

std::string Code::toString() const {
    return "code block in lines " + std::to_string(start) + " to " + std::to_string(end);
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

Data* Code::implement(const OperationType operation, BuiltinArgs* args) {
    //if (args->size == 1)
    //    return this;
    
    throw Unimplemented();
}
