// Code.cpp
#include "Code.h"
#include "common.h"

// Constructor to initialize Code object with program segment details
Code::Code(void* programAt, int startAt, int endAt, std::shared_ptr<Memory> declMemory)
    : program(programAt), start(startAt), end(endAt), declarationMemory(std::move(declMemory)) {}

// Return the type ID
int Code::getType() const {
    return CODE;
}

// Convert to string representation
std::string Code::toString() const {
    return "code from " + std::to_string(start) + " to " + std::to_string(end);
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
std::shared_ptr<Memory>& Code::getDeclarationMemory() {
    return declarationMemory;
}

// Create a shallow copy of this Code object
std::shared_ptr<Data> Code::shallowCopy() const {
    return std::make_shared<Code>(program, start, end, declarationMemory);
}

// Implement the specified operation for the Code class
std::shared_ptr<Data> Code::implement(const OperationType operation, const BuiltinArgs& args)  {
    if (args.size == 1 && operation == TOCOPY) {
        return std::make_shared<Code>(program, start, end, declarationMemory);
    }

    // Unimplemented operation
    throw Unimplemented();
}
