#include "interpreter/thread.h"
#include "data/Future.h"
#include "interpreter/functional.h"


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

    } 
    catch (const BBError& e) {
        memory->unsafeSet(variableManager.thisId, nullptr, nullptr);
        result->error = new BBError(enrichErrorDescription(command, e.what()));
    }

    try {
        memory->unsafeSet(variableManager.thisId, nullptr, nullptr);
        // value should have been 
        delete memory;
    } 
    catch (const BBError& e) {
        result->error = new BBError(enrichErrorDescription(command, e.what()));
    }
}