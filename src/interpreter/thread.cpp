#include "interpreter/thread.h"
#include "data/Future.h"
#include "interpreter/functional.h"
#include "data/Struct.h"


void threadExecute(Code* code,
                   BMemory* memory,
                   ThreadResult* result,
                   Command *command,
                   Data* thisObj) {

    std::unique_lock<std::recursive_mutex> executorLock;
    if(thisObj) {
        bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
        executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj)->memoryLock);
    }

    try {
        bool returnSignal(false);
        Result returnValue = executeBlock(code, memory, returnSignal, thisObj);
        Data* value = returnValue.get();
        if(!returnSignal) {
            value = nullptr;
            returnValue = Result(nullptr);
        }
        if(value) value->leak();
        result->value = std::move(returnValue);

    } 
    catch (const BBError& e) {
        result->error = new BBError(enrichErrorDescription(command, e.what()));
    }

    try {
        memory->detach(nullptr); 
        memory->unsafeSet(variableManager.thisId, nullptr, nullptr);
        delete memory;
    } 
    catch (const BBError& e) {
        result->error = new BBError(enrichErrorDescription(command, e.what()));
    }
}