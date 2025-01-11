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
            returnValue = (Result(nullptr));
        }
        memory->detach(nullptr); // synchronizes threads
        if(value) value->leak();
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