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


std::chrono::steady_clock::time_point program_start;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;

#define SET_RESULT if(command->args[0]!=variableManager.noneId) memory->unsafeSet(command->args[0], result, nullptr);return


void handleCommand(std::vector<Command*>* program, int& i, BMemory* memory, bool &returnSignal, BuiltinArgs &args, Data*& result) {
    Command* command = program->at(i);
    //Data* toReplace = command->nargs?memory->getOrNullShallow(command->args[0]):nullptr;
    //BMemory* memory = memory_.get();

    //std::cout<<command->toString()<<"\t "<<std::this_thread::get_id()<<"\n";
    
    switch (command->operation) {
        case BUILTIN:
            result = command->value;
            break;
        case BEGIN:
        case BEGINCACHED:
        case BEGINFINAL: {
            // Start a block of code
            if (command->value) {
                auto code = static_cast<Code*>(command->value);
                auto val = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory, nullptr);//, code->getAllMetadata());
                val->scheduleForParallelExecution = code->scheduleForParallelExecution;
                val->jitable = code->jitable;
                result = val;
                if (command->operation == BEGINFINAL) 
                    memory->setFinal(command->args[0]);
                i = code->getEnd();
                break;
            }
            // Find the matching END for this block
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            bool scheduleForParallelExecution = true;
            while (pos <= program->size()) {
                command_type = program->at(pos)->operation;
                if (command_type == BEGIN || command_type == BEGINFINAL)
                    depth += 1;
                //if (command_type == WHILE)
                //    scheduleForParallelExecution = true;
                if (command_type == END) {
                    if (depth == 0)
                        break;
                    depth -= 1;
                }
                pos += 1;
            }
            bbassert(depth >= 0, "Code block never ended.");
            auto cache = new Code(program, i + 1, pos, memory);
            cache->addOwner();
            auto val = new Code(program, i + 1, pos, memory, nullptr);//cache->getAllMetadata());
            val->jitable = jit(val);
            cache->jitable = val->jitable;
            val->scheduleForParallelExecution = scheduleForParallelExecution;
            cache->scheduleForParallelExecution = scheduleForParallelExecution;
            command->value = cache;
            result = (val);
            if (command->operation == BEGINFINAL) {
                memory->setFinal(command->args[0]);
            }
            i = pos;
        } break;

        case CALL: {
            // Function or method call
            Data* context = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            Data* called = memory->get(command->args[2]);
            bbassert(called, "Cannot call a missing value.");
            bbassert(called->getType()==CODE || called->getType()==STRUCT, "Only structs or code blocks can be called.");
            if(called->getType()==STRUCT) {
                // struct calls are never executed in parallel
                auto strct = static_cast<Struct*>(called);
                auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
                bbassert(val && val->getType()==CODE, "Struct was called like a method but has no implemented code for `call`.");
                static_cast<Code*>(val)->setDeclarationMemory(static_cast<Struct*>(called)->getMemory());
                static_cast<Code*>(val)->scheduleForParallelExecution = false;
                called = (val);
            }
            auto code = static_cast<Code*>(called);
            if (!code->scheduleForParallelExecution || !Future::acceptsThread()) {
                BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    Result returnedValue = executeBlock(static_cast<Code*>(context), &newMemory, newReturnSignal);
                    if(newReturnSignal) {
                        result = returnedValue.get();
                        if(result && result->getType()==CODE)
                            static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                        SET_RESULT;
                        break;
                    }
                }
                //newMemory.detach(code->getDeclarationMemory());
                Data* thisObj = nullptr;
                if(code->getDeclarationMemory())
                    thisObj = code->getDeclarationMemory()->getOrNull(variableManager.thisId, true);
                if(thisObj)
                    newMemory.unsafeSet(variableManager.thisId, thisObj, nullptr);
                newMemory.allowMutables = false;
                //newMemory.leak(); (this is for testing only - we are not leaking any memory to other threads if we continue in the same thread, so no need to enable atomic reference counting)
                Result returnedValue = executeBlock(code, &newMemory, newReturnSignal);
                result = returnedValue.get();
                if(result && result->getType()==CODE)
                    static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                if(thisObj)
                    newMemory.unsafeSet(variableManager.thisId, nullptr, nullptr);
                SET_RESULT;
                break;
            } 
            else {
                auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    Result returnedValue = executeBlock(static_cast<Code*>(context), newMemory, newReturnSignal);
                    if(newReturnSignal) {
                        result = returnedValue.get();
                        if(result && result->getType()==CODE)
                            static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                        SET_RESULT;
                    }
                }
                if(newReturnSignal) {
                    if(result && result->getType()==CODE)
                        static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                    break;
                }
                //newMemory->detach(code->getDeclarationMemory());
                Data* thisObj = nullptr;
                if(code->getDeclarationMemory())
                    thisObj = code->getDeclarationMemory()->getOrNull(variableManager.thisId, true);
                if(thisObj)
                    newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
                newMemory->allowMutables = false;
                newMemory->leak(); // for all transferred variables, make their reference counter thread safe

                auto futureResult = new ThreadResult();
                auto future = new Future(futureResult);
                memory->attached_threads.insert(future);
                //newMemory->attached_threads.insert(future);
                futureResult->start(code, newMemory, futureResult, command);
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
            result = memory->get(command->args[1]);
            if (result->getType() == STRUCT) {
                auto obj = static_cast<Struct*>(result);
                result = obj->getMemory()->get(command->args[2]);
                if(result->getType() == CODE)
                    static_cast<Code*>(result)->setDeclarationMemory(obj->getMemory());
            }
            else if (result->getType() == CODE) {
                auto code = static_cast<Code*>(result);
                result = code->getMetadata(command->args[2]);
            } 
            else
                bberror("Can only get elements from struct or code");
        } break;

        case IS: {
            result = command->knownLocal[1]?memory->getShallow(command->args[1]):memory->get(command->args[1], true);
            bbassert(result, "Missing value: " + variableManager.getSymbol(command->args[1]));
            if(result->getType()==ERRORTYPE) {
                std::string comm = command->toString();
                comm.resize(40, ' ');
                bberror(static_cast<BError*>(result)->toString() + std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line));
            }
        } break;

        case AS: {
            result = memory->getOrNull(command->args[1], true);
            //result = command->knownLocal[1]?memory->getShallow(command->args[1]):memory->get(command->args[1], true);
            bbassert(result, "Missing value: " + variableManager.getSymbol(command->args[1]));
            if(result->getType()==ERRORTYPE) //{
                static_cast<BError*>(result)->consume();
                //result = new BError(true);
            //}
        } break;

        case EXISTS: {
            Data* res = memory->getOrNull(command->args[1], true);
            if(res) 
                result = new Boolean(res->getType()!=ERRORTYPE);
            else
                result = new Boolean(false);
            //bool exists = memory->contains(command->args[1]);
            //result = new Boolean(exists);
        } break;

        case SET: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = static_cast<Struct*>(obj);
            Data* setValue = memory->getOrNullShallow(command->args[3]);
            if(setValue)
                setValue->leak();
            auto structMemory = structObj->getMemory();
            structMemory->unsafeSet(memory, command->args[2], setValue, nullptr);//structMemory->getOrNullShallow(command->args[2]));
            result = nullptr;
        } break;

        case SETFINAL: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = static_cast<Code*>(obj);
            Data* setValue = memory->get(command->args[3]);
            if(setValue)
                setValue->leak();
            code->setMetadata(command->args[2], setValue);
            result = nullptr;
        } break;

        case WHILE: {
            Data* condition = memory->get(command->args[1]);
            Data* body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            auto codeBody = static_cast<Code*>(body);

            while (condition->isTrue()) {
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
        } break;

        case IF: {
            Data* condition = memory->get(command->args[1]);
            Data* accept = memory->get(command->args[2]);
            Data* reject = command->nargs > 3 ? memory->get(command->args[3]) : nullptr;

            if (condition->isTrue()) {
                if (accept->getType() == CODE) {

                    Code* code = static_cast<Code*>(accept);
                    if(code->jitable && code->jitable->run(memory, result, returnSignal)) 
                        return;

                    Result returnedValue = executeBlock(code, memory, returnSignal);
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
                if(code->jitable && code->jitable->run(memory, result, returnSignal)) 
                    return;
                Result returnedValue = executeBlock(code, memory, returnSignal);
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
                    std::string out = printable->toString();
                    printing += out + " ";
                }
            }
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
                    std::string out = printable->toString();
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
                Result returnedValue = executeBlock(codeCondition, memory, tryReturnSignal);
                memory->detach(memory->parent);
                result = returnedValue.get();
                if(!tryReturnSignal) {
                    //result = nullptr;
                    //std::string comm = command->toString();
                    //comm.resize(40, ' ');
                    std::string message = "No error or return statement intercepted with `try`.";//+ std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line);
                    BError* res = new BError(std::move(message));
                    res->consume();
                    result = res;
                }
                SET_RESULT;
            }
            catch (const BBError& e) {
                std::string comm = command->toString();
                comm.resize(40, ' ');
                std::string message = std::move(e.what()) + std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line);
                result = new BError(std::move(message));
                //handleExecutionError(program, i, e);
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
                    Result returnValue = executeBlock(codeAccept, memory, returnSignal);
                    result = returnValue.get();
                    SET_RESULT;
                }
            }
            else if(codeReject) {
                Result returnValue = executeBlock(codeReject, memory, returnSignal);
                result = returnValue.get();
                SET_RESULT;
            }
        } break;

        case FAIL: {
            Data* result = MEMGET(memory, 1);
            std::string comm = command->toString();
            comm.resize(40, ' ');
            throw BBError(result->toString()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
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
                Result returnedValue = executeBlock(code, memory, returnSignal);
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
            executeBlock(code, &newMemory, defaultReturnSignal);
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
            Result returnedValue = executeBlock(code, newMemory, newReturnSignal);
            result = returnedValue.get();
            // newMemory->detach(nullptr);
            newMemory->leak(); // TODO: investigate if this should be here or manually on getting and setting (here may be better to remove checks given that setting and getting are already slower)
            if(result!=thisObj) {
                if(result && result->getType()==CODE) 
                    static_cast<Code*>(result)->setDeclarationMemory(nullptr);
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
            if (nargs > 1) 
                args.arg0 = MEMGET(memory, 1);
            if (nargs > 2) 
                args.arg1 = MEMGET(memory, 2);
            if (nargs > 3) 
                args.arg2 = MEMGET(memory, 3);
            Result returnValue = Data::run(command->operation, &args);
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
