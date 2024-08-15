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
                  BMemory* memory,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins,
                  std::shared_ptr<ThreadResult> result,
                  Command* command) {
        try {
            Data *value = executeBlock(code, memory, returnSignal, allocatedBuiltins);
            for(Future* thread : memory->attached_threads)
                thread->getResult();
            memory->attached_threads.clear();
            if(value)
                value = value->shallowCopyIfNeeded();
            result->value = value;
        }
        catch(const BBError& e) {
            std::string comm = command->toString();
            comm.resize(40, ' ');
            result->error = new BBError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        }
        try {
            delete memory;
        }
        catch(const BBError& e) {
            std::string comm = command->toString();
            comm.resize(40, ' ');
            result->error = new BBError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        }
}


#endif // THREAD_EXECUTE_H