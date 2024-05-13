#ifndef THREAD_EXECUTE_H
#define THREAD_EXECUTE_H

#include <vector>
#include <memory>
#include "common.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "BMemory.h"


void threadExecute(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins,
                  std::shared_ptr<ThreadResult> result) {
        bool* call_returnSignal = new bool(false);
        BuiltinArgs* call_args = new BuiltinArgs();
        Data *value = executeBlock(program, start, end, memory, call_returnSignal, call_args);
        delete call_returnSignal;
        delete call_args;
        for(Future* thread : memory->attached_threads)
            thread->getResult();
        memory->attached_threads.clear();
        if(value) {
            if((value->getType()!=CODE || !((Code*)value)->getDeclarationMemory()->isOrDerivedFrom(memory))
            && (value->getType()!=STRUCT || !((Struct*)value)->getMemory()->isOrDerivedFrom(memory))) {
                value = value->shallowCopy();
                memory->release(); // release only after the copy, as the release will delet the original value stored internally
            }
            else {
                value = value->shallowCopy();
            }
        }
        else
            memory->release();
        result->value = value;
}


#endif // THREAD_EXECUTE_H