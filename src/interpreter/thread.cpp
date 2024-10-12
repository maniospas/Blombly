#include "interpreter/thread.h"
#include "data/Future.h"


void threadExecute(Code* code,
                   BMemory* memory,
                   ThreadResult* result,
                   Command *command) {
    Data* value(nullptr);
    try {
        bool returnSignal(false);
        value = executeBlock(code, memory, returnSignal);
        for (auto& thread : memory->attached_threads) 
            thread->getResult();
        memory->attached_threads.clear(); // TODO: maybe we need to release the memory instead (investigate)
        if(!returnSignal)
            value = nullptr;
        if(value && value->getType()==CODE)
            static_cast<Code*>(value)->setDeclarationMemory(nullptr);
        result->value = value;
    } catch (const BBError& e) {
        // Capture and format the error message
        std::string comm = command->toString();
        comm.resize(40, ' ');
        result->error = new BBError(e.what() + 
            ("\n   \x1B[34m\u2192\033[0m " + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line)));
    }

    try {
        // value should have been 
        delete memory;
    } catch (const BBError& e) {
        std::string comm = command->toString();
        comm.resize(40, ' ');
        result->error = new BBError(e.what() + 
            ("\n   \x1B[34m\u2192\033[0m " + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line)));
    }
}