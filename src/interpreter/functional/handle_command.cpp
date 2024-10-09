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
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"


std::chrono::steady_clock::time_point program_start;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;


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
                auto val = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory, code->getAllMetadata());
                val->scheduleForParallelExecution = code->scheduleForParallelExecution;
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
            cache->setAsBuiltin();
            auto val = new Code(program, i + 1, pos, memory, cache->getAllMetadata());
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
                bbassert(val && val->getType()==CODE, "Struct was called like a method but has no implemented code for `\\call`.");
                called = (val);
            }
            auto code = static_cast<Code*>(called);
            if (!code->scheduleForParallelExecution || !Future::acceptsThread()) {
                BMemory newMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(static_cast<Code*>(context), &newMemory, newReturnSignal);
                }
                if(newReturnSignal) {
                    if(result && result->getType()==CODE)
                        static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                    break;
                }
                newMemory.detach(code->getDeclarationMemory());
                result = executeBlock(code, &newMemory, newReturnSignal);
                if(result && result->getType()==CODE)
                    static_cast<Code*>(result)->setDeclarationMemory(nullptr);
            } 
            else {
                auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(static_cast<Code*>(context), newMemory, newReturnSignal);
                }
                if(newReturnSignal) {
                    if(result && result->getType()==CODE)
                        static_cast<Code*>(result)->setDeclarationMemory(nullptr);
                    break;
                }
                newMemory->detach(code->getDeclarationMemory());

                auto futureResult = new ThreadResult();
                auto future = new Future(futureResult);
                futureResult->start(code, newMemory, futureResult, command);
                memory->attached_threads.insert(future);
                result = future;
            }
        } break;

        case RETURN: {
            result = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            returnSignal = true;
        } break;

        case GET: {
            result = memory->get(command->args[1]);
            if (result->getType() == CODE) {
                auto code = static_cast<Code*>(result);
                result = code->getMetadata(command->args[2]);
            } else if (result->getType() == STRUCT) {
                auto obj = static_cast<Struct*>(result);
                result = obj->getMemory()->get(command->args[2]);
            }
        } break;

        case IS: {
            result = memory->get(command->args[1], true);
            bbassert(result, "Missing value: " + variableManager.getSymbol(command->args[1]));
        } break;

        case AS: {
            result = memory->getOrNull(command->args[1], true);
        } break;

        case EXISTS: {
            bool exists = memory->contains(command->args[1]);
            result = new Boolean(exists);
        } break;

        case SET: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = static_cast<Struct*>(obj);
            Data* setValue = memory->getOrNullShallow(command->args[3]);
            auto structMemory = structObj->getMemory();
            structMemory->unsafeSet(memory, command->args[2], setValue, nullptr);//structMemory->getOrNullShallow(command->args[2]));
        } break;

        case SETFINAL: {
            Data* obj = memory->get(command->args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = static_cast<Code*>(obj);
            Data* setValue = memory->get(command->args[3]);
            code->setMetadata(command->args[2], setValue);
        } break;

        case WHILE: {
            Data* condition = memory->get(command->args[1]);
            Data* body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            auto codeBody = static_cast<Code*>(body);

            while (condition->isTrue()) {
                result = executeBlock(codeBody, memory, returnSignal);
                if (returnSignal) break;
                condition = memory->get(command->args[1]);
            }
        } break;

        case IF: {
            Data* condition = memory->get(command->args[1]);
            Data* accept = memory->get(command->args[2]);
            Data* reject = command->nargs > 3 ? memory->get(command->args[3]) : nullptr;

            if (condition->isTrue()) {
                if (accept->getType() == CODE) {
                    result = executeBlock(static_cast<Code*>(accept), memory, returnSignal);
                }
            } else if (reject && reject->getType() == CODE) {
                result = executeBlock(static_cast<Code*>(reject), memory, returnSignal);
            }
        } break;

        case PRBB_INT: {
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
            bbassert(port->getType()==BB_INT, "The server's port must be an integer.");
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
                std::cin >> printing;
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
            try {
                Data* condition = MEMGET(memory, 1);
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for try condition");
                auto codeCondition = static_cast<Code*>(condition);
                bool tryReturnSignal(false);
                result = executeBlock(codeCondition, memory, tryReturnSignal);
                if(!tryReturnSignal)
                    result = nullptr;
            }
            catch (const BBError& e) {
                result = new BError(e.what());
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
                if(codeAccept) 
                    result = executeBlock(codeAccept, memory, returnSignal);
            }
            else if(codeReject) 
                result = executeBlock(codeReject, memory, returnSignal);
        } break;

        case FAIL: {
            Data* result = MEMGET(memory, 1);
            std::string comm = command->toString();
            comm.resize(40, ' ');
            throw BBError(result->toString()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        } break;

        case INLINE: {
            Data* source = MEMGET(memory, 1);
            if(source->getType()==FILETYPE) {
                if(command->value) {
                    auto code = static_cast<Code*>(command->value);
                    result = new Code(code->getProgram(), code->getStart(), code->getEnd(), nullptr, code->getAllMetadata());
                }
                else {
                    result = compileAndLoad(static_cast<BFile*>(source)->getPath(), nullptr);
                    command->value = result;
                }
            }
            else if(source->getType()==STRUCT) 
                memory->pull(static_cast<Struct*>(source)->getMemory());
            else if(source->getType()!=CODE) 
                bberror("Can only inline a non-called code block or struct");
            else {
                auto code = static_cast<Code*>(source);
                result = executeBlock(code, memory, returnSignal);
            }
        } break;

        case DEFAULT: {
            Data* source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `default` on a code block");
            auto code = static_cast<Code*>(source);
            auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            bool defaultReturnSignal(false);
            executeBlock(code, newMemory, defaultReturnSignal);
            if(defaultReturnSignal)
                bberror("Cannot return from within a `default` statement");
            memory->replaceMissing(newMemory);
        } break;

        case NEW: {
            Data* source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `new` on a code block");
            auto code = static_cast<Code*>(source);
            auto newMemory = new BMemory(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            auto thisObj = new Struct(newMemory); 
            newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
            newMemory->setFinal(variableManager.thisId);
            bool newReturnSignal(false);
            result = executeBlock(code, newMemory, newReturnSignal);
            if(result && result->getType()==CODE) 
                static_cast<Code*>(result)->setDeclarationMemory(nullptr);
            newMemory->detach();
        } break;

        case TOLIST: {
            auto list = new BList(command->nargs-1);
            for(int i=1;i<command->nargs;i++) {
                Data* element = MEMGET(memory, i);
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
            result = Data::run(command->operation, &args);
        } break;
    }
    //if(!result && command->knownLocal[0])
    //    bberror("Missing value encountered in intermediate computation "+variableManager.getSymbol(command->args[0])+". Explicitly use the `as` assignment to explicitly set potentially missing values\n");
    memory->unsafeSet(command->args[0], result, nullptr);
}
