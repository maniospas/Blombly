// Code.cpp
#include "data/Code.h"
#include "common.h"
#include "BMemory.h"

Metadata::Metadata() {
}

Metadata::~Metadata() {
    for(const auto& element : metadata)
        if(element.second && element.second->isDestroyable) {
            //std::cout << "deleting\n";
            //std::cout << "#"<<element.second << "\n";
            //std::cout << element.second->toString() << "\n";
            element.second->isDestroyable = false;
            delete element.second;
        }
}

// Constructor to initialize Code object with program segment details
Code::Code(void* programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(std::make_shared<Metadata>()) {
        // TODO: move all attributes (programAt, startAt, endAt, into Metadata)
    }

Code::Code(void* programAt, int startAt, int endAt, const std::shared_ptr<BMemory>& declMemory, const std::shared_ptr<Metadata>& metadata)
    : program(programAt), start(startAt), end(endAt), declarationMemory(declMemory), metadata(metadata) {
    }

void Code::setMetadata(int id, Data* data) {
    if(metadata->metadata[id])
        bberror(toString()+" already has the specification entry: "+variableManager.getSymbol(id));
    metadata->metadata[id] = data;
}

std::shared_ptr<Metadata> Code::getAllMetadata() const {
    return metadata;
}

Data* Code::getMetadata(int id) {
    if(metadata->metadata.empty())
        bberror(toString()+" has no declared specification");
    Data* ret = metadata->metadata[id];
    if(ret==nullptr)
        bberror(toString()+" has no specification entry: "+variableManager.getSymbol(id));
    return ret;
}

// Return the type ID
int Code::getType() const {
    return CODE;
}

// Convert to string representation
std::string Code::toString() const {
    return "code block in lines " + std::to_string(start) + " to " + std::to_string(end);
}

// Get the starting position of the code segment
int Code::getStart() const {
    return start;
}

// Get the ending position of the code segment
int Code::getEnd() const {
    return end;
}

// Get the associated program pointer
void* Code::getProgram() const {
    return program;
}

// Get the memory declarations associated with this code
std::shared_ptr<BMemory> Code::getDeclarationMemory() const {
    return declarationMemory;
}

// Create a shallow copy of this Code object
Data* Code::shallowCopy() const {
    Code* ret = new Code(program, start, end, declarationMemory, metadata);
    return ret;
}

// Implement the specified operation for the Code class
Data* Code::implement(const OperationType operation, BuiltinArgs* args)  {
    if (args->size == 1) 
        return shallowCopy();
    
    // Unimplemented operation
    throw Unimplemented();
}
