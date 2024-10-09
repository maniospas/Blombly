#ifndef EXECUTE_BLOCK_CPP
#define EXECUTE_BLOCK_CPP

#include "interpreter/functional.h"

Data* executeBlock(Code* code, BMemory* memory, bool &returnSignal) {
    BuiltinArgs args;
    Data* value = nullptr;
    auto program = code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();

    try {
        for (; i <= end; ++i) {
            handleCommand(program, i, memory, returnSignal, args, value);
            if (returnSignal) 
                return value;
        }
    } 
    catch (const BBError& e) {
        handleExecutionError(program, i, e);
    }

    return value;
}


#endif