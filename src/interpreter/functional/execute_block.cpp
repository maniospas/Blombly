#ifndef EXECUTE_BLOCK_CPP
#define EXECUTE_BLOCK_CPP

#include "interpreter/functional.h"
#include "data/BError.h"

Result executeBlock(Code* code, BMemory* memory, bool &returnSignal) {
    BuiltinArgs args;
    Data* value = nullptr;
    auto program = code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();

    try {
        for (; i <= end; ++i) {
            handleCommand(program, i, memory, returnSignal, args, value);
            if (returnSignal) 
                return std::move(Result(value));
        }
    } 
    catch (const BBError& e) {
        //return std::move(Result(new BError(std::move(e.what()))));
        handleExecutionError(program, i, e);
        // return std::move(Result(nullptr));
    }

    return std::move(Result(value));
}


#endif