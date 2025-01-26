#include "data/Code.h"
#include "common.h"
#include "BMemory.h"
#include "data/Jitable.h"

Code::Code(const std::vector<Command>* programAt, size_t startAt, size_t endAt, size_t premature_end)
    : program(programAt), start(startAt), end(endAt), scheduleForParallelExecution(true), Data(CODE), jitable(nullptr), premature_end(premature_end) {}

std::string Code::toString(BMemory* memory){
    if(jitable) return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end) + " with "+jitable->toString();
    return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end);
}
size_t Code::getStart() const {return start;}
size_t Code::getEnd() const {return end;}
size_t Code::getOptimizedEnd() const {return premature_end;}
const std::vector<Command>* Code::getProgram() const {return program;}
