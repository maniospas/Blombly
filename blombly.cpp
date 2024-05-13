/**
 * This file implements the blombly language compiler and virtual machine
 * to run the compiled program.
 * Produce an executable and run one of the following two file formats:
 *  * blombly file.bb
 *  * blombly file.bbvm
 * The .bb format the progamming language code, which is converted to the
 * .bbvm format, which is then run. The .bbvm format is the assembly language
 * of the blombly virtual machine.
 * 
 * Author: Emmanouil (Manios) Krasanakis
 * Email: maniospas@hotmail.com
 * License: Apache 2.0
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <map>
#include <sstream>
#include <memory> 
#include <cstdlib>
#include <algorithm>
#include <unordered_set>  
#include <pthread.h>
#include <queue>
#include <atomic>
#include <functional>
#include <thread>
#include <cmath>
#include <future>
#include <ctime>
#include <ratio>
#include <chrono>
#include "utils/stringtrim.cpp"
#include "utils/parser.cpp"
#include "utils/optimizer.cpp"

// include data types
#include "include/common.h"
#include "include/data/Boolean.h"
#include "include/data/Integer.h"
#include "include/data/BFloat.h"
#include "include/data/Vector.h"
#include "include/data/BString.h"
#include "include/data/List.h"
#include "include/data/Future.h"
#include "include/data/Code.h"
#include "include/data/Struct.h"
#include "include/BMemory.h"
#include "include/interpreter/Command.h"
#include "include/interpreter/thread.h"

std::chrono::steady_clock::time_point program_start;
//#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getLocal(command->args[arg]):memory->get(command->args[arg]))

#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getOrNullShallow(command->args[arg]):memory->get(command->args[arg]))

#define CHECK_FOR_RETURN(expr) if(*returnSignal) { \
                        if(returnSignalHandler){ \
                            memory->release(); \
                            delete returnSignal; \
                            delete args; \
                        } \
                        return expr; \
                    }
#define FILL_REPLACEMENT toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0])


pthread_mutex_t printLock; 
Data* executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory_,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins
                  ) {
    /**
     * Executes a block of code from within a list of commands.
     * @param program The list of command pointers.
     * @param start The position from which to start the interpreting.
     * @param end The position at which to stop the interpreting (inclusive).
     * @param memory A pointer to the read and write memory.
     * @param returnSignal A pointer showing whether the block called a return internally 
     *  (so the returned value should break execution). Should be nullptr for new method calls.
     * @return A Data shared pointer holding the code block's outcome. This is nullptr if nothing is returned.
    */
    //std::cout << memory_.use_count()<<" entered\n";
    BMemory* memory = memory_.get();
    //std::cout << memory<<" "<<memory_.use_count()<<" enteted count\n";
    bool returnSignalHandler = false;
    BuiltinArgs* args = allocatedBuiltins;
    if(!returnSignal) {
        returnSignal = new bool(false);
        returnSignalHandler = true;
        args = new BuiltinArgs();
    }
    //std::shared_ptr<Data> prevValue;
    int cmdSize;
    Data* value;
    for(int i=start;i<=end;i++) {
        Command* command = program->at(i);
        Data* toReplace;
        //std::cout << "COMMAND "<<memory_.use_count()<<" "<<command->operation<<getOperationTypeName(command->operation)<<"\n";
        switch(command->operation) {
            case BUILTIN:
                FILL_REPLACEMENT;
                if(command->value==toReplace)
                    continue;
                value = command->value;
            break;
            case FINAL:
                // setting a memory content to final should alway be an attomic operation
                if(command->knownLocal[1]) {
                    std::cerr << "Cannot finalize a local variable (starting with _bb...)"<<std::endl;
                    exit(1);
                }
                value = MEMGET(memory, 1);
                memory->lock();
                value->isMutable = false;
                memory->unlock();
                continue;
            break;
            case PRINT:{
                std::string printing("");
                for(int i=1;i<command->nargs;i++) {
                    Data* printable = MEMGET(memory, i);
                    if(printable) {
                        std::string out = printable->toString();
                        printing += out+" ";
                    }
                }
                printing += "\n";
                pthread_mutex_lock(&printLock); 
                std::cout << printing;
                pthread_mutex_unlock(&printLock);
                value = nullptr;
                continue;
            }
            break;
            case BEGIN:
            case BEGINFINAL: {
                if(command->value) {
                    Code* code = (Code*)command->value;
                    Code* codeCommand = (Code*)command->value;
                    if(command->operation==BEGINFINAL 
                        && code->getProgram()==codeCommand->getProgram()
                        && code->getStart()==codeCommand->getStart()
                        && code->getEnd()==codeCommand->getEnd()
                        && code->getDeclarationMemory().get()==memory_.get()) {
                        continue;
                    }
                    else {
                        toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
                        value = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory_);
                        value->isMutable = command->operation!=BEGINFINAL;
                    }
                    i = code->getEnd();
                    break;
                }
                int pos = i+1;
                int depth = 0;
                OperationType command_type;
                while(pos<=end) {
                    command_type = program->at(pos)->operation;
                    if(command_type==BEGIN || command_type==BEGINFINAL)
                        depth += 1;
                    if(command_type==END) {
                        if(depth==0)
                            break;
                        depth -= 1;
                    }
                    pos += 1;
                }
                bbassert(depth>=0, "Code block never ended");
                value = new Code(program, i+1, pos, memory_);
                if(command->operation==BEGINFINAL)
                    value->isMutable = false;
                i = pos;
                command->value = value->shallowCopy();
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case END: 
                // does not create a return signal
                if(returnSignalHandler){
                    memory->release();
                    delete returnSignal;
                    delete args;
                }
                return value;
            break;
            case CALL: {
                //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                Data* context = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                Data* execute = MEMGET(memory, 2);
                // create new writable memory under the current context
                std::shared_ptr<BMemory> newMemory;
                // check if the call has some context, and if so, execute it in the new memory
                if(context && context->getType()==CODE) {
                    // define a return signal and args for the call, to prevent it from releasing the memory, which should be cleaned up after the block finishes executing
                    bool* call_returnSignal = new bool(false);
                    BuiltinArgs* call_args = new BuiltinArgs();

                    Code* code = (Code*)context;
                    newMemory = std::make_shared<BMemory>(memory_, LOCAL_EXPACTATION_FROM_CODE(code));
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, 
                                         call_returnSignal, args);

                    delete call_args;
                    delete call_returnSignal;
                    
                    // still wait for threads to execute on the new memory
                    //for(Future* thread : newMemory->attached_threads)
                    //    thread->getResult();
                    //newMemory->attached_threads.clear();
                }
                else if(context && context->getType()==STRUCT) {
                    newMemory = std::make_shared<BMemory>(memory_, ((Struct*)context)->getMemory()->size());
                    newMemory->pull(((Struct*)context)->getMemory());
                }
                else
                    newMemory = std::make_shared<BMemory>(memory_, DEFAULT_LOCAL_EXPECTATION);
                // 
                Code* code = (Code*)execute;
                // reframe which memory is this
                if(newMemory!=nullptr)
                    newMemory->detach(code->getDeclarationMemory()); // reattaches the new memory on the declaration memory
                
                std::shared_ptr<FutureData> data = std::make_shared<FutureData>();
                data->result = std::make_shared<ThreadResult>();
                data->thread = std::thread(threadExecute, 
                                            program, 
                                            code->getStart(), 
                                            code->getEnd(), 
                                            newMemory, 
                                            nullptr, 
                                            nullptr,
                                            data->result);
                Future* future = new Future(data);
                memory->attached_threads.insert(future);
                value = future;
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case RETURN:
                value = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                *returnSignal = true;
                CHECK_FOR_RETURN(value);
            break;
            case GET:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==STRUCT, "Can only get fields from a struct");
                bbassert(!command->knownLocal[2], "Cannot get a field that is a local variable (starting with _bb...)");
                FILL_REPLACEMENT;
                Struct* obj = (Struct*)value;
                value = obj->getMemory()->get(command->args[2])->shallowCopyIfNeeded();
            }
            break;
            case IS:
                value = MEMGET(memory, 1)->shallowCopy();
                FILL_REPLACEMENT;
            break;
            case SET:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==STRUCT, "Can only set fields in a struct" );
                Struct* obj = (Struct*)value;
                Data* setValue = MEMGET(memory, 3);
                bbassert(!command->knownLocal[2], "Cannot set a field that is a local variable (starting with _bb...)");
                if(setValue && setValue->getType()==CODE && ((Code*)setValue)->getDeclarationMemory()->isOrDerivedFrom(obj->getMemory())) {
                    std::cerr << "Cannot set a code block to a struct field from a scope that is not internal to the code scope's definition"<<std::endl;
                    exit(1);
                }
                obj->getMemory()->set(command->args[2], setValue->shallowCopyIfNeeded()); 
                continue;
            }
            break;
            case WHILE: {
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for while condition");
                bbassert(accept->getType()==CODE, "Can only inline a non-called code block for while loop");

                // hash the outcome of getting code properties
                Code* codeCondition = (Code*)condition;
                Code* codeAccept = (Code*)accept;
                int conditionStart = codeCondition->getStart();
                int conditionEnd = codeCondition->getEnd();
                std::vector<Command*>* conditionProgram = (std::vector<Command*>*)codeCondition->getProgram();
                int acceptStart = codeAccept->getStart();
                int acceptEnd = codeAccept->getEnd();
                std::vector<Command*>* acceptProgram = (std::vector<Command*>*)codeAccept->getProgram();

                // implement the loop
                while(true) {
                    Data* check = executeBlock(conditionProgram, conditionStart, conditionEnd, memory_, returnSignal, args);
                    CHECK_FOR_RETURN(check);
                    if(check==nullptr || (check->getType()==BOOL && !((Boolean*)check)->getValue()))
                        break;
                    value = executeBlock(acceptProgram, acceptStart, acceptEnd, memory_, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
                continue;
            }
            break;
            case IF:{
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
                Data*reject = command->nargs>3?MEMGET(memory, 3):nullptr;
                bbassert(condition->getType()==CODE, "Can only inline a non-called code block for if condition");
                bbverify(accept, accept->getType()==CODE, "Can only inline a non-called code block for if acceptance");
                bbverify(reject, reject->getType()==CODE, "Can only inline a non-called code block for if rejection");
                Code* codeCondition = (Code*)condition;
                Code* codeAccept = (Code*)accept;
                Code* codeReject = (Code*)reject;
                Data* check = executeBlock((std::vector<Command*>*)codeCondition->getProgram(), codeCondition->getStart(), codeCondition->getEnd(), memory_, returnSignal, args);
                CHECK_FOR_RETURN(check);
                if(check && (check->getType()!=BOOL || ((Boolean*)check)->getValue())) {
                    if(codeAccept) {
                        value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory_, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                    }
                }
                else if(codeReject) {
                    value = executeBlock(program, codeReject->getStart(), codeReject->getEnd(), memory_, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
                continue;
            }
            break;
            case INLINE:{
                value = MEMGET(memory, 1);
                if(value->getType()==STRUCT) {
                    memory->pull(((Struct*)value)->getMemory());
                }
                else if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    Code* code = (Code*)value;
                    value = executeBlock(program, code->getStart(), code->getEnd(), memory_, returnSignal, args);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            memory->release();
                            delete returnSignal;
                            delete args;
                        }
                        return value;
                    }
                }
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case DEFAULT:{
                value = MEMGET(memory, 1);
                if(value->getType()==STRUCT) {
                    memory->replaceMissing(((Struct*)value)->getMemory());
                }
                else if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value;
                    std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory_, LOCAL_EXPACTATION_FROM_CODE(code));
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, returnSignal, args);
                    memory->replaceMissing(newMemory);
                    newMemory->release();
                    if(*returnSignal){
                        if(returnSignalHandler){
                            memory->release();
                            delete args;
                        }
                        return value;
                    }
                }
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case NEW:{
                value = MEMGET(memory, 1);
                if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    Code* code = (Code*)value;
                    std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory_, LOCAL_EXPACTATION_FROM_CODE(code));
                    Struct* thisObj = new Struct(newMemory);
                    thisObj->isMutable = false;
                    thisObj->isDestroyable = false;
                    newMemory->set(variableManager. thisId, thisObj);
                    bool* call_returnSignal = new bool(false);
                    BuiltinArgs* call_args = new BuiltinArgs();
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, 
                                         call_returnSignal, call_args);
                    delete call_returnSignal;
                    delete call_args;
                    if(value) {
                        // free up the memory only if we returned neither this nor some code defined within that mamory (we don't need to check for the memory's parrents recursively, as we then detach the memory)
                        if(value!=thisObj 
                            && (value->getType()!=CODE || ((Code*)value)->getDeclarationMemory()->isOrDerivedFrom(newMemory))) {
                            value = value->shallowCopy();
                            newMemory->release(); // release only after the copy, as the release will delet the original value stored internally
                        }
                        else
                            value = value;
                    }
                    else
                        newMemory->release();
                    newMemory->detach();
                }
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case TIME:
                value = new BFloat(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count());
            break;
            case TOLIST:{
                BList* list = new BList();
                for(int i=1;i<command->nargs;i++)
                    list->contents->contents.push_back(MEMGET(memory, i)->shallowCopyIfNeeded());
                value = list;
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            default:
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
                cmdSize = command->nargs;
                args->size = cmdSize-1;
                if(cmdSize>1)
                    args->arg0 = MEMGET(memory, 1);
                if(cmdSize>2)
                    args->arg1 = MEMGET(memory, 2);
                if(cmdSize>3)
                    args->arg2 = MEMGET(memory, 3);
                //args->preallocResult = (command->args[0]?(command->knownLocal[0]?memory->locals[command->args[0]]:memory->getOrNull(command->args[0], true)):prevValue);
                args->preallocResult = toReplace;
                if(args->preallocResult && !(args->preallocResult->isMutable && args->preallocResult->isDestroyable)) 
                    args->preallocResult = nullptr;
                value = Data::run(command->operation, args);
                //if(value && value==args->preallocResult)
                //    continue;
            break;
        }
        memory->unsafeSet(command->args[0], value, toReplace);
    }
    if(returnSignalHandler) {
        memory->release();
        delete returnSignal;
        delete args;
    }
    return value;
}


int vm(const std::string& fileName, int numThreads) {
    /**
     * Reads commands from a compiled blombly assembly file (.bbvm) 
     * and runs them in the virtual machine.
     * @param fileName The path to the file.
     * @return 0 if execution completed successfully
    */

    // open the blombly assembly file
    std::ifstream inputFile(fileName);
    std::vector<Command*> program;
    if (!inputFile.is_open())  {
        std::cerr << "Unable to open file: " << fileName << std::endl;
        exit(1);
        return 1;
    }

    // organizes each line to a new assembly command
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(new Command(line));
    inputFile.close();

    // initialize memory and execute the assembly commands
    std::shared_ptr<BMemory> memory = std::make_shared<BMemory>(nullptr, DEFAULT_LOCAL_EXPECTATION);
    executeBlock(&program, 0, program.size()-1, memory, nullptr, nullptr);
    return 0;
}


int main(int argc, char* argv[]) {
    initializeOperationMapping();
    // parse file to run
    std::string fileName = "main.bb";
    int threads = std::thread::hardware_concurrency();
    if(threads==0)
        threads = 4;
    if (argc > 1) 
        fileName = argv[1];
    if (argc > 2)  
        threads = atoi(argv[2]);
    // if the file has a blombly source code format (.bb) compile 
    // it into an assembly file (.bbvm)
    if(fileName.substr(fileName.size()-3, 3)==".bb") {
        if(compile(fileName, fileName+"vm"))
            return false;
        if(optimize(fileName+"vm", fileName+"vm"))
            return false;
        fileName = fileName+"vm";
    }

    // if no threads, keep the compiled file only
    if(threads==0)
        return 0;

    // initialize mutexes
    if (pthread_mutex_init(&printLock, NULL) != 0) {
        printf("\nPrint mutex init failed\n"); 
        return 1; 
    }

    program_start = std::chrono::steady_clock::now();
    // run the assembly file in the virtual machine
    return vm(fileName, threads);
}