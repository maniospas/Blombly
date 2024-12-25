#include "interpreter/thread.h"
#include "data/Future.h"


void threadExecute(Code* code,
                   BMemory* memory,
                   ThreadResult* result,
                   Command *command) {
    try {
        bool returnSignal(false);
        Result returnValue = executeBlock(code, memory, returnSignal);
        Data* value = returnValue.get();
        if(!returnSignal) {
            value = nullptr;
            returnValue = (Result(nullptr));
        }
        memory->detach(nullptr); // synchronizes threads
        if(value)
            value->leak();
        if(value && value->getType()==CODE)
            static_cast<Code*>(value)->setDeclarationMemory(nullptr);
        result->value = std::move(returnValue);
        memory->unsafeSet(variableManager.thisId, nullptr, nullptr);
        //memory->await(); // await here to prevent awaiting during the destructor

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