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
#include <filesystem>
#include "src/utils/stringtrim.cpp"
#include "src/utils/parser.cpp"
#include "src/utils/optimizer.cpp"
#include "src/utils/transpiler.cpp"
#include "src/utils/cross_platform.cpp"

// include data types
#include "include/common.h"
#include "include/data/Boolean.h"
#include "include/data/Integer.h"
#include "include/data/BFloat.h"
#include "include/data/Vector.h"
#include "include/data/BString.h"
#include "include/data/List.h"
#include "include/data/BHashMap.h"
#include "include/data/Future.h"
#include "include/data/Code.h"
#include "include/data/Struct.h"
#include "include/data/BFile.h"
#include "include/data/BError.h"
#include "include/BMemory.h"
#include "include/interpreter/Command.h"
#include "include/interpreter/thread.h"


std::chrono::steady_clock::time_point program_start;
//#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getLocal(command->args[arg]):memory->get(command->args[arg]))

#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getOrNullShallow(command->args[arg]):memory->get(command->args[arg]))

#define CHECK_FOR_RETURN(expr) if(*returnSignal) { \
                        if(returnSignalHandler){ \
                            if(memory->release(expr)) delete memory;\
                            delete returnSignal; \
                            delete args; \
                        } \
                        return expr; \
                    }
//#define FILL_REPLACEMENT toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0])
#define FILL_REPLACEMENT toReplace = memory->getOrNullShallow(command->args[0])


pthread_mutex_t printLock; 
pthread_mutex_t compileLock; 


Code* compileAndLoad(std::string fileName, BMemory* currentMemory) {
    pthread_mutex_lock(&compileLock); 

    // compile and optimize
    if(fileName.substr(fileName.size()-3, 3)==".bb") {
        compile(fileName, fileName+"vm");
        optimize(fileName+"vm", fileName+"vm");
        fileName = fileName+"vm";
    }

    // open the blombly assembly file
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open())  {
        bberror("Unable to open file: " + fileName);
        return nullptr;
    }

    // organize each line to a new assembly command
    std::vector<Command*>* program = new std::vector<Command*>();
    //SourceFile* source = new SourceFile(fileName);
    SourceFile* source = new SourceFile(fileName.substr(0, fileName.size()));
    std::string line;
    std::string originalName = fileName.substr(0, fileName.size()-2);
    int i = 1;
    CommandContext* descriptor = nullptr;//std::filesystem::exists(originalName)?new CommandContext(originalName+" line 1"):nullptr;
    while (std::getline(inputFile, line)) {
        if(line[0]=='%') {
            if(descriptor)
                descriptor = new CommandContext(originalName+" "+line.substr(1));
        }
        else 
            program->push_back(new Command(line, source, i, descriptor));
        ++i;
    }
    inputFile.close();
    
    // create the code
    pthread_mutex_unlock(&compileLock);
    return new Code(program, 0, program->size()-1, currentMemory);
}


Data* executeBlock(const Code* code,
                  BMemory* memory,
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
    //BMemory* memory = memory_.get();
    //std::cout << memory_.use_count()<<" entered\n";
    //std::cout << memory<<" "<<memory_.use_count()<<" enteted count\n";
    bool returnSignalHandler = !returnSignal;
    BuiltinArgs* args = allocatedBuiltins;
    if(returnSignalHandler) {
        returnSignal = new bool(false);
        args = new BuiltinArgs();
    }
    //std::shared_ptr<Data> prevValue;
    int cmdSize;
    Data* value;
    std::vector<Command*>* program = (std::vector<Command*>*)code->getProgram();
    int end = code->getEnd();
    int i=code->getStart();
    try {
    for(;i<=end;++i) {
        Command* command = program->at(i);
        //std::cout << command->toString() << "     | refs to memory " << memory_.use_count() << "\n";
        Data* toReplace;
        //std::cout << "COMMAND "<<memory_.use_count()<<" "<<command->operation<<getOperationTypeName(command->operation)<<"\n";
        switch(command->operation) {
            case BEGIN:
            case BEGINCACHED:
            case BEGINFINAL: {
                if(command->value) {
                    Code* code = (Code*)command->value;
                    toReplace = memory->getOrNullShallow(command->args[0]);
                    Code* val = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory, code->getAllMetadata());
                    val->scheduleForParallelExecution = code->scheduleForParallelExecution;
                    value = val;
                    if(command->operation==BEGINFINAL)
                        memory->setFinal(command->args[0]);
                    i = code->getEnd();
                    break; // break the switch
                }
                int pos = i+1;
                int depth = 0;
                OperationType command_type;
                bool scheduleForParallelExecution = true; // FOR NOW SCHEDULE EVERYTHING FOR PARALLEL EXECUTION
                int countCalls = 0;
                while(pos<=end) {
                    command_type = program->at(pos)->operation;
                    //if(command_type==WHILE || command_type==CALL || command_type==TOLIST || command_type==TOVECTOR) 
                    //    scheduleForParallelExecution = true;
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
                Code* cache = new Code(program, i+1, pos, nullptr);
                Code* val = new Code(program, i+1, pos, memory, cache->getAllMetadata());
                val->scheduleForParallelExecution = scheduleForParallelExecution;
                cache->scheduleForParallelExecution = scheduleForParallelExecution;
                cache->isDestroyable = false;
                command->value = cache;
                value = val;
                if(command->operation==BEGINFINAL) 
                    memory->setFinal(command->args[0]);
                i = pos;
                toReplace = memory->getOrNullShallow(command->args[0]);
            }
            break;
            case CALL:{
                Data* context = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                Data* called = MEMGET(memory, 2);
                bbassert(called, "Cannot call a missing value");
                Code* code;
                if(called->getType()==STRUCT) {
                    Data* entry = ((Struct*)called)->getMemory()->get(variableManager.callId);
                    bbassert(entry->getType(), "Struct is not callable. Assign to its `\\call` field during construction.");
                    bbassert(entry->getType()==CODE, "Struct `\\call` is not a code block.");
                    code = (Code*)entry;
                }
                else {
                    bbassert(called->getType()==CODE, "Only structs or code blocks can be called.");
                    code = (Code*)called;
                }
                
                BMemory* codeMemory = code->getDeclarationMemory();
                bbassert(codeMemory,
                            "Memoryless code block cannot be called."
                            "\n   \033[33m!!!\033[0m Returned code blocks are not attached to any memory context"
                            "\n       to use its final variables. Consider these options:"
                            "\n       - Set `\\call=block;` to make the struct callable."
                            "\n       - Set the block as an object field and call that. It can"
                            "\n         also be set as a field of another object."
                            "\n       - Inline the returned block. For example, `rebased={block:}`"
                            "\n         rebases it to the current context."
                            "\n       Recursive calls could be violated if you assign to a different"
                            "\n       name. No option is automated to prevent ambiguity.");
                BMemory* newMemory = new BMemory(memory, LOCAL_EXPACTATION_FROM_CODE(code));
                FILL_REPLACEMENT;
                if(context) {
                    bbassert(context->getType()==CODE, "The call's context must be a code block");
                    value = executeBlock((Code*)context, newMemory, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
                newMemory->detach(codeMemory); // detach the memory after executing the context

                if(!code->scheduleForParallelExecution || !Future::acceptsThread()) 
                    value = executeBlock(code, newMemory, nullptr, nullptr);
                else {
                    std::shared_ptr<FutureData> data = std::make_shared<FutureData>();
                    data->result = std::make_shared<ThreadResult>();
                    data->thread = std::thread(threadExecute, 
                                                code, 
                                                newMemory, 
                                                nullptr, 
                                                nullptr,
                                                data->result,
                                                command);
                    Future* future = new Future(data);
                    memory->attached_threads.insert(future);
                    value = future;
                }
            }
            break;
            case RETURN:
                value = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                *returnSignal = true;
                CHECK_FOR_RETURN(value);
            break;
            case GET:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==STRUCT || value->getType()==CODE, "Can only get fields from structs or metadata from code blocks");
                bbassert(!command->knownLocal[2], "Cannot get a field that is a local variable (starting with _bb...)");
                FILL_REPLACEMENT;
                if(value->getType()==CODE) {
                    Code* obj = (Code*)value;
                    value = obj->getMetadata(command->args[2])->shallowCopyIfNeeded();
                }
                else {
                    Struct* obj = (Struct*)value;
                    value = obj->getMemory()->get(command->args[2]);;
                    if(value) {
                        if(value->getType()==CODE) {
                            Code* code = (Code*)value;
                            value = new Code(code->getProgram(), code->getStart(), code->getEnd(), obj->getMemory(), code->getAllMetadata());
                        }
                        else
                            value = value->shallowCopyIfNeeded();
                    }

                }
            }
            break;
            case IS:
                value = MEMGET(memory, 1)->shallowCopyIfNeeded();
                FILL_REPLACEMENT;
            break;
            case EXISTS:
                FILL_REPLACEMENT;
                if(toReplace && toReplace->getType()==BOOL) {
                    ((Boolean*)toReplace)->setValue(memory->contains(command->args[1]));
                    value = toReplace;
                }
                else
                    value = new Boolean(memory->contains(command->args[1])); // TODO: optimize this to overwrite a boolean toReplace
                break;
            break;
            case SET:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==STRUCT, "Can only set fields (with the non-final setter) in a struct" );
                Struct* obj = (Struct*)value;
                Data* setValue = MEMGET(memory, 3);
                bbassert(!command->knownLocal[2], "Cannot set a field that is a local variable (starting with _bb...)");
                //if(setValue && setValue->getType()==CODE && ((Code*)setValue)->getDeclarationMemory()->isOrDerivedFrom(obj->getMemory())) {
                //    bberror("Cannot set a code block to a struct field here.\n    This is only possible for blocks defined inside the struct scope's definition");
                //}
                if(setValue)
                    setValue = setValue->shallowCopyIfNeeded();
                int item = command->args[2];
                toReplace = obj->getMemory()->getOrNullShallow(item);
                obj->getMemory()->unsafeSet(item, setValue, toReplace); 
                continue;
            }
            break;
            case SETFINAL:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==CODE, "Can only set metadata (with the final setter) for code blocks");
                Code* obj = (Code*)value;
                Data* setValue = MEMGET(memory, 3);
                bbassert(!command->knownLocal[2], "Cannot set a specification that is a local variable (starting with _bb...)");
                obj->setMetadata(command->args[2], setValue->shallowCopyIfNeeded());
                continue;
            }
            break;
            case WHILE: {
                // get params
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);

                // get body
                bbassert(accept->getType()==CODE, "While body can only be a code block");
                Code* codeAccept = (Code*)accept;
                
                // if condition is boolean do a different kind of loop where it is re-evaluated continuously
                bbassert(condition, "while condition's expression did not evaluate to anything");
                if(condition->getType()==BOOL) {
                    // implement the loop
                    while(condition->isTrue()) {
                        //bbassert(check, "while condition's expression did not evaluate to anything");
                        //bbassert(check->getType()==BOOL, "While condition variable that started as bool was changed to something else midway");
                        //if(!condition->isTrue())
                        //    break;
                        value = executeBlock(codeAccept, memory, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                        condition = MEMGET(memory, 1);
                    }
                    continue;
                }
                bbassert(condition->getType()==CODE, "While condition can only be a bool or a code block that evaluates to bool");
                // hash the outcome of getting code properties
                Code* codeCondition = (Code*)condition;

                // implement the loop
                while(true) {
                    Data* check = executeBlock(codeCondition, memory, returnSignal, args);
                    CHECK_FOR_RETURN(check);
                    //bbassert(check, "while condition's expression did not evaluate to anything");
                    //bbassert(check->getType()==BOOL, "while condition's expression did not evaluate to bool");
                    if(!check->isTrue())
                        break;
                    value = executeBlock(codeAccept, memory, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
                continue;
            }
            break;
            case IF:{
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
                Data* reject = command->nargs>3?MEMGET(memory, 3):nullptr;
                //bbverify(accept, accept->getType()==CODE, "Can only inline a non-called code block for if acceptance");
                //bbverify(reject, reject->getType()==CODE, "Can only inline a non-called code block for if rejection");
                if(condition->getType()==CODE) {
                    //bbassert(condition->getType()==CODE, "Can only have a bool or a code block for if condition");
                    Code* codeCondition = (Code*)condition;
                    condition = executeBlock(codeCondition, memory, returnSignal, args);
                }
                //bbassert(check, "if condition's expression did not evaluate to anything");
                CHECK_FOR_RETURN(condition);
                //bbassert(check->getType()==BOOL, "If condition did not evaluate to bool");
                Code* codeAccept = (Code*)accept;
                Code* codeReject = (Code*)reject;
                if(condition->isTrue()) {
                    if(codeAccept) {
                        value = executeBlock(codeAccept, memory, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                    }
                    value = nullptr;
                }
                else if(codeReject) {
                    value = executeBlock(codeReject, memory, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
                else
                    value = nullptr;
                continue;
            }
            break;
            case TRY:{
                try {
                    Data* condition = MEMGET(memory, 1);
                    bbassert(condition->getType()==CODE, "Can only inline a non-called code block for try condition");
                    Code* codeCondition = (Code*)condition;
                    value = executeBlock(codeCondition, memory, returnSignal, args);
                    
                    if(value==nullptr || !*returnSignal) {
                        *returnSignal = false;
                        std::string comm = command->toString();
                        comm.resize(40, ' ');
                        BError* error = new BError(" No return or fail signal was intercepted."
                                                    "\n   \033[33m!!!\033[0m Code enclosed in `try` should use either `return value`"
                                                    "\n       or `error(\"message\")` to respectfully generate return and error signals."
                                                    "\n       This error was created because no such signal was obtained."
                                                    +("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
                        error->consume();  // this is not enough to make the code block to fail
                        value = error;
                    }
                    FILL_REPLACEMENT;
                }
                catch(const BBError& e) {
                    std::string comm = command->toString();
                    comm.resize(40, ' ');
                    value = new BError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
                    FILL_REPLACEMENT;
                }
            }
            break;
            case CATCH:{
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
                Data* reject = command->nargs>3?MEMGET(memory, 3):nullptr;
                bbverify(accept, accept->getType()==CODE, "Can only inline a non-called code block for if acceptance");
                bbverify(reject, reject->getType()==CODE, "Can only inline a non-called code block for if rejection");
                Code* codeAccept = (Code*)accept;
                Code* codeReject = (Code*)reject;
                if(condition->getType()==ERRORTYPE) {
                    ((BError*)condition)->consume();
                    if(codeAccept) {
                        value = executeBlock(codeAccept, memory, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                    }
                }
                else if(codeReject) {
                    value = executeBlock(codeReject, memory, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
            }
            break;
            case INLINE:{
                value = MEMGET(memory, 1);
                if(value->getType()==FILETYPE) {
                    if(command->value) {
                        Code* code = (Code*)command->value;
                        value = new Code(code->getProgram(), code->getStart(), code->getEnd(), nullptr, code->getAllMetadata());
                    }
                    else {
                        value = compileAndLoad(((BFile*)value)->getPath(), nullptr);
                        command->value = value;
                        command->value->isDestroyable = false;
                    }
                }

                if(value->getType()==STRUCT) {
                    memory->pull(((Struct*)value)->getMemory());
                }
                else if(value->getType()!=CODE) {
                    bberror("Can only inline a non-called code block or struct");
                    value = nullptr;
                }
                else {
                    Code* code = (Code*)value;
                    value = executeBlock(code, memory, returnSignal, args);
                    CHECK_FOR_RETURN(value)
                }
                FILL_REPLACEMENT;
            }
            break;
            case DEFAULT:{
                value = MEMGET(memory, 1);
                if(value->getType()==STRUCT) 
                    memory->replaceMissing(((Struct*)value)->getMemory());
                else if(value->getType()!=CODE) {
                    bberror("Can only inline a non-called code block or struct");
                    value = nullptr;
                }
                else {
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value;
                    BMemory*  newMemory =  new BMemory(memory, LOCAL_EXPACTATION_FROM_CODE(code));
                    value = executeBlock(code, newMemory, returnSignal, args);
                    memory->replaceMissing(newMemory);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            delete args;
                        }
                        return value;
                    }
                }
                toReplace = memory->getOrNullShallow(command->args[0]);
            }
            break;
            case NEW:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==CODE, "Can only inline a non-called code block");
                Code* code = (Code*)value;
                BMemory* newMemory = new BMemory(memory, LOCAL_EXPACTATION_FROM_CODE(code));
                Struct* thisObj = new GlobalStruct(newMemory);
                //thisObj->isDestroyable = false;
                newMemory->unsafeSet(variableManager.thisId, thisObj, nullptr);
                newMemory->setFinal(variableManager.thisId);
                bool* call_returnSignal = new bool(false);
                BuiltinArgs* call_args = new BuiltinArgs();
                try {
                    value = executeBlock(code, newMemory, call_returnSignal, call_args);
                }
                catch(BBError e) {
                    delete call_args;
                    delete call_returnSignal;
                    throw e;
                }
                delete call_args;
                delete call_returnSignal;
                if(value!=thisObj) {// && newMemory->release(value))
                    //delete newMemory;
                    if(value) {
                        value = value->shallowCopyIfNeeded();
                        if(value && value->getType()==CODE)// && ((Code*)preserve)->getDeclarationMemory()==this) 
                            ((Code*)value)->setDeclarationMemory(nullptr);
                    }
                    delete thisObj;
                }
                else
                    newMemory->detach();
                /*
                if(value) {
                    // free up the memory only if we returned neither this nor some code defined within that memory (we don't need to check for the memory's parents recursively, as we then detach the memory)
                    if(value==thisObj) {
                    }
                    else if(value->getType()!=CODE) {
                        value = value->shallowCopyIfNeeded();
                        newMemory->release(value); // release only after the copy, as the release will delete the original value stored internally
                        //delete thisObj;
                    }
                    else if(!((Code*)value)->getDeclarationMemory()->isOrDerivedFrom(newMemory)) {
                        value = value->shallowCopyIfNeeded();
                    }
                }
                else {
                    newMemory->release(value);
                }
                newMemory->detach();
                toReplace = memory->getOrNullShallow(command->args[0]);
                */
            }
            break;
            case BUILTIN:
                // if there is already a local variable value for the builtin, skip setting it
                //if(command->lastCalled==memory && command->knownLocal[0]) 
                //    continue;
                //command->lastCalled = memory;
                FILL_REPLACEMENT;
                if(command->value==toReplace)
                    continue;
                value = command->value;
            break;
            case FINAL:
                // setting a memory content to final should alway be an attomic operation
                if(command->knownLocal[1]) {
                    bberror("Cannot finalize a local variable (starting with _bb...)");
                }
                //value = MEMGET(memory, 1);
                //memory->lock();
                memory->setFinal(command->args[1]);
                //std::cout << memory->isFinal(command->args[1]) << variableManager.getSymbol(command->args[1]);
                //value->isMutable = false;
                //memory->unlock();
                continue; // completely ignore any setting
            break;
            case FAIL:{
                value = MEMGET(memory, 1);
                std::string comm = command->toString();
                comm.resize(40, ' ');
                throw BBError(value->toString()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
                continue;
            }
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
            case END: 
                // does not create a return signal
                if(returnSignalHandler){
                    delete returnSignal;
                    delete args;
                }
                return value;
            break;
            case TIME:
                value = new BFloat(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count());
                FILL_REPLACEMENT;
            break;
            case TOLIST://if(command->nargs==1)
            {
                BList* list = new BList();
                list->contents->contents.reserve(command->nargs-1);
                for(int i=1;i<command->nargs;i++)
                    list->contents->contents.push_back(MEMGET(memory, i)->shallowCopyIfNeeded());
                value = list;
                FILL_REPLACEMENT;
                break;
            }
            case TOMAP:if(command->nargs==1){
                BHashMap* map = new BHashMap();
                value = map;
                FILL_REPLACEMENT;
                break;
            }
            default:
                cmdSize = command->nargs;
                args->size = cmdSize-1;
                if(cmdSize>1) {
                    args->arg0 = MEMGET(memory, 1);
                    bbassert(args->arg0, "Missing value: "+variableManager.getSymbol(command->args[1]));
                }
                if(cmdSize>2) {
                    args->arg1 = MEMGET(memory, 2);
                    bbassert(args->arg1, "Missing value: "+variableManager.getSymbol(command->args[2]));
                }
                if(cmdSize>3) {
                    args->arg2 = MEMGET(memory, 3);
                    bbassert(args->arg2, "Missing value: "+variableManager.getSymbol(command->args[3]));
                }
                FILL_REPLACEMENT;
                // locals are never final
                if((!command->knownLocal[0] && memory->isFinal(command->args[0])) || (toReplace && !toReplace->isDestroyable))
                    args->preallocResult = nullptr;
                else
                    args->preallocResult = toReplace;
                value = Data::run(command->operation, args);
            break;
        }
        memory->unsafeSet(command->args[0], value, toReplace);
        //std::cout << command->toString()<< " \t OBJECTS "<<Data::countObjects()<<"\n";
        }
    }
    catch(const BBError& e) {
        Command* command = program->at(i);
        std::string comm = command->toString();
        comm.resize(40, ' ');
        if(returnSignalHandler) {
            memory->release();
            delete memory;
            delete returnSignal;
            delete args;
        }
        throw BBError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
    }
    if(returnSignalHandler) {
        if(memory->release(value))
            delete memory;
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
   
    Future::setMaxThreads(numThreads);
    // std::cout << "Setting number of threads "<<numThreads<<"\n";
   
    try {
        {
            // open the blombly assembly file
            std::ifstream inputFile(fileName);
            std::vector<Command*> program;
            if (!inputFile.is_open())  
                bberror("Unable to open file: " + fileName);

            // organizes each line to a new assembly command
            //SourceFile* source = new SourceFile(fileName);
            SourceFile* source = new SourceFile(fileName);
            std::string line;
            std::string originalName = fileName.substr(0, fileName.size()-2);
            int i = 1;
            CommandContext* descriptor = nullptr;//std::filesystem::exists(originalName)?new CommandContext(originalName+" line 1"):nullptr;
            while (std::getline(inputFile, line)) {
                if(line[0]=='%') {
                    if(descriptor)
                        descriptor = new CommandContext(originalName+" "+line.substr(1));
                }
                else 
                    program.push_back(new Command(line, source, i, descriptor));
                ++i;
            }
            inputFile.close();

            BMemory* memory = new BMemory(nullptr, DEFAULT_LOCAL_EXPECTATION);
            Code code(&program, 0, program.size()-1, memory);
            executeBlock(&code, memory, nullptr, nullptr); // releases and deletes
        }
        BMemory::verify_noleaks();
    }
    catch(const BBError& e) {
        std::cout << e.what() << "\n";
        std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    Terminal::enableVirtualTerminalProcessing();
    initializeOperationMapping();

    std::string fileName = "main.bb";
    int threads = std::thread::hardware_concurrency();
    int default_threads = threads;
    bool cexecute = false;

    if (threads == 0)
        threads = 4;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--threads" || arg == "-threads") && i + 1 < argc) {
            threads = atoi(argv[++i]);
        } else if (arg == "-version" || arg == "--version" || arg == "-v") {
            std::cout << "\n\033[0mVersion: \033[33mblombly 0.2.1\n\x1B[0mCopyright (c): \x1B[32m 2024 Emmanouil Krasanakis\n\x1B[0mDocs and bug reports: \x1B[34mhttps://maniospas.github.io/Blombly\x1B[0m\n\n";
            return 0;
        } else if (arg == "-help" || arg == "--help" || arg == "-h") {
            std::cout << "\033[0mUsage:\n  \033[33m.\\blombly\033[90m [options] \033[0m[file]\n";
            std::cout << "\033[0mOptions:\n";
            std::cout << "\033[90m  If no file is provided, main.bb will be compiled and run.\n";
            std::cout << "\033[90m  --version, -v       Show version information.\n";
            std::cout << "\033[90m  --help, -h          Show this help message.\n";
            std::cout << "\033[90m  --threads <num>     Set max threads. Default for this machine: "<<default_threads<<"\n";
            return 0;
        } 
        else {
            fileName = arg;
        }
    }

    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        std::cout << "\033[0m(\x1B[31m ERROR \033[0m) File not found: "<<fileName<<"\n   \033[33m!!!\033[0m Run blombly --help for more information.\n";
        std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
        return 1;
    }
    else
        inputFile.close();

    if (fileName.substr(fileName.size() - 3, 3) == ".bb") {
        try {
            compile(fileName, fileName + "vm");
            //std::cout << " \033[0m(\x1B[32m OK \033[0m) Compilation\n";
        }
        catch (const BBError& e) {
            std::cout << e.what() << "\n";
            std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
            return 1;
        }
        try {
            optimize(fileName + "vm", fileName + "vm");
            //std::cout << " \033[0m(\x1B[32m OK \033[0m) Optimization\n";
        }
        catch (const BBError& e) {
            std::cout << e.what() << "\n";
            std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
            return 1;
        }
        fileName = fileName + "vm";
    }

    if (cexecute) {
        std::string cfilename = fileName.substr(0, fileName.size() - 4) + "c";
        try {
            transpile(fileName, cfilename);
            //std::cout << " \033[0m(\x1B[32m OK \033[0m) Transpilation (to .c)\n";
        }
        catch (const BBError& e) {
            std::cout << e.what() << " in " << fileName << "\n";
            return 1;
        }
        std::string execfilename = fileName.substr(0, fileName.size() - 5);
        int ret = system(("gcc -o " + execfilename + " " + cfilename).c_str());
        if (ret != 0)
            return ret;
        if (threads != 0) {
            return system(execfilename.c_str());
        }
        return 0;
    }

    if (threads == 0)
        return 0;

    if (pthread_mutex_init(&printLock, NULL) != 0) {
        printf("\nPrint mutex initialization failed.\n");
        std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
        return 1;
    }

    if (pthread_mutex_init(&compileLock, NULL) != 0) {
        printf("\nPrint mutex initialization failed.\n");
        std::cout << "\n\033[0mDocs and bug reports: \033[34mhttps://maniospas.github.io/Blombly\x1B[0m\n";
        return 1;
    }

    program_start = std::chrono::steady_clock::now();
    return vm(fileName, threads);
}
