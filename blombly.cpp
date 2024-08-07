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
#include "utils/stringtrim.cpp"
#include "utils/parser.cpp"
#include "utils/optimizer.cpp"
#include "utils/transpiler.cpp"

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
#include "include/data/BFile.h"
#include "include/data/BError.h"
#include "include/BMemory.h"
#include "include/interpreter/Command.h"
#include "include/interpreter/thread.h"


namespace Terminal {
    #ifdef _WIN32
    #include <windows.h>

    // Function to enable virtual terminal processing and set UTF-8 encoding
    void enableVirtualTerminalProcessing() {
        // Enable Virtual Terminal Processing on Windows
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            return;
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            return;
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            return;
        }

        // Set console output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }
    #else
    void enableVirtualTerminalProcessing(){}
    #endif
}


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
pthread_mutex_t compileLock; 


Code* compileAndLoad(std::string fileName, const std::shared_ptr<BMemory> currentMemory) {
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
    try {
    for(int i=start;i<=end;i++) {
        Command* command = program->at(i);
        try {
        Data* toReplace;
        //std::cout << "COMMAND "<<memory_.use_count()<<" "<<command->operation<<getOperationTypeName(command->operation)<<"\n";
        switch(command->operation) {
            case BEGIN:
            case BEGINCACHED:
            case BEGINFINAL: {
                if(command->lastCalled==memory && command->knownLocal[0]) {
                    i = ((Code*)command->value)->getEnd();
                    continue;
                }
                command->lastCalled = memory;
                if(command->value) {
                    Code* code = (Code*)command->value;
                    //Code* codeCommand = (Code*)command->value;
                    if(command->operation==BEGINFINAL && 
                        //&& code->getProgram()==codeCommand->getProgram()
                        //&& code->getStart()==codeCommand->getStart()
                        //&& code->getEnd()==codeCommand->getEnd()
                        code->getDeclarationMemory()==memory_) {
                        i = code->getEnd();
                        continue;  // also skip the assignment
                    }
                    toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
                    value = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory_);
                    if(command->operation==BEGINFINAL)
                        memory->setFinal(command->args[0]);
                    i = code->getEnd();
                    break; // break the switch
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
                    memory->setFinal(command->args[0]);
                    //value->isMutable = false;
                i = pos;
                command->value = value->shallowCopy();
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
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
                    for(Future* thread : newMemory->attached_threads)
                        thread->getResult();
                    newMemory->attached_threads.clear();
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
                                            data->result,
                                            command);
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
                bbassert(value->getType()==STRUCT || value->getType()==CODE, "Can only get fields from a struct or metadata from code");
                bbassert(!command->knownLocal[2], "Cannot get a field that is a local variable (starting with _bb...)");
                FILL_REPLACEMENT;
                if(value->getType()==CODE) {
                    Code* obj = (Code*)value;
                    value = obj->getMetadata(command->args[2])->shallowCopyIfNeeded();
                }
                else {
                    Struct* obj = (Struct*)value;
                    value = obj->getMemory()->get(command->args[2])->shallowCopyIfNeeded();
                }
            }
            break;
            case IS:
                value = MEMGET(memory, 1)->shallowCopyIfNeeded();
                FILL_REPLACEMENT;
            break;
            case AS:
                value = MEMGET(memory, 2);
                if(value)
                    value = value->shallowCopyIfNeeded();
           	    toReplace = command->args[1]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[1]);
   		 	    memory->unsafeSet(command->args[1], value, toReplace);
                FILL_REPLACEMENT;
                value = new Boolean(value!=nullptr); // TODO: optimize this to overwrite a boolean toReplace
                break;
            break;
            case SET:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==STRUCT, "Can only set fields (with the non-final setter) in a struct" );
                Struct* obj = (Struct*)value;
                Data* setValue = MEMGET(memory, 3);
                bbassert(!command->knownLocal[2], "Cannot set a field that is a local variable (starting with _bb...)");
                if(setValue && setValue->getType()==CODE && ((Code*)setValue)->getDeclarationMemory()->isOrDerivedFrom(obj->getMemory())) {
                    bberror("Cannot set a code block to a struct field here.\n    This is only possible from blocks defined inside the struct scope's definition");
                }
                obj->getMemory()->set(command->args[2], setValue->shallowCopyIfNeeded()); 
                continue;
            }
            break;
            case SETFINAL:{
                value = MEMGET(memory, 1);
                bbassert(value->getType()==CODE, "Can only set set metadata (with the final setter) in a code block" );
                Code* obj = (Code*)value;
                Data* setValue = MEMGET(memory, 3);
                bbassert(!command->knownLocal[2], "Cannot set a metadata that is a local variable (starting with _bb...)");
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
                int acceptStart = codeAccept->getStart();
                int acceptEnd = codeAccept->getEnd();
                std::vector<Command*>* acceptProgram = (std::vector<Command*>*)codeAccept->getProgram();
                
                // if condition is boolean do a different kind of loop where it is re-evaluated continuously
                bbassert(condition, "while condition's expression did not evaluate to anything");
                if(condition->getType()==BOOL) {
                    // implement the loop
                    while(condition->isTrue()) {
                        //bbassert(check, "while condition's expression did not evaluate to anything");
                        //bbassert(check->getType()==BOOL, "While condition variable that started as bool was changed to something else midway");
                        //if(!condition->isTrue())
                        //    break;
                        value = executeBlock(acceptProgram, acceptStart, acceptEnd, memory_, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                        condition = MEMGET(memory, 1);
                    }
                    continue;
                }
                bbassert(condition->getType()==CODE, "While condition can only be a bool or a code block that evaluates to bool");
                // hash the outcome of getting code properties
                Code* codeCondition = (Code*)condition;
                int conditionStart = codeCondition->getStart();
                int conditionEnd = codeCondition->getEnd();
                std::vector<Command*>* conditionProgram = (std::vector<Command*>*)codeCondition->getProgram();

                // implement the loop
                while(true) {
                    Data* check = executeBlock(conditionProgram, conditionStart, conditionEnd, memory_, returnSignal, args);
                    CHECK_FOR_RETURN(check);
                    //bbassert(check, "while condition's expression did not evaluate to anything");
                    //bbassert(check->getType()==BOOL, "while condition's expression did not evaluate to bool");
                    if(!check->isTrue())
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
                Data* reject = command->nargs>3?MEMGET(memory, 3):nullptr;
                //bbverify(accept, accept->getType()==CODE, "Can only inline a non-called code block for if acceptance");
                //bbverify(reject, reject->getType()==CODE, "Can only inline a non-called code block for if rejection");
                if(condition->getType()!=BOOL) {
                    //bbassert(condition->getType()==CODE, "Can only have a bool or a code block for if condition");
                    Code* codeCondition = (Code*)condition;
                    condition = executeBlock((std::vector<Command*>*)codeCondition->getProgram(), codeCondition->getStart(), codeCondition->getEnd(), memory_, returnSignal, args);
                }
                Code* codeAccept = (Code*)accept;
                Code* codeReject = (Code*)reject;
                //bbassert(check, "if condition's expression did not evaluate to anything");
                CHECK_FOR_RETURN(condition);
                //bbassert(check->getType()==BOOL, "If condition did not evaluate to bool");
                if(condition->isTrue()) {
                    if(codeAccept) {
                        value = executeBlock((std::vector<Command*>*)codeAccept->getProgram(), codeAccept->getStart(), codeAccept->getEnd(), memory_, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                    }
                    value = nullptr;
                }
                else if(codeReject) {
                    value = executeBlock((std::vector<Command*>*)codeReject->getProgram(), codeReject->getStart(), codeReject->getEnd(), memory_, returnSignal, args);
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
                    value = executeBlock((std::vector<Command*>*)codeCondition->getProgram(), codeCondition->getStart(), codeCondition->getEnd(), memory_, returnSignal, args);
                    
                    if(value==nullptr || !*returnSignal) {
                        *returnSignal = false;
                        std::string comm = command->toString();
                        comm.resize(40, ' ');
                        BError* error = new BError(" No return or fail signal was intercepted.\n   \033[33m!!!\033[0m Code enclosed in `try` should use either `return value`\n      or `error(\"message\")` to respectfully generate return and error signals.\n      This error was created because no such signal was obtained."
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
                        value = executeBlock((std::vector<Command*>*)codeAccept->getProgram(), codeAccept->getStart(), codeAccept->getEnd(), memory_, returnSignal, args);
                        CHECK_FOR_RETURN(value);
                    }
                }
                else if(codeReject) {
                    value = executeBlock((std::vector<Command*>*)codeReject->getProgram(), codeReject->getStart(), codeReject->getEnd(), memory_, returnSignal, args);
                    CHECK_FOR_RETURN(value);
                }
            }
            break;
            case INLINE:{
                value = MEMGET(memory, 1);
                if(value->getType()==FILETYPE) {
                    if(command->value) {
                        Code* code = (Code*)command->value;
                        value = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory_);
                    }
                    else {
                        value = compileAndLoad(((BFile*)value)->getPath(), memory_);
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
                    value = executeBlock((std::vector<Command*>*)code->getProgram(), code->getStart(), code->getEnd(), memory_, returnSignal, args);
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
                    bberror("Can only inline a non-called code block or struct");
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
                bbassert(value->getType()==CODE, "Can only inline a non-called code block" );
                Code* code = (Code*)value;
                std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory_, LOCAL_EXPACTATION_FROM_CODE(code));
                Struct* thisObj = new GlobalStruct(newMemory);
                //thisObj->isMutable = false;
                thisObj->isDestroyable = false;
                newMemory->set(variableManager. thisId, thisObj);
                newMemory->setFinal(variableManager.thisId);
                bool* call_returnSignal = new bool(false);
                BuiltinArgs* call_args = new BuiltinArgs();
                value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, call_returnSignal, call_args);
                delete call_returnSignal;
                delete call_args;
                if(value) {
                    // free up the memory only if we returned neither this nor some code defined within that mamory (we don't need to check for the memory's parrents recursively, as we then detach the memory)
                    if(value==thisObj)
                        value = value;//->shallowCopy(); // creates a global struct (because *this* is final, but we don't need the returned object to be set as a final value)
                    else if(value->getType()!=CODE || ((Code*)value)->getDeclarationMemory()->isOrDerivedFrom(newMemory)) {
                        value = value->shallowCopyIfNeeded();
                        newMemory->release(); // release only after the copy, as the release will delete the original value stored internally
                    }
                }
                else
                    newMemory->release();
                newMemory->detach();
                toReplace = command->args[0]==variableManager.thisId?nullptr:memory->getOrNullShallow(command->args[0]);
            }
            break;
            case BUILTIN:
                // if there is already a local variable value for the builtin, skip setting it
                if(command->lastCalled==memory && command->knownLocal[0]) 
                    continue;
                command->lastCalled = memory;
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
                memory->lock();
                memory->setFinal(command->args[1]);
                //std::cout << memory->isFinal(command->args[1]) << variableManager.getSymbol(command->args[1]);
                //value->isMutable = false;
                memory->unlock();
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
                    memory->release();
                    delete returnSignal;
                    delete args;
                }
                return value;
            break;
            case TIME:
                FILL_REPLACEMENT;
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
                FILL_REPLACEMENT;
                cmdSize = command->nargs;
                args->size = cmdSize-1;
                if(cmdSize>1)
                    args->arg0 = MEMGET(memory, 1);
                if(cmdSize>2)
                    args->arg1 = MEMGET(memory, 2);
                if(cmdSize>3)
                    args->arg2 = MEMGET(memory, 3);
                // locals are never final
                if((!command->knownLocal[0] && memory->isFinal(command->args[0])) || (toReplace && !toReplace->isDestroyable))
                    args->preallocResult = nullptr;
                else
                    args->preallocResult = toReplace;
                value = Data::run(command->operation, args);
            break;
        }
        memory->unsafeSet(command->args[0], value, toReplace);
        }
        catch(const BBError& e) {
            std::string comm = command->toString();
            comm.resize(40, ' ');
            throw BBError(e.what()+("\n   \x1B[34m\u2192\033[0m "+comm+" \t\x1B[90m "+command->source->path+" line "+std::to_string(command->line)));
        }
    }
    }
    catch(const BBError& e) {
        if(returnSignalHandler) {
            //memory->release();
            delete returnSignal;
            delete args;
        }
        throw e;
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
   
    try {
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

        std::shared_ptr<BMemory> memory = std::make_shared<BMemory>(nullptr, DEFAULT_LOCAL_EXPECTATION);
        executeBlock(&program, 0, program.size()-1, memory, nullptr, nullptr);
        memory->release();
        
    }
    catch(const BBError& e) {
        std::cout << e.what() << "\n";
        return 1;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    Terminal::enableVirtualTerminalProcessing();
    initializeOperationMapping();
    // parse file to run
    std::string fileName = "main.bb";
    bool cexecute = false;
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
        try {
            compile(fileName, fileName+"vm");
            std::cout << " \033[0m(\x1B[32m OK \033[0m) Compilation\n";
        }
        catch(const BBError& e) {
            std::cout << e.what() << " in " << fileName << "\n";
            //std::cout << " \033[0m(\x1B[31m FAIL \033[0m) Compilation\n";
            return 1;
        }
        try {
            optimize(fileName+"vm", fileName+"vm");
            std::cout << " \033[0m(\x1B[32m OK \033[0m) Optimization\n";
        }
        catch(const BBError& e) {
            std::cout << e.what() << " in " << fileName << "\n";
            //std::cout << " \033[0m(\x1B[31m FAIL \033[0m) Optimization\n";
            return 1;
        }
        fileName = fileName+"vm";
    }
    
    if(cexecute) {
	   std::string cfilename = fileName.substr(0, fileName.size()-4)+"c";
        try {
            transpile(fileName, cfilename);
            std::cout << " \033[0m(\x1B[32m OK \033[0m) Transpilation (to .c)\n";
        }
        catch(const BBError& e) {
            std::cout << e.what() << " in " << fileName << "\n";
            return 1;
        }
        std::string execfilename = fileName.substr(0, fileName.size()-5);
   	   int ret = system(("gcc -o "+execfilename+" "+cfilename).c_str());
   	   if(ret!=0)
   	  	return ret;
        if(threads!=0) {
        	  return system(execfilename.c_str());
        }
        return 0;
    }

    // if no threads, keep the compiled file only
    if(threads==0)
        return 0;

    // initialize mutexes
    if (pthread_mutex_init(&printLock, NULL) != 0) {
        printf("\nPrint mutex init failed\n"); 
        return 1; 
    }

    // initialize mutexes
    if (pthread_mutex_init(&compileLock, NULL) != 0) {
        printf("\nPrint mutex init failed\n"); 
        return 1; 
    }
    program_start = std::chrono::steady_clock::now();
    // run the assembly file in the virtual machine
    return vm(fileName, threads);
}