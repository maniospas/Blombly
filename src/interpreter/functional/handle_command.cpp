#include <iostream>
#include "common.h"
#include <memory>

#include "data/Struct.h"
#include "data/Code.h"
#include "data/Boolean.h"
#include "data/Future.h"
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"


std::shared_ptr<Data> handleCommand(const std::shared_ptr<std::vector<Command>>& program,
                    int& i,
                    const std::shared_ptr<BMemory>& memory,
                    bool &returnSignal, 
                    BuiltinArgs &args) {
    Command command = program->at(i);
    std::shared_ptr<Data> result = nullptr;
    std::shared_ptr<Data> toReplace = nullptr;
    //BMemory* memory = memory_.get();

    switch (command.operation) {
        case BEGIN:
        case BEGINCACHED:
        case BEGINFINAL: {
            // Start a block of code
            toReplace = memory->getOrNullShallow(command.args[0]);
            if (command.value) {
                auto code = std::dynamic_pointer_cast<Code>(command.value);
                auto val = std::make_shared<Code>(code->getProgram(), code->getStart(), code->getEnd(), memory, code->getAllMetadata());
                val->scheduleForParallelExecution = code->scheduleForParallelExecution;
                result = std::move(val);
                if (command.operation == BEGINFINAL) 
                    memory->setFinal(command.args[0]);
                i = code->getEnd();
                break;
            }
            // Find the matching END for this block
            int pos = i + 1;
            int depth = 0;
            OperationType command_type;
            while (pos <= program->size()) {
                command_type = program->at(pos).operation;
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
            command.value = cache;
            result = std::move(val);
            if (command.operation == BEGINFINAL) {
                memory->setFinal(command.args[0]);
            }
            i = pos;
        } break;

        case CALL: {
            // Function or method call
            std::shared_ptr<Data> context = command.args[1] == variableManager.noneId ? nullptr : memory->get(command.args[1]);
            std::shared_ptr<Data> called = memory->get(command.args[2]);
            bbassert(called, "Cannot call a missing value.");
            auto code = std::dynamic_pointer_cast<Code>(called);
            bbassert(code, "Only structs or code blocks can be called.");
            if (!code->scheduleForParallelExecution || !Future::acceptsThread()) {
                auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(std::dynamic_pointer_cast<Code>(context), newMemory, returnSignal, args);
                }
                newMemory->detach(code->getDeclarationMemory());
                result = executeBlock(code, std::move(newMemory), returnSignal, args);
                if (result)
                    result = result->shallowCopy();
            } else {
                auto newMemory = std::make_shared<BMemory>(memory, LOCAL_EXPECTATION_FROM_CODE(code));
                if (context) {
                    bbassert(context->getType() == CODE, "Call context must be a code block.");
                    result = executeBlock(std::dynamic_pointer_cast<Code>(context), newMemory, returnSignal, args);
                }
                newMemory->detach(code->getDeclarationMemory());

                auto futureResult = std::make_shared<ThreadResult>();
                auto future = std::make_shared<Future>(futureResult);
                futureResult->start(code, newMemory, futureResult, command);
                memory->attached_threads.insert(future);
                result = future;
            }
        } break;

        case RETURN: {
            result = command.args[1] == variableManager.noneId ? nullptr : memory->get(command.args[1]);
            returnSignal = true;
        } break;

        case GET: {
            result = memory->get(command.args[1]);
            if (result->getType() == CODE) {
                auto code = std::dynamic_pointer_cast<Code>(result);
                result = code->getMetadata(command.args[2]);
                if (result) result = result->shallowCopy();
            } else if (result->getType() == STRUCT) {
                auto obj = std::static_pointer_cast<Struct>(result);
                result = obj->getMemory()->get(command.args[2]);
                if (result) result = result->shallowCopy();
            }
        } break;

        case IS: {
            result = memory->getOrNull(command.args[1], true);
            if (result) result = result->shallowCopy();
        } break;

        case EXISTS: {
            bool exists = memory->contains(command.args[1]);
            result = std::make_shared<Boolean>(exists);
        } break;

        case SET: {
            std::shared_ptr<Data> obj = memory->get(command.args[1]);
            bbassert(obj->getType() == STRUCT, "Can only set fields in a struct.");
            auto structObj = std::static_pointer_cast<Struct>(obj);
            std::shared_ptr<Data> setValue = memory->get(command.args[3]);
            if(setValue)
                setValue = setValue->shallowCopy();
            structObj->getMemory()->unsafeSet(command.args[2], setValue, memory->get(command.args[2]));
        } break;

        case SETFINAL: {
            std::shared_ptr<Data> obj = memory->get(command.args[1]);
            bbassert(obj->getType() == CODE, "Can only set metadata for code blocks.");
            auto code = std::static_pointer_cast<Code>(obj);
            std::shared_ptr<Data> setValue = memory->get(command.args[3]);
            if(setValue)
                setValue = setValue->shallowCopy();
            code->setMetadata(command.args[2], setValue);
        } break;

        case WHILE: {
            std::shared_ptr<Data> condition = memory->get(command.args[1]);
            std::shared_ptr<Data> body = memory->get(command.args[2]);
            bbassert(body->getType() == CODE, "While body can only be a code block.");
            auto codeBody = std::dynamic_pointer_cast<Code>(body);

            while (condition->isTrue()) {
                result = executeBlock(codeBody, memory, returnSignal, args);
                if (returnSignal) break;
                condition = memory->get(command.args[1]);
            }
        } break;

        case IF: {
            std::shared_ptr<Data> condition = memory->get(command.args[1]);
            std::shared_ptr<Data> accept = memory->get(command.args[2]);
            std::shared_ptr<Data> reject = command.nargs > 3 ? memory->get(command.args[3]) : nullptr;

            if (condition->isTrue()) {
                if (accept->getType() == CODE) {
                    result = executeBlock(std::static_pointer_cast<Code>(accept), memory, returnSignal, args);
                }
            } else if (reject && reject->getType() == CODE) {
                result = executeBlock(std::static_pointer_cast<Code>(reject), memory, returnSignal, args);
            }
        } break;

        case PRINT: {
            std::string output;
            for (int j = 1; j < command.nargs; ++j) {
                std::shared_ptr<Data> value = memory->get(command.args[j]);
                output += value->toString() + " ";
            }
            std::cout << output << std::endl;
        } break;

        case END:
            return result;

        default: {
            args.size = command.nargs - 1;
            if (command.nargs > 1) {
                args.arg0 = memory->get(command.args[1]);
            }
            if (command.nargs > 2) {
                args.arg1 = memory->get(command.args[2]);
            }
            if (command.nargs > 3) {
                args.arg2 = memory->get(command.args[3]);
            }
            
            result = Data::run(command.operation, args);
        } break;
    }

    memory->unsafeSet(command.args[0], result, toReplace);
    return result;
}
