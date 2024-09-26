#ifndef EXECUTE_BLOCK_CPP
#define EXECUTE_BLOCK_CPP

#include "interpreter/functional.h"

std::shared_ptr<Data> executeBlock(const std::shared_ptr<Code>& code, const std::shared_ptr<BMemory>& memory, bool  &returnSignal, const BuiltinArgs& allocatedBuiltins) {
    BuiltinArgs args;
    std::shared_ptr<Data> value = nullptr;
    auto program = code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();

    try {
        for (; i <= end; ++i) {
            value = handleCommand(program, i, memory, returnSignal, args);
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