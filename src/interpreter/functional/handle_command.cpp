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
#include "data/Iterator.h"
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
std::unordered_map<int, DataPtr> cachedData;

#define SET_RESULT if(command->args[0]!=variableManager.noneId) memory->set(command->args[0], result);return
#define SET_RESULT_LITERAL(expr) result=DataPtr(expr); memory->set(command->args[0], result); return;


void ExecutionInstance::handleCommand(int &i){
    Command* command = program[i];
    //DataPtr toReplace = command->nargs?memory->getOrNullShallow(command->args[0]):nullptr;
    //BMemory* memory = memory_.get();

    //std::cout<<command->toString()<<"\t "<<std::this_thread::get_id()<<"\n";
    
    switch (command->operation) {
        case ADD: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()+arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL((double)(arg0.unsafe_toint()+arg1.tofloat()));
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL((double)(arg0.unsafe_tofloat()+arg1.toint()));
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()+arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case SUB: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()-arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL((double)(arg0.unsafe_toint()-arg1.tofloat()));
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL((double)(arg0.unsafe_tofloat()-arg1.toint()));
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()-arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case MUL: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()*arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL((double)(arg0.unsafe_toint()*arg1.tofloat()));
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL((double)(arg0.unsafe_tofloat()*arg1.toint()));
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()*arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case DIV: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()/(double)arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL((double)(arg0.unsafe_toint()/arg1.tofloat()));
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL((double)(arg0.unsafe_tofloat()/arg1.toint()));
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()/arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case LT: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()<arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()<arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()<arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()<arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case GT: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()>arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()>arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()>arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()>arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case LE: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()<=arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()<=arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()<=arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()<=arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case GE: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()>=arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()>=arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()>=arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()>=arg1.tofloat());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case EQ: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()==arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()==arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()==arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()==arg1.tofloat());
            if(arg0.isbool() && arg1.isbool()) SET_RESULT_LITERAL(arg0.unsafe_tobool()==arg1.tobool());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case NEQ: {
            const auto& arg0 = MEMGET(memory, 1);
            const auto& arg1 = MEMGET(memory, 2);
            if(arg0.isint() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_toint()!=arg1.toint());
            if(arg0.isint() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_toint()!=arg1.tofloat());
            if(arg0.isfloat() && arg1.isint()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()!=arg1.toint());
            if(arg0.isfloat() && arg1.isfloat()) SET_RESULT_LITERAL(arg0.unsafe_tofloat()!=arg1.tofloat());
            if(arg0.isbool() && arg1.isbool()) SET_RESULT_LITERAL(arg0.unsafe_tobool()!=arg1.tobool());
            args.arg0 = arg0;
            args.arg1 = arg1;
            args.size = 2;
        }
        case TOSTR: {
            const auto& arg0 = MEMGET(memory, 1);
            if(arg0.islit()) {
                result = new BString(arg0.torepr());
                SET_RESULT;
            }
            args.arg0 = arg0;
            args.size = 1;
        }
        case TORANGE: {
            const auto& arg0 = MEMGET(memory, 1);
            if(arg0.isint()) {
                result = new IntRange(0, arg0.unsafe_toint(), 1);
                break;
            }
            args.arg0 = arg0;
        }
        case BUILTIN: result = command->value;break;
        case BEGINCACHE: {
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while(pos <= program.size()) {
                command_type = program[pos]->operation;
                if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
                if(command_type == END) {
                    if (depth == 0) break;
                    depth--;
                }
                pos++;
            }
            bbassert(depth >= 0, "Cache declaration never ended.");
            auto cache = new Code(&program, i + 1, pos);
            BMemory cacheMemory(nullptr, 16, nullptr);
            ExecutionInstance cacheExecutor(cache, &cacheMemory, forceStayInThread);
            cacheExecutor.run(cache);
            cacheMemory.await();
            bbassert(!cacheExecutor.hasReturned(), "Cache declaration cannot return a value");
            for (int key : cacheMemory.finals) {
                DataPtr obj = cacheMemory.get(key);
                bbassert(!obj.existsAndTypeEquals(STRUCT), "Structs cannot be cached");
                obj->addOwner();
                cachedData[key] = obj;
            }
            result = nullptr;
            return;
        }
        case BEGIN:
        case BEGINFINAL: {
            // Start a block of code
            if(command->value.exists()) {
                auto code = static_cast<Code*>(command->value.get());
                result = code;
                if (command->operation == BEGINFINAL) memory->setFinal(command->args[0]);
                i = code->getEnd();
                break;
            }
            // Find the matching END for this block
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while(pos <= program.size()) {
                command_type = program[pos]->operation;
                if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
                if(command_type == END) {
                    if(depth == 0) break;
                    depth--;
                }
                pos++;
            }
            bbassert(depth >= 0, "Code block never ended.");
            auto cache = new Code(&program, i + 1, pos);
            cache->addOwner();
            cache->jitable = jit(cache);
            command->value = cache;
            result = cache;
            if(command->operation == BEGINFINAL) memory->setFinal(command->args[0]);
            i = pos;
        } break;

        case CALL: {
            // Function or method call
            DataPtr context = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            DataPtr called = memory->get(command->args[2]);
            bbassert(called.exists(), "Cannot call a missing value or literal.");
            if(called->getType()!=CODE && called->getType()!=STRUCT) {
                args.size = 2;
                args.arg0 = called;
                args.arg1 = context;
                Result returnValue = Data::run(command->operation, &args, memory);
                result = returnValue.get();
                SET_RESULT;
            }
            if(called->getType()==STRUCT) {
                auto strct = static_cast<Struct*>(called.get());
                auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
                bbassert(val.existsAndTypeEquals(CODE), "Struct was called like a method but has no implemented code for `call`.");
                //static_cast<Code*>(val)->scheduleForParallelExecution = false; // struct calls are never executed in parallel
                memory->codeOwners[static_cast<Code*>(val.get())] = static_cast<Struct*>(called.get());
                called = (val);
            }
            auto code = static_cast<Code*>(called.get());
            if(forceStayInThread || !code->scheduleForParallelExecution || !Future::acceptsThread()) {
                BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                if(context.exists()) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    Code* callCode = static_cast<Code*>(context.get());
                    ExecutionInstance executor(callCode, &newMemory, forceStayInThread);
                    Result returnedValue = executor.run(callCode);
                    if(executor.hasReturned()) {
                        result = returnedValue.get();
                        SET_RESULT;
                        break;
                    }
                }
                auto it = memory->codeOwners.find(code);
                const auto& thisObj = (it != memory->codeOwners.end() ? it->second->getMemory() : memory)->getOrNull(variableManager.thisId, true);
                if(thisObj.exists()) newMemory.set(variableManager.thisId, thisObj);
                std::unique_lock<std::recursive_mutex> executorLock;
                if(thisObj.exists()) {
                    bbassert(thisObj->getType()==STRUCT, "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
                    //if(!forceStayInThread) 
                    executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj.get())->memoryLock);
                }
                newMemory.allowMutables = false;
                bool forceStayInThread = thisObj.exists(); // overwrite the option
                ExecutionInstance executor(code, &newMemory, forceStayInThread);
                newMemory.detach(nullptr);
                Result returnedValue = executor.run(code);
                result = returnedValue.get();
                if(thisObj.exists()) newMemory.set(variableManager.thisId, nullptr);
                SET_RESULT;
                break;
            } 
            else { // 
                auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                if(context.exists()) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    ExecutionInstance executor(static_cast<Code*>(context.get()), newMemory, forceStayInThread);
                    Result returnedValue = executor.run(static_cast<Code*>(context.get()));
                    if(executor.hasReturned()) {
                        result = returnedValue.get();
                        SET_RESULT;
                        break;
                    }
                }
                //newMemory->detach(memory);
                auto it = memory->codeOwners.find(code);
                const auto& thisObj = (it != memory->codeOwners.end() ? it->second->getMemory() : memory)->getOrNull(variableManager.thisId, true);
                if(thisObj.exists()) newMemory->set(variableManager.thisId, thisObj);
                newMemory->allowMutables = false;
                auto futureResult = new ThreadResult();
                auto future = new Future(futureResult);
                future->addOwner();//the attached_threads are also an owner
                memory->attached_threads.insert(future);
                futureResult->start(code, newMemory, futureResult, command, thisObj);
                result = future;
            }
        } break;

        case RETURN: {
            result = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            returnSignal = true;
        } return;

        case GET: {
            BMemory* from(nullptr);
            const auto& objFound = memory->getOrNull(command->args[1], true);
            if(!objFound.exists()) {
                bbassert(command->args[1]==variableManager.thisId, "Missing value"+std::string(memory->size()?"":" in cleared memory ")+": " + variableManager.getSymbol(command->args[1]));
                from = memory;
                result = from->get(command->args[2]);
            }
            else {
                bbassert(objFound.existsAndTypeEquals(STRUCT), "Can only get elements from structs, not"+std::to_string(objFound->getType()));
                auto obj = static_cast<Struct*>(objFound.get());
                std::lock_guard<std::recursive_mutex> lock(obj->memoryLock);
                from = obj->getMemory();
                result = from->get(command->args[2]);
                if(result.existsAndTypeEquals(CODE)) memory->codeOwners[static_cast<Code*>(result.get())] = obj;
            }
        } break;
        case ISCACHED: {
            result = cachedData[command->args[1]];
            bbassert(result.exists(), "Missing cache value (typically cached due to optimization):" + variableManager.getSymbol(command->args[1]));
            break;
        }
        case IS: {
            result = memory->get(command->args[1], true);
            if(result->getType()==ERRORTYPE) bberror(enrichErrorDescription(command, result->toString(memory)));
        } break;

        case AS: {
            result = memory->getOrNull(command->args[1], true);
            //bbassert(!result.islitorexists(), "Missing value"+std::string(memory->size()?"":" in cleared memory ")+": " + variableManager.getSymbol(command->args[1]));
            if(result.existsAndTypeEquals(ERRORTYPE)) static_cast<BError*>(result.get())->consume();
        } break;

        case EXISTS: {
            DataPtr res = memory->getOrNull(command->args[1], true);
            result = DataPtr(!res.existsAndTypeEquals(ERRORTYPE));
        } break;

        case SET: {
            DataPtr obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = static_cast<Struct*>(obj.get());
            std::lock_guard<std::recursive_mutex> lock(structObj->memoryLock);
            DataPtr setValue = memory->getOrNullShallow(command->args[3]);
            if(setValue.existsAndTypeEquals(CODE)) setValue = static_cast<Code*>(setValue.get())->copy();
            if(setValue.exists()) setValue->leak();
            auto structMemory = structObj->getMemory();
            structMemory->set(command->args[2], setValue);//structMemory->getOrNullShallow(command->args[2]));
            result = nullptr;
        } break;

        case SETFINAL: {
            DataPtr obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            bberror("Cannot set final fields in a struct using field access operators (. or \\). This also ensures that finals can only be set during `new` statements.");
            /*DataPtr obj = memory->get(command->args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = static_cast<Code*>(obj);
            DataPtr setValue = memory->get(command->args[3]);
            if(setValue)
                setValue->leak();
            code->setMetadata(command->args[2], setValue);
            result = nullptr;*/
        } break;

        case WHILE: {
            DataPtr condition = memory->get(command->args[1]);
            DataPtr body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            bbassert(condition->getType() == CODE, "While condition can only be a code block.");
            auto codeBody = static_cast<Code*>(body.get());
            auto codeCondition = static_cast<Code*>(condition.get());
            
            bool checkValue(true);
            while(checkValue) {
                Result returnedValue = run(codeCondition);
                DataPtr check = returnedValue.get();
                if (returnSignal) {result = check;SET_RESULT;break;}
                bbassert(check.isbool(), "While condition did not evaluate to bool");
                checkValue = check.unsafe_tobool();
                if(!checkValue) break;

                Result returnedValueFromBody = run(codeBody);
                if (returnSignal) {result = returnedValueFromBody.get();SET_RESULT;break;}
            }
            result = nullptr;
            if(returnSignal) break;
        } break;

        case IF: {
            DataPtr condition = memory->get(command->args[1]);
            DataPtr accept = memory->get(command->args[2]);
            DataPtr reject = command->nargs > 3 ? memory->get(command->args[3]) : nullptr;

            if(condition==Boolean::valueTrue) {
                if(accept->getType() == CODE) {
                    Code* code = static_cast<Code*>(accept.get());
                    Result returnedValue = run(code);
                    result = returnedValue.get();
                    SET_RESULT;
                }
                else {
                    result = accept;
                    SET_RESULT;
                }
            } 
            else if (reject.existsAndTypeEquals(CODE)) {
                Code* code = static_cast<Code*>(reject.get());
                Result returnedValue = run(code);
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
                DataPtr printable = MEMGET(memory, i);
                std::string out = printable.exists()?printable->toString(memory):printable.torepr();
                printing += out + " ";
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
            DataPtr port = memory->get(command->args[1]);
            bbassert(port.existsAndTypeEquals(BB_INT), "The server's port must be an integer.");
            auto res = new RestServer(static_cast<Integer*>(port.get())->getValue());
            res->runServer();
            result = res;
        } break;

        case READ:{
            std::string printing;
            for(int i=1;i<command->nargs;i++) {
                DataPtr printable = MEMGET(memory, i);
                if(printable.exists()) {
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
            bool prevReturnSignal = returnSignal;
            try {
                DataPtr condition = MEMGET(memory, 1);
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for try condition");
                auto codeCondition = static_cast<Code*>(condition.get());
                Result returnedValue = run(codeCondition);
                memory->detach(memory->parent);
                result = returnedValue.get();
                if(!returnSignal) result = NO_TRY_INTERCEPT;
                returnSignal = prevReturnSignal;
                SET_RESULT;
            }
            catch (const BBError& e) {
                result = new BError(std::move(enrichErrorDescription(command, e.what())));
                returnSignal = prevReturnSignal;
            }
        } break;
        
        case CATCH: {
            DataPtr condition = (command->knownLocal[1]?memory->getOrNullShallow(command->args[1]):memory->getOrNull(command->args[1], true)); //MEMGET(memory, 1);
            DataPtr accept = MEMGET(memory, 2);
            DataPtr reject = command->nargs>3?MEMGET(memory, 3):nullptr;
            bbverify(accept.exists(), !accept.exists() || accept->getType()==CODE, "Can only inline a code block for catch acceptance");
            bbverify(reject.exists(), !reject.exists() || reject->getType()==CODE, "Can only inline a code block for catch rejection");
            auto codeAccept = static_cast<Code*>(accept.get());
            auto codeReject = static_cast<Code*>(reject.get());
            
            if(condition.existsAndTypeEquals(ERRORTYPE)) { //&& !((BError*)condition)->isConsumed()) {
                static_cast<BError*>(condition.get())->consume();
                if(codeAccept) {
                    Result returnValue = run(codeAccept);
                    result = returnValue.get();
                    SET_RESULT;
                }
            }
            else if(codeReject) {
                Result returnValue = run(codeReject);
                result = returnValue.get();
                SET_RESULT;
            }
        } break;

        case FAIL: {
            DataPtr result = MEMGET(memory, 1);
            bberror(std::move(enrichErrorDescription(command, result->toString(memory))));
        } break;

        case INLINE: {
            DataPtr source = MEMGET(memory, 1);
            if(source->getType()==STRUCT) {
                memory->pull(static_cast<Struct*>(source.get())->getMemory());
                result = nullptr;
            }
            else if(source->getType()!=CODE) bberror("Can only inline a code block or struct");
            else {
                auto code = static_cast<Code*>(source.get());
                ExecutionInstance executor(code, memory, forceStayInThread);
                Result returnedValue = executor.run(code);
                result = returnedValue.get();
                SET_RESULT;
            }
        } break;

        case DEFER: {
            DataPtr source = MEMGET(memory, 1);
            if(source->getType()!=CODE) bberror("Finally can only inline a code block or struct");
            memory->addFinally(static_cast<Code*>(source.get()));
            break;
        }

        case DEFAULT: {
            DataPtr source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `default` on a code block");
            auto code = static_cast<Code*>(source.get());
            BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            ExecutionInstance executor(code, &newMemory, forceStayInThread);
            Result returnedValue = executor.run(code);
            if(executor.hasReturned())  bberror("Cannot return from within a `default` statement");
            memory->replaceMissing(&newMemory);
        } break;

        case NEW: {
            DataPtr source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `new` on a code block");
            auto code = static_cast<Code*>(source.get());
            auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            auto thisObj = new Struct(newMemory); 
            newMemory->set(variableManager.thisId, thisObj);
            newMemory->setFinal(variableManager.thisId);
            ExecutionInstance executor(code, newMemory, forceStayInThread);
            Result returnedValue = executor.run(code);
            newMemory->detach(nullptr);
            if(returnedValue.get().get()!=thisObj) {
                if(command->args[0]!=variableManager.noneId) memory->set(command->args[0], result);
                thisObj->removeFromOwner(); // do this after setting
                return;
            }
            SET_RESULT;
        } break;

        case TOLIST: {
            auto list = new BList(command->nargs-1);
            for(int i=1;i<command->nargs;i++) {
                DataPtr element = MEMGET(memory, i);
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
            if(command->operation==ADD || command->operation==SUB || command->operation==MUL || command->operation==DIV || command->operation==TOSTR 
            || command->operation==LT || command->operation==LE || command->operation==GT || command->operation==GE || command->operation==EQ || command->operation==NEQ) {
                // DO NOT RETRIEVE MEMORY AGAIN SINCE WE ALREADY TOOK CARE TO HELP THIS BLOCK AT THE BEGINNING WHERE WE WERE HANDLING LITERALS
            }
            else {
                int nargs = command->nargs;
                args.size = nargs - 1;
                if (nargs > 1) args.arg0 = MEMGET(memory, 1);
                if (nargs > 2) args.arg1 = MEMGET(memory, 2);
                if (nargs > 3) args.arg2 = MEMGET(memory, 3);
            }
            Result returnValue = Data::run(command->operation, &args, memory);
            result = returnValue.get();
            SET_RESULT;
        } break;
    }
    //if(!result && command->knownLocal[0])
    //    bberror("Missing value encountered in intermediate computation "+variableManager.getSymbol(command->args[0])+". Explicitly use the `as` assignment to explicitly set potentially missing values\n");
    //DataPtr prevResult = command->knownLocal[0]?memory->getOrNullShallow(command->args[0]):memory->getOrNull(command->args[0], true)
    if(command->args[0]!=variableManager.noneId) memory->set(command->args[0], result);
}
