#ifndef EXECUTE_BLOCK_CPP
#define EXECUTE_BLOCK_CPP

#include "interpreter/functional.h"
#include "data/BError.h"
#include "data/Future.h"
#include "data/Jitable.h"

Result executeBlock(Code* code, BMemory* memory, bool &returnSignal) {
    BuiltinArgs args;
    Data* value = nullptr;

    auto program = code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();

    try {
        for (; i <= end; ++i) {
            handleCommand(program, i, memory, returnSignal, args, value);
            if (returnSignal) {
                memory->runFinally();
                //if(value && value->getType()==FUTURE) return std::move(static_cast<Future*>(value)->getResult());
                return std::move(Result(value));
            }
        }
        //if(value && value->getType()==FUTURE) value = static_cast<Future*>(value)->getResult();
    } 
    catch (const BBError& e) {
        //return std::move(Result(new BError(std::move(e.what()))));
        handleExecutionError(program, i, e);
        value = nullptr;
        // return std::move(Result(nullptr));
    }
    //memory->runFinally();
    return std::move(Result(value));
}


#endif