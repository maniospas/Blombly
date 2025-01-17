#ifndef EXECUTE_BLOCK_CPP
#define EXECUTE_BLOCK_CPP

#include "interpreter/functional.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Jitable.h"

Result ExecutionInstance::run(Code* code) {
    bbassert(code->getProgram()==&program, "Internal error: it should be impossible to change the global program pointer.");
    auto program = code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();
    try {
        for (; i <= end; ++i) {
            handleCommand(i);
            if (returnSignal) {
                Result res(result);
                memory->runFinally();
                return std::move(res);
            }
        }
    } 
    catch (const BBError& e) {handleExecutionError(i, e);}
    return std::move(Result(result));
}


#endif