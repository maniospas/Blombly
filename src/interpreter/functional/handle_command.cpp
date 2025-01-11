#include <iostream>
#include "common.h"
#include <memory>

#include "common.h"
#include "data/Struct.h"
#include "data/Code.h"
#include "data/Boolean.h"
#include "data/Future.h"
#include "data/BFile.h"
#include "data/List.h"
#include "data/BHashMap.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "data/BError.h"
#include "data/Integer.h"
#include "data/RestServer.h"
#include "data/Jitable.h"
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"

#define WHILE_WITH_CODE_BLOCKS

extern BError* NO_TRY_INTERCEPT;

std::string replaceEscapeSequences(const std::string& input) {
    std::string output;
    size_t pos = 0;
    while (pos < input.size()) {
        // Look for \e[ in the string
        size_t found = input.find("\\e[", pos);
        if (found != std::string::npos) {
            // Copy everything up to the \e[
            output.append(input, pos, found - pos);

            // Replace \e with the ANSI escape code \033
            output += '\033';

            // Find the end of the ANSI sequence (looking for 'm')
            size_t end = input.find('m', found);
            if (end != std::string::npos) {
                // Copy the rest of the ANSI sequence including 'm'
                output.append(input, found + 2, end - found - 2 + 1);
                pos = end + 1;
            } else {
                // If no 'm' is found, treat it as an incomplete sequence
                output += input.substr(found + 2);
                break;
            }
        } else {
            // No more \e[ found, copy the rest of the string
            output.append(input, pos, input.size() - pos);
            break;
        }
    }
    return output;
}


std::chrono::steady_clock::time_point program_start;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;
std::unordered_map<int, Data*> cachedData;

#define SET_RESULT if(command->args[0]!=variableManager.noneId) memory->unsafeSet(command->args[0], result, nullptr);return


void handleCommand(std::vector<Command*>* program, int& i, BMemory* memory, bool &returnSignal, BuiltinArgs &args, Data*& result, bool forceStayInThread) {
    Command* command = (*program)[i];
    //Data* toReplace = command->nargs?memory->getOrNullShallow(command->args[0]):nullptr;
    //BMemory* memory = memory_.get();

    //std::cout<<command->toString()<<"\t "<<std::this_thread::get_id()<<"\n";
    
    switch (command->operation) {
        case BUILTIN: result = command->value;break;
        case BEGINCACHE: {
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while(pos <= program->size()) {
                command_type = (*program)[pos]->operation;
                if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
                if(command_type == END) {
                    if (depth == 0) break;
                    depth--;
                }
                pos++;
            }
            bbassert(depth >= 0, "Cache declaration never ended.");
            auto cache = new Code(program, i + 1, pos);
            bool cacheReturn(false);
            BMemory cacheMemory(nullptr, 16, nullptr);
            executeBlock(cache, &cacheMemory, cacheReturn, forceStayInThread);
            cacheMemory.await();
            bbassert(!cacheReturn, "Cache declaration cannot return a value");
            for (int key : cacheMemory.finals) {
                Data* obj = cacheMemory.get(key);
                bbassert(obj && obj->getType()!=STRUCT, "Structs cannot be cached");
                obj->addOwner();
                cachedData[key] = obj;
            }
            result = nullptr;
            return;
        }
        case BEGIN:
        case BEGINFINAL: {
            // Start a block of code
            if(command->value) {
                auto code = static_cast<Code*>(command->value);
                result = code;
                if (command->operation == BEGINFINAL) memory->setFinal(command->args[0]);
                i = code->getEnd();
                break;
            }
            // Find the matching END for this block
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while(pos <= program->size()) {
                command_type = (*program)[pos]->operation;
                if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
                if(command_type == END) {
                    if(depth == 0) break;
                    depth--;
                }
                pos++;
            }
            bbassert(depth >= 0, "Code block never ended.");
            auto cache = new Code(program, i + 1, pos);
            cache->addOwner();
            cache->jitable = jit(cache);
            command->value = cache;
            result = cache;
            if(command->operation == BEGINFINAL) memory->setFinal(command->args[0]);
            i = pos;
        } break;

        case CALL: {
            // Function or method call
            Data* context = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            Data* called = memory->get(command->args[2]);
            bbassert(called, "Cannot call a missing value.");
            bbassert(called->getType()==CODE || called->getType()==STRUCT, "Only structs or code blocks can be called.");
            if(called->getType()==STRUCT) {
                auto strct = static_cast<Struct*>(called);
                auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
                bbassert(val && val->getType()==CODE, "Struct was called like a method but has no implemented code for `call`.");
                //static_cast<Code*>(val)->scheduleForParallelExecution = false; // struct calls are never executed in parallel
                memory->codeOwners[static_cast<Code*>(val)] = static_cast<Struct*>(called);
                called = (val);
            }
            auto code = static_cast<Code*>(called);
            if(forceStayInThread || !code->scheduleForParallelExecution || !Future::acceptsThread()) {
                BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if(context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    Result returnedValue = executeBlock(static_cast<Code*>(context), &newMemory, newReturnSignal, forceStayInThread);
                    if(newReturnSignal) {
                        result = returnedValue.get();
                        SET_RESULT;
                        break;
                    }
                }
                //newMemory.detach(code->getDeclarationMemory());
                newMemory.detach(memory);
                auto it = memory->codeOwners.find(code);
                Data* thisObj = (it != memory->codeOwners.end() ? it->second->getMemory() : memory)->getOrNull(variableManager.thisId, true);
                if(thisObj) newMemory.unsafeSet(variableManager.thisId, thisObj, nullptr);
                std::unique_lock<std::recursive_mutex> executorLock;
                if(thisObj) {
                    bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
                    executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj)->memoryLock);
                }
                newMemory.allowMutables = false;
                bool forceStayInThread = thisObj; // overwrite the option
                //newMemory.leak(); (this is for testing only - we are not leaking any memory to other threads if we continue in the same thread, so no need to enable atomic reference counting)
                if(code->jitable && code->jitable->run(&newMemory, result, newReturnSignal, forceStayInThread)) {
                    if(thisObj) newMemory.unsafeSet(variableManager.thisId, nullptr, nullptr);
                    SET_RESULT;
                }
                else {
                    Result returnedValue = executeBlock(code, &newMemory, newReturnSignal, forceStayInThread);
                    result = returnedValue.get();
                    if(thisObj) newMemory.unsafeSet(variableManager.thisId, nullptr, nullptr);
                    SET_RESULT;
                }
                break;
            } 
            else {
                auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if(context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    Result returnedValue = executeBlock(static_cast<Code*>(context), newMemory, newReturnSignal, forceStayInThread);
                    if(newReturnSignal) {
                        result = returnedValue.get();
                        SET_RESULT;
                        break;
                    }
                }
                //newMemory->detach(code->getDeclarationMemory());
                newMemory->detach(memory);
                auto it = memory->codeOwners.find(code);
                Data* thisObj = (it != memory->codeOwners.end() ? it->second->getMemory() : memory)->getOrNull(variableManager.thisId, true);
                if(thisObj) newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
                newMemory->allowMutables = false;
                newMemory->leak(); // for all transferred variables, make their reference counter thread safe
                auto futureResult = new ThreadResult();
                auto future = new Future(futureResult);
                memory->attached_threads.insert(future);
                //newMemory->attached_threads.insert(future);
                futureResult->start(code, newMemory, futureResult, command, thisObj);
                result = future;
            }
        } break;

        case RETURN: {
            result = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            //if(result)
            //    result->leak();
            returnSignal = true;
        } return;

        case GET: {
            BMemory* from(nullptr);
            result = memory->getOrNull(command->args[1], true);
            if(result==nullptr) {
                bbassert(command->args[1]==variableManager.thisId, "Missing value"+std::string(memory->size()?"":" in cleared memory ")+": " + variableManager.getSymbol(command->args[1]));
                from = memory;
                result = from->get(command->args[2]);
            }
            else {
                bbassert(result->getType() == STRUCT, "Can only get elements from structs");
                auto obj = static_cast<Struct*>(result);
                std::lock_guard<std::recursive_mutex> lock(obj->memoryLock);
                from = obj->getMemory();
                result = from->get(command->args[2]);
                if(result->getType() == CODE && from) {
                    static_cast<Code*>(result)->scheduleForParallelExecution = false;  // never execute struct calls in parallel
                    memory->codeOwners[static_cast<Code*>(result)] = obj;
                }
            }
        } break;
        case ISCACHED: {
            result = cachedData[command->args[1]];
            bbassert(result, "Missing cache value (typically cached due to optimization):" + variableManager.getSymbol(command->args[1]));
            break;
        }
        case IS: {
            result = command->knownLocal[1]?memory->getShallow(command->args[1]):memory->get(command->args[1], true);
            bbassert(result, "Missing value"+std::string(memory->size()?"":" in cleared memory ")+": " + variableManager.getSymbol(command->args[1]));
            if(result->getType()==ERRORTYPE) bberror(enrichErrorDescription(command, result->toString(memory)));
        } break;

        case AS: {
            result = memory->getOrNull(command->args[1], true);
            bbassert(result, "Missing value"+std::string(memory->size()?"":" in cleared memory ")+": " + variableManager.getSymbol(command->args[1]));
            if(result->getType()==ERRORTYPE) static_cast<BError*>(result)->consume();
        } break;

        case EXISTS: {
            Data* res = memory->getOrNull(command->args[1], true);
            result = (res?res->getType()!=ERRORTYPE:false)?::Boolean::valueTrue:Boolean::valueFalse;
        } break;

        case SET: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = static_cast<Struct*>(obj);
                std::lock_guard<std::recursive_mutex> lock(structObj->memoryLock);
            Data* setValue = memory->getOrNullShallow(command->args[3]);
            if(setValue && setValue->getType()==CODE) setValue = static_cast<Code*>(setValue)->copy();
            if(setValue) setValue->leak();
            auto structMemory = structObj->getMemory();
            structMemory->unsafeSet(memory, command->args[2], setValue, nullptr);//structMemory->getOrNullShallow(command->args[2]));
            result = nullptr;
        } break;

        case SETFINAL: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            bberror("Cannot set final fields in a struct using field access operators (. or \\). This also ensures that finals can only be set during `new` statements.");
            /*Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = static_cast<Code*>(obj);
            Data* setValue = memory->get(command->args[3]);
            if(setValue)
                setValue->leak();
            code->setMetadata(command->args[2], setValue);
            result = nullptr;*/
        } break;

        case WHILE: {
            // WE HAVE TWO OPTIONS: either the block-based mechanism or the inlined mechanism. Do not forget to change the parser too.
            #ifdef WHILE_WITH_CODE_BLOCKS
            Data* condition = memory->get(command->args[1]);
            Data* body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            bbassert(condition->getType() == CODE, "While condition can only be a code block.");
            auto codeBody = static_cast<Code*>(body);
            auto codeCondition = static_cast<Code*>(condition);
            
            bool checkValue;
            if(codeCondition->jitable && codeCondition->jitable->runWithBooleanIntent(memory, checkValue, forceStayInThread)){

            }
            else if(codeCondition->jitable && codeCondition->jitable->run(memory, result, returnSignal, forceStayInThread)){
                Data* check = result;
                if (returnSignal) {result = check;SET_RESULT;break;}
                bbassert(check, "Nothing was evaluated in while condition");
                bbassert(check->getType()==BB_BOOL, "Condition did not evaluate to a boolean");
                checkValue = check==Boolean::valueTrue;
            }
            else
            {
                Result returnedValue = executeBlock(codeCondition, memory, returnSignal, forceStayInThread);
                Data* check = returnedValue.get();
                if (returnSignal) {result = check;SET_RESULT;break;}
                bbassert(check, "Nothing was evaluated in while condition");
                bbassert(check->getType()==BB_BOOL, "Condition did not evaluate to a boolean");
                checkValue = check==Boolean::valueTrue;
            }
            while(checkValue) {
                if(codeBody->jitable && codeBody->jitable->run(memory, result, returnSignal, forceStayInThread)) {
                    if(returnSignal) {SET_RESULT;break;}
                }
                else {
                    Result returnedValue = executeBlock(codeBody, memory, returnSignal, forceStayInThread);
                    if (returnSignal) {result = returnedValue.get();SET_RESULT;break;}
                }
                if(codeCondition->jitable && codeCondition->jitable->runWithBooleanIntent(memory, checkValue, forceStayInThread)){
                }
                else 
                if(codeCondition->jitable && codeCondition->jitable->run(memory, result, returnSignal, forceStayInThread)) {
                    Data* check = result;
                    if (returnSignal) {result = check;SET_RESULT;break;}
                    bbassert(check, "Nothing was evaluated in while condition");
                    bbassert(check->getType()==BB_BOOL, "Condition did not evaluate to a boolean");
                    checkValue = check==Boolean::valueTrue;
                }
                else {
                    Result returnedValue = executeBlock(codeCondition, memory, returnSignal, forceStayInThread);
                    Data* check = returnedValue.get();
                    if (returnSignal) {result = check;SET_RESULT;break;}
                    bbassert(check, "Nothing was evaluated in while condition");
                    bbassert(check->getType()==BB_BOOL, "Condition did not evaluate to a boolean");
                    checkValue = check==Boolean::valueTrue;
                }
            }
            result = nullptr;
            if(returnSignal)
                break;
            #else
            Data* condition = memory->get(command->args[1]);
            Data* body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            auto codeBody = static_cast<Code*>(body);

            while (condition==Boolean::valueTrue) {
                if(codeBody->jitable && codeBody->jitable->run(memory, result, returnSignal)) {
                    if(returnSignal)
                        break;
                    condition = memory->get(command->args[1]);
                    continue;
                }
                Result returnedValue = executeBlock(codeBody, memory, returnSignal);
                if (returnSignal) {result = returnedValue.get();SET_RESULT;break;}
                condition = memory->get(command->args[1]);
            }
            result = nullptr;
            #endif
        } break;

        case IF: {
            Data* condition = memory->get(command->args[1]);
            Data* accept = memory->get(command->args[2]);
            Data* reject = command->nargs > 3 ? memory->get(command->args[3]) : nullptr;

            if(condition==Boolean::valueTrue) {
                if(accept->getType() == CODE) {
                    Code* code = static_cast<Code*>(accept);
                    if(code->jitable && code->jitable->run(memory, result, returnSignal, forceStayInThread)) return;
                    Result returnedValue = executeBlock(code, memory, returnSignal, forceStayInThread);
                    result = returnedValue.get();
                    SET_RESULT;
                }
                else {
                    result = accept;
                    SET_RESULT;
                }
            } 
            else if (reject && reject->getType() == CODE) {
                Code* code = static_cast<Code*>(reject);
                if(code->jitable && code->jitable->run(memory, result, returnSignal, forceStayInThread))  return;
                Result returnedValue = executeBlock(code, memory, returnSignal, forceStayInThread);
                result = returnedValue.get();
                SET_RESULT;
            }
            else {
                result = reject;
                SET_RESULT;
            }
        } break;

        case BB_PRINT: {
            std::string printing;
            for(int i = 1; i < command->nargs; i++) {
                Data* printable = MEMGET(memory, i);
                if(printable) {
                    std::string out = printable->toString(memory);
                    printing += out + " ";
                }
            }
            printing = replaceEscapeSequences(printing);
            printing += "\n";
            {
                std::lock_guard<std::recursive_mutex> lock(printMutex);
                std::cout << printing;
            } 
            result = nullptr;
        } return;

        case CREATESERVER: {
            Data* port = memory->get(command->args[1]);
            bbassert(port && port->getType()==BB_INT, "The server's port must be an integer.");
            auto res = new RestServer(static_cast<Integer*>(port)->getValue());
            res->runServer();
            result = res;
        } break;

        case READ:{
            std::string printing;
            for(int i=1;i<command->nargs;i++) {
                Data* printable = MEMGET(memory, i);
                if(printable) {
                    std::string out = printable->toString(memory);
                    printing += out+" ";
                }
            }
            {
                std::lock_guard<std::recursive_mutex> lock(printMutex);
                std::cout << printing;
                printing = "";
                std::getline(std::cin, printing);
                //std::cin >> printing;
            }
            result = new BString(printing);
        } break;

        case FINAL:
            // setting a memory content to final should alway be an attomic operation
            if(command->knownLocal[1]) 
                bberror("Cannot finalize a local variable (starting with _bb...)");
            memory->setFinal(command->args[1]);
            return;

        case END:
            return;

        case TRY: {
            memory->detach(memory->parent);
            try {
                Data* condition = MEMGET(memory, 1);
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for try condition");
                auto codeCondition = static_cast<Code*>(condition);
                bool tryReturnSignal(false);
                Result returnedValue = executeBlock(codeCondition, memory, tryReturnSignal, forceStayInThread);
                memory->detach(memory->parent);
                result = returnedValue.get();
                if(!tryReturnSignal) {
                    result = NO_TRY_INTERCEPT;
                }
                SET_RESULT;
            }
            catch (const BBError& e) {
                result = new BError(std::move(enrichErrorDescription(command, e.what())));
            }
        } break;
        
        case CATCH: {
            Data* condition = (command->knownLocal[1]?memory->getOrNullShallow(command->args[1]):memory->getOrNull(command->args[1], true)); //MEMGET(memory, 1);
            Data* accept = MEMGET(memory, 2);
            Data* reject = command->nargs>3?MEMGET(memory, 3):nullptr;
            bbverify(accept, !accept || accept->getType()==CODE, "Can only inline a code block for catch acceptance");
            bbverify(reject, !reject || reject->getType()==CODE, "Can only inline a code block for catch rejection");
            auto codeAccept = static_cast<Code*>(accept);
            auto codeReject = static_cast<Code*>(reject);
            
            if(condition && condition->getType()==ERRORTYPE) { //&& !((BError*)condition)->isConsumed()) {
                static_cast<BError*>(condition)->consume();
                if(codeAccept) {
                    Result returnValue = executeBlock(codeAccept, memory, returnSignal, forceStayInThread);
                    result = returnValue.get();
                    SET_RESULT;
                }
            }
            else if(codeReject) {
                Result returnValue = executeBlock(codeReject, memory, returnSignal, forceStayInThread);
                result = returnValue.get();
                SET_RESULT;
            }
        } break;

        case FAIL: {
            Data* result = MEMGET(memory, 1);
            bberror(std::move(enrichErrorDescription(command, result->toString(memory))));
        } break;

        case INLINE: {
            Data* source = MEMGET(memory, 1);
            /*if(source->getType()==FILETYPE) {
                if(command->value) {
                    auto code = static_cast<Code*>(command->value);
                    auto res = new Code(code->getProgram(), code->getStart(), code->getEnd(), nullptr, nullptr);//code->getAllMetadata());
                    res->jitable = jit(res);
                    result = res;
                }
                else {
                    Result returnValue = compileAndLoad(static_cast<BFile*>(source)->getPath(), nullptr);
                    result = returnValue.get();
                    command->value = result;
                    result->addOwner();
                    SET_RESULT;
                }
            }
            else */
            if(source->getType()==STRUCT) {
                memory->pull(static_cast<Struct*>(source)->getMemory());
                result = nullptr;
            }
            else if(source->getType()!=CODE) 
                bberror("Can only inline a code block or struct");
            else {
                auto code = static_cast<Code*>(source);
                Result returnedValue = executeBlock(code, memory, returnSignal, forceStayInThread);
                result = returnedValue.get();
                SET_RESULT;
            }
        } break;

        case DEFER: {
            Data* source = MEMGET(memory, 1);
            if(source->getType()!=CODE) 
                bberror("Finally can only inline a code block or struct");
            memory->addFinally(static_cast<Code*>(source));
            break;
        }

        case DEFAULT: {
            Data* source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `default` on a code block");
            auto code = static_cast<Code*>(source);
            BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            bool defaultReturnSignal(false);
            executeBlock(code, &newMemory, defaultReturnSignal, forceStayInThread);
            if(defaultReturnSignal)
                bberror("Cannot return from within a `default` statement");
            memory->replaceMissing(&newMemory);
        } break;

        case NEW: {
            Data* source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `new` on a code block");
            auto code = static_cast<Code*>(source);
            auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            auto thisObj = new Struct(newMemory); 
            newMemory->thisObject = thisObj;
            newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
            newMemory->setFinal(variableManager.thisId);
            bool newReturnSignal(false);
            Result returnedValue = executeBlock(code, newMemory, newReturnSignal, forceStayInThread);
            result = returnedValue.get();
            newMemory->detach(nullptr);
            newMemory->leak(); // TODO: investigate if this should be here or manually on getting and setting (here may be better to remove checks given that setting and getting are already slower)
            if(result!=thisObj) {
                if(command->args[0]!=variableManager.noneId) memory->unsafeSet(command->args[0], result, nullptr);
                thisObj->removeFromOwner(); // do this after setting
                return;
            }
            SET_RESULT;
        } break;

        case TOLIST: {
            auto list = new BList(command->nargs-1);
            for(int i=1;i<command->nargs;i++) {
                Data* element = MEMGET(memory, i);
                bbassert(element->getType()!=ERRORTYPE, "Cannot push an error to a list");
                element->addOwner();
                element->leak();
                list->contents.push_back(element);
            }
            result = list;
        } break;

        case TOMAP: if(command->nargs==1) {
            result = new BHashMap();
            break;
        }
        case TIME:
            result = new BFloat(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count());
        break;
        default: {
            int nargs = command->nargs;
            args.size = nargs - 1;
            if (nargs > 1) args.arg0 = MEMGET(memory, 1);
            if (nargs > 2) args.arg1 = MEMGET(memory, 2);
            if (nargs > 3) args.arg2 = MEMGET(memory, 3);
            Result returnValue = Data::run(command->operation, &args, memory);
            result = returnValue.get();
            SET_RESULT;
        } break;
    }
    //if(!result && command->knownLocal[0])
    //    bberror("Missing value encountered in intermediate computation "+variableManager.getSymbol(command->args[0])+". Explicitly use the `as` assignment to explicitly set potentially missing values\n");
    //Data* prevResult = command->knownLocal[0]?memory->getOrNullShallow(command->args[0]):memory->getOrNull(command->args[0], true)
    if(command->args[0]!=variableManager.noneId) 
        memory->unsafeSet(command->args[0], result, nullptr);
}
