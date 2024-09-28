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
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"


std::chrono::steady_clock::time_point program_start;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;


void handleCommand(const std::shared_ptr<std::vector<Command*>>& program,
                    int& i,
                    const std::shared_ptr<BMemory>& memory,
                    bool &returnSignal, 
                    BuiltinArgs &args,
                    std::shared_ptr<Data>& result) {
    Command* command = program->at(i);
    std::shared_ptr<Data> toReplace = command->nargs?memory->getOrNullShallow(command->args[0]):nullptr;
    //BMemory* memory = memory_.get();

    //std::cout<<command->toString()<<"\n";
    
    switch (command->operation) {
        case BUILTIN:
            result = command->value;
            break;
        case BEGIN:
        case BEGINCACHED:
        case BEGINFINAL: {
            // Start a block of code
            toReplace = memory->getOrNullShallow(command->args[0]);
            if (command->value) {
                auto code = std::static_pointer_cast<Code>(command->value);
                auto val = std::make_shared<Code>(code->getProgram(), code->getStart(), code->getEnd(), memory, code->getAllMetadata());
                val->scheduleForParallelExecution = code->scheduleForParallelExecution;
                result = std::move(val);
                if (command->operation == BEGINFINAL) 
                    memory->setFinal(command->args[0]);
                i = code->getEnd();
                break;
            }
            // Find the matching END for this block
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while (pos <= program->size()) {
                command_type = program->at(pos)->operation;
                if (command_type == BEGIN || command_type == BEGINFINAL)
                    depth += 1;
                if (command_type == END) {
                    if (depth == 0)
                        break;
                    depth -= 1;
                }
                pos += 1;
            }
            bbassert(depth >= 0, "Code block never ended.");
            auto cache = std::make_shared<Code>(program, i + 1, pos, nullptr);
            auto val = std::make_shared<Code>(program, i + 1, pos, memory, cache->getAllMetadata());
            val->scheduleForParallelExecution = true;
            cache->scheduleForParallelExecution = true;
            cache->isDestroyable = false;
            command->value = cache;
            result = std::move(val);
            if (command->operation == BEGINFINAL) {
                memory->setFinal(command->args[0]);
            }
            i = pos;
        } break;

        case CALL: {
            // Function or method call
            std::shared_ptr<Data> context = command->args[1] == variableManager.noneId ? nullptr : memory->get(command->args[1]);
            std::shared_ptr<Data> called = memory->get(command->args[2]);
            bbassert(called, "Cannot call a missing value.");
            bbassert(called->getType()==CODE, "Only structs or code blocks can be called.");
            auto code = std::static_pointer_cast<Code>(called);
            if (true || !code->scheduleForParallelExecution || !Future::acceptsThread()) {
                auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(std::static_pointer_cast<Code>(context), newMemory, newReturnSignal, args);
                }
                if(newReturnSignal)
                    break;
                newMemory->detach(code->getDeclarationMemory());
                result = executeBlock(code, std::move(newMemory), newReturnSignal, args);
                SCOPY(result);
                newMemory.reset();
            } else {
                auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                bool newReturnSignal(false);
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(std::static_pointer_cast<Code>(context), newMemory, newReturnSignal, args);
                }
                if(newReturnSignal)
                    break;
                newMemory->detach(code->getDeclarationMemory());

                auto futureResult = std::make_shared<ThreadResult>();
                auto future = std::make_shared<Future>(futureResult);
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
                auto code = std::static_pointer_cast<Code>(result);
                result = code->getMetadata(command->args[2]);
                SCOPY(result);
            } else if (result->getType() == STRUCT) {
                auto obj = std::static_pointer_cast<Struct>(result);
                result = obj->getMemory()->get(command->args[2]);
                SCOPY(result);
            }
        } break;

        case IS: {
            result = memory->getOrNull(command->args[1], true);
            //SCOPY(result);
            if(result)
                result = result->shallowCopy();
        } break;

        case EXISTS: {
            bool exists = memory->contains(command->args[1]);
            result = std::make_shared<Boolean>(exists);
        } break;

        case SET: {
            std::shared_ptr<Data> obj = memory->get(command->args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = std::static_pointer_cast<Struct>(obj);
            std::shared_ptr<Data> setValue = memory->get(command->args[3]);
            SCOPY(setValue);
            structObj->getMemory()->unsafeSet(command->args[2], setValue, memory->get(command->args[2]));
        } break;

        case SETFINAL: {
            std::shared_ptr<Data> obj = memory->get(command->args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = std::static_pointer_cast<Code>(obj);
            std::shared_ptr<Data> setValue = memory->get(command->args[3]);
            SCOPY(setValue);
            code->setMetadata(command->args[2], setValue);
        } break;

        case WHILE: {
            std::shared_ptr<Data> condition = memory->get(command->args[1]);
            std::shared_ptr<Data> body = memory->get(command->args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            auto codeBody = std::static_pointer_cast<Code>(body);

            while (condition->isTrue()) {
                result = executeBlock(codeBody, memory, returnSignal, args);
                if (returnSignal) break;
                condition = memory->get(command->args[1]);
            }
        } break;

        case IF: {
            std::shared_ptr<Data> condition = memory->get(command->args[1]);
            std::shared_ptr<Data> accept = memory->get(command->args[2]);
            std::shared_ptr<Data> reject = command->nargs > 3 ? memory->get(command->args[3]) : nullptr;

            if (condition->isTrue()) {
                if (accept->getType() == CODE) {
                    result = executeBlock(std::static_pointer_cast<Code>(accept), memory, returnSignal, args);
                }
            } else if (reject && reject->getType() == CODE) {
                result = executeBlock(std::static_pointer_cast<Code>(reject), memory, returnSignal, args);
            }
        } break;

        case PRINT: {
            std::string printing;
            for(int i = 1; i < command->nargs; i++) {
                std::shared_ptr<Data> printable = MEMGET(memory, i);
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

        case READ:{
            std::string printing;
            for(int i=1;i<command->nargs;i++) {
                std::shared_ptr<Data> printable = MEMGET(memory, i);
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
            result = std::make_shared<BString>(printing);
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
                std::shared_ptr<Data> condition = MEMGET(memory, 1);
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for try condition");
                auto codeCondition = std::static_pointer_cast<Code>(condition);
                bool tryReturnSignal(false);
                result = executeBlock(codeCondition, memory, returnSignal, args);
                if(!tryReturnSignal)
                    result = nullptr;
            }
            catch (const BBError& e) {
                result = std::make_shared<BError>(e.what());
                //handleExecutionError(program, i, e);
            }
        } break;
        
        case CATCH: {
            std::shared_ptr<Data> condition = (command->knownLocal[1]?memory->getOrNullShallow(command->args[1]):memory->getOrNull(command->args[1], true)); //MEMGET(memory, 1);
            std::shared_ptr<Data> accept = MEMGET(memory, 2);
            std::shared_ptr<Data> reject = command->nargs>3?MEMGET(memory, 3):nullptr;
            bbverify(accept, !accept || accept->getType()==CODE, "Can only inline a code block for catch acceptance");
            bbverify(reject, !reject || reject->getType()==CODE, "Can only inline a code block for catch rejection");
            auto codeAccept = std::static_pointer_cast<Code>(accept);
            auto codeReject = std::static_pointer_cast<Code>(reject);

            if(condition && condition->getType()==ERRORTYPE) { //&& !((BError*)condition)->isConsumed()) {
                std::static_pointer_cast<BError>(condition)->consume();
                if(codeAccept) 
                    result = executeBlock(codeAccept, memory, returnSignal, args);
            }
            else if(codeReject) 
                result = executeBlock(codeReject, memory, returnSignal, args);
        } break;

        case FAIL: {
            std::shared_ptr<Data> result = MEMGET(memory, 1);
            std::string comm = command->toString();
            comm.resize(40, ' ');
            throw BBError(result->toString()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        } break;

        case INLINE: {
            std::shared_ptr<Data> source = MEMGET(memory, 1);
            if(source->getType()==FILETYPE) {
                if(command->value) {
                    auto code = std::static_pointer_cast<Code>(command->value);
                    result = std::make_shared<Code>(code->getProgram(), code->getStart(), code->getEnd(), nullptr, code->getAllMetadata());
                }
                else {
                    result = compileAndLoad(std::static_pointer_cast<BFile>(source)->getPath(), nullptr);
                    command->value = result;
                    command->value->isDestroyable = false;
                }
            }
            else if(source->getType()==STRUCT) 
                memory->pull(std::static_pointer_cast<Struct>(source)->getMemory());
            else if(source->getType()!=CODE) 
                bberror("Can only inline a non-called code block or struct");
            else {
                auto code = std::static_pointer_cast<Code>(source);
                result = executeBlock(code, memory, returnSignal, args);
            }
            // SCOPY(result);
        } break;

        case DEFAULT: {
            std::shared_ptr<Data> source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `default` on a code block");
            auto code = std::static_pointer_cast<Code>(source);
            auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            bool defaultReturnSignal(false);
            //result = executeBlock(code, memory, defaultReturnSignal, args);
            executeBlock(code, newMemory, defaultReturnSignal, args);
            if(defaultReturnSignal)
                bberror("Cannot return from within a `default` statement");
            //newMemory->detach();
            memory->replaceMissing(newMemory);
            newMemory.reset();
        } break;

        case NEW: {
            std::shared_ptr<Data> source = MEMGET(memory, 1);
            bbassert(source->getType()==CODE, "Can only call `new` on a code block");
            auto code = std::static_pointer_cast<Code>(source);
            auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
            auto thisObj = std::make_shared<WeakStruct>(newMemory);  // this MUST be weak to let the memory destroy itself. it will be converted to a strong version with a shallow copy after being returned
            newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
            newMemory->setFinal(variableManager.thisId);
            bool newReturnSignal(false);
            result = executeBlock(code, newMemory, newReturnSignal, args);
            if(result) {
                SCOPY(result);
                if(result->getType()==CODE) 
                    std::static_pointer_cast<Code>(result)->setDeclarationMemory(nullptr);
            }
            newMemory->detach();
        } break;

        case TOLIST: {
            auto list = std::make_shared<BList>(command->nargs-1);
            for(int i=1;i<command->nargs;i++) {
                std::shared_ptr<Data> element = MEMGET(memory, i);
                SCOPY(element);
                list->contents->push_back(element);
            }
            result = list;
        } break;

        case TOMAP: if(command->nargs==1) {
            result = std::make_shared<BHashMap>();
            break;
        }
        case TIME:
            result = std::make_shared<BFloat>(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count());
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
            if(toReplace && toReplace->isDestroyable && (command->knownLocal[0] || !memory->isFinal(command->args[0])))
                args.preallocResult = toReplace;
            else
                args.preallocResult = nullptr;
            result = Data::run(command->operation, &args);
        } break;
    }
    memory->unsafeSet(command->args[0], result, toReplace);
}
