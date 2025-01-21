#include "data/Code.h"
#include "common.h"
#include "BMemory.h"
#include "data/Jitable.h"

Code::Code(const std::vector<Command>* programAt, int startAt, int endAt, int premature_end)
    : program(programAt), start(startAt), end(endAt), scheduleForParallelExecution(true), Data(CODE), jitable(nullptr), premature_end(premature_end) {}

std::string Code::toString(BMemory* memory){
    if(jitable) return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end) + " with "+jitable->toString();
    return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end);
}
int Code::getStart() const {return start;}
int Code::getEnd() const {return end;}
int Code::getOptimizedEnd() const {return premature_end;}
const std::vector<Command>* Code::getProgram() const {return program;}
