#include "interpreter/thread.h"
#include "data/Future.h"


void threadExecute(std::shared_ptr<Code> code,
                   std::shared_ptr<BMemory> memory,
                   std::shared_ptr<ThreadResult> result,
                   Command *command) {
    try {
        bool returnSignal(false);
        BuiltinArgs allocatedBuiltins;
        std::shared_ptr<Data> value = executeBlock(code, memory, returnSignal, allocatedBuiltins);
        for (auto& thread : memory->attached_threads) 
            thread->getResult();
        memory->attached_threads.clear();
        if (value) 
            value = value->shallowCopy();
        result->value = value;
    } catch (const BBError& e) {
        // Capture and format the error message
        std::string comm = command->toString();
        comm.resize(40, ' ');
        result->error = std::make_shared<BBError>(e.what() + 
            ("\n   \x1B[34m\u2192\033[0m " + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line)));
    }

    try {
        memory.reset();
    } catch (const BBError& e) {
        std::string comm = command->toString();
        comm.resize(40, ' ');
        result->error = std::make_shared<BBError>(e.what() + 
            ("\n   \x1B[34m\u2192\033[0m " + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line)));
    }
}