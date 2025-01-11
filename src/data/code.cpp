#include "data/Code.h"
#include "common.h"
#include "BMemory.h"
#include "data/Jitable.h"

Code::Code(std::vector<Command*>* programAt, int startAt, int endAt)
    : program(programAt), start(startAt), end(endAt), scheduleForParallelExecution(true), Data(CODE), jitable(nullptr) {}

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

Result Code::implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) {
    throw Unimplemented();
}
