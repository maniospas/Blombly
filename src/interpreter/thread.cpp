#include "interpreter/thread.h"
#include "data/Future.h"
#include "interpreter/functional.h"
#include "data/Struct.h"


void threadExecute(unsigned int depth,
                   Code* code,
                   BMemory* memory,
                   ThreadResult* result,
                   const Command* command,
                   DataPtr thisObj) {

    std::unique_lock<std::recursive_mutex> executorLock;
    if(thisObj.exists()) {
        bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
        executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj.get())->memoryLock);
    }

    try {
        ExecutionInstance executor(depth, code, memory, thisObj.exists());
        Result returnedValue = executor.run(code);
        DataPtr value = returnedValue.get();
        if(!executor.hasReturned()) {
            value = nullptr;
            returnedValue = Result(DataPtr::NULLP);
        }
        result->value = std::move(returnedValue);

    } 
    catch (const BBError& e) {
        result->error = new BBError(enrichErrorDescription(*command, e.what()));
    }
    try {
        memory->detach(nullptr); 
        memory->setToNullIgnoringFinals(variableManager.thisId);
        delete memory;
    } 
    catch (const BBError& e) {
        result->error = new BBError(enrichErrorDescription(*command, e.what()));
    }
}