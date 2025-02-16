/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
    
    //CodeExiter codeExiter(code);
    /*std::unique_lock<std::recursive_mutex> executorLock;
    if(thisObj.exists()) {
        bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
        executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj.get())->memoryLock);
    }*/

    try {
        ExecutionInstance executor(depth, code, memory, thisObj.exists());
        auto returnedValue = executor.run(code);
        DataPtr value = returnedValue.get();
        if(!returnedValue.returnSignal) {
            value = nullptr;
            returnedValue.result = Result(DataPtr::NULLP);
        }
        result->value = Result(returnedValue.get());

    } 
    catch (const BBError& e) {result->value = Result(new BError(enrichErrorDescription(*command, e.what())));}
    try {
        memory->release();
        delete memory;
    } 
    catch (const BBError& e) {result->value = Result(new BError(enrichErrorDescription(*command, e.what())));}
}