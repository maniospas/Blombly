#include "data/Jitable.h"
#include "BMemory.h"
#include "interpreter/Command.h"
#include "common.h"
#include <unordered_set>

// Constructor for ReturnPrimitiveJitable
ReturnPrimitiveJitable::ReturnPrimitiveJitable(Data* primitive) : primitive(primitive) {}

// Implementation of the run method for ReturnPrimitiveJitable
bool ReturnPrimitiveJitable::run(BMemory* memory, Data*& returnValue, bool &returnSignal) {
    returnValue = primitive;
    returnSignal = true;
    return true;
}

// Implementation of the jit function
Jitable* jit(const Code* code) {
    std::vector<Command*>* program = code->getProgram();
    int start = code->getStart();
    int end = code->getEnd();
    int size = end-start;
    if(size==2) {
        Command* c0 = program->at(start);
        Command* c1 = program->at(start+1);
        if(c0->operation==BUILTIN && c1->operation==RETURN && c0->args[0]==c1->args[1]) 
            return new ReturnPrimitiveJitable(c0->value);
    }

    /*std::unordered_set<int> inputs;
    std::unordered_set<int> outputs;

    for(int i=start;i<=end;++i) {

    }*/
    return nullptr;
}
