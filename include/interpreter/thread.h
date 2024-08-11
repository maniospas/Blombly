#ifndef THREAD_EXECUTE_H
#define THREAD_EXECUTE_H

#include <vector>
#include <memory>
#include "common.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Struct.h"
#include "BMemory.h"


void threadExecute(Code* code,
                  const std::shared_ptr<BMemory>& memory,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins,
                  std::shared_ptr<ThreadResult> result,
                  Command* command) {
        try {
            bool* call_returnSignal = new bool(false);
            BuiltinArgs* call_args = new BuiltinArgs();
            Data *value = executeBlock(code, memory, call_returnSignal, call_args);
            delete call_returnSignal;
            delete call_args;
            for(Future* thread : memory->attached_threads)
                thread->getResult();
            memory->attached_threads.clear();
            if(value) {
                if((value->getType()!=CODE || !((Code*)value)->getDeclarationMemory()->isOrDerivedFrom(memory))
                && (value->getType()!=STRUCT || !((Struct*)value)->getMemory()->isOrDerivedFrom(memory))) {
                    value = value->shallowCopyIfNeeded();
                    memory->release(); // release only after the copy, as the release will delet the original value stored internally
                }
                else {
                    value = value->shallowCopyIfNeeded();
                }
            }
            else
                memory->release();
            result->value = value;
        }
        catch(const BBError& e) {
            std::string comm = command->toString();
            comm.resize(40, ' ');
            result->error = new BBError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        }
}


#endif // THREAD_EXECUTE_H