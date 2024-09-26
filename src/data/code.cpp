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

Code::Code(const std::shared_ptr<std::vector<Command>>& programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(std::make_shared<Metadata>()), scheduleForParallelExecution(false) {}

Code::Code(const std::shared_ptr<std::vector<Command>>& programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory, const std::shared_ptr<Metadata>& metadata)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(metadata), scheduleForParallelExecution(false) {}

void Code::setMetadata(int id, const std::shared_ptr<Data>& data) {
    if (metadata->metadata[id] && metadata->metadata[id] != data) {
        bberror(toString() + " already has the specification entry: " + variableManager.getSymbol(id));
    }
    metadata->metadata[id] = data;
}

std::shared_ptr<Metadata> Code::getAllMetadata() const {
    return metadata;
}

std::shared_ptr<Data> Code::getMetadata(int id) const {
    if (metadata->metadata.empty())
        bberror(toString() + " has no declared specification");
    std::shared_ptr<Data> ret = metadata->metadata[id];
    if (!ret)
        bberror(toString() + " has no specification entry: " + variableManager.getSymbol(id));
    return ret;
}

bool Code::getMetadataBool(int id, bool def) const {
    if (metadata->metadata.empty())
        return def;
    std::shared_ptr<Data> ret = metadata->metadata[id];
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

std::shared_ptr<std::vector<Command>> Code::getProgram() const {
    return program;
}

std::shared_ptr<BMemory> Code::getDeclarationMemory() const {
    return declarationMemory;
}

void Code::setDeclarationMemory(const std::shared_ptr<BMemory>& newMemory) {
    declarationMemory = newMemory;
}

std::shared_ptr<Data> Code::shallowCopy() const {
    auto ret = std::make_shared<Code>(program, start, end, declarationMemory, metadata);
    ret->scheduleForParallelExecution = scheduleForParallelExecution;
    return ret;
}

std::shared_ptr<Data> Code::implement(const OperationType operation, BuiltinArgs* args) {
    if (args->size == 1)
        return shallowCopy();
    
    throw Unimplemented();
}
