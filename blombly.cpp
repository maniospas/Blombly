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
#include "include/Boolean.h"
#include "include/Integer.h"
#include "include/Float.h"
#include "include/Vector.h"
#include "include/BString.h"
#include "include/List.h"
#include "include/Future.h"
#include "include/Code.h"
#include "include/Memory.h"


std::chrono::steady_clock::time_point program_start;
VariableManager variableManager; // .lastId will always be 0 (currently it is disabled)
//#define MEMGET(memory, arg) (command->args[arg]?(command->knownLocal[arg]?memory->locals[command->args[arg]]:memory->get(command->args[arg])):prevValue)
#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getLocal(command->args[arg]):memory->get(command->args[arg]))


class Command {
public:
    OperationType operation;
    int* args;
    bool* knownLocal;
    int nargs;
    std::shared_ptr<Data> value;

    ~Command() {
        delete args;
    }

    Command(std::string command) {
        std::vector<std::string> argNames;
        argNames.reserve(4);
        std::string accumulate;
        int pos = 0;
        bool inString = false;
        while(pos<command.size()){
            if(command[pos]=='"')
                inString = !inString;
            if(!inString && (command[pos]==' ' || pos==command.size()-1)){
                if(command[pos]!=' ')
                    accumulate += command[pos];
                argNames.push_back(accumulate);
                accumulate = "";
            }
            else
                accumulate += command[pos];
            pos += 1;
        }
        operation = getOperationType(argNames[0]);
        nargs = argNames.size()-1;

        if(operation==BUILTIN) {
            nargs -= 1;
            std::string raw = argNames[2];
            if(raw[0]=='"')
                value = std::make_shared<BString>(raw.substr(1, raw.size()-2));
            else if(raw[0]=='I')
                value = std::make_shared<Integer>(std::atoi(raw.substr(1).c_str()));
            else if(raw[0]=='F')
                value = std::make_shared<Float>(std::atof(raw.substr(1).c_str()));
            else if(raw[0]=='B') 
                value = std::make_shared<Boolean>(raw=="Btrue");
            else {
                std::cerr << "Unable to understand builtin value: " << raw << std::endl;
                exit(1);
            }
            value->isMutable = false;
        }
        
        args = new int[nargs];
        knownLocal = new bool[nargs];
        for(int i=0;i<nargs;i++) {
            knownLocal[i] = argNames[i+1].substr(0, 3)=="_bb";
            args[i] = variableManager.getId(argNames[i+1]);
        }

    }
};


std::shared_ptr<Data> executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory, 
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins
                  );


class Struct: public Data {
private:
    std::shared_ptr<Memory> memory;
public:
    Struct(std::shared_ptr<Memory> mem) {
        memory=mem;
    }
    /*virtual bool couldBeShallowCopy(std::shared_ptr<Data> data) {
        return data->getType()==STRUCT && std::static_pointer_cast<Struct>(data)->memory==memory;
    }*/
    int getType() const override {return STRUCT;}
    std::string toString() const override {
        try{
            BuiltinArgs args;
            args.size = 1;
            args.arg0 = (Data*)this;
            std::shared_ptr<Data> repr = Data::run(TOSTR, &args);
            return repr->toString();
        }
        catch(Unimplemented){
        }
        return "struct";
    }
    std::shared_ptr<Memory>& getMemory() {return memory;}
    void lock() {memory->lock();}
    void unlock(){memory->unlock();}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Struct>(memory);}
    std::shared_ptr<Data> implement(const OperationType operation_, const BuiltinArgs* args_) override {
        if(args_->size==1 && args_->arg0->getType()==STRUCT && operation_==TOCOPY)
            return std::make_shared<Struct>(memory);
        std::string operation = getOperationTypeName(operation_);
        std::shared_ptr<Data> implementation = memory->getOrNull(variableManager.getId(operation), true);
        if(implementation==nullptr)
            throw Unimplemented();
        std::shared_ptr<BList> args = std::make_shared<BList>();
        args->contents->contents.reserve(4);
         // TODO: investigate if shallow copy is needed bellow (update: needed for new version of BuiltinArgs)
        if(args_->size>0)
            args->contents->contents.push_back(args_->arg0->shallowCopy()); 
        if(args_->size>1)
            args->contents->contents.push_back(args_->arg1->shallowCopy()); 
        if(args_->size>2)
            args->contents->contents.push_back(args_->arg2->shallowCopy()); 
        if(implementation->getType()==CODE) {
            Code* code = (Code*)implementation.get();
            std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
            //newMemory->set("locals", std::make_shared<Struct>(newMemory));
            newMemory->set(variableManager.argsId, args);
            //memory->detach(code->getDeclarationMemory()); // MEMORY HIERARCHY FROM PARENT TO CHILD: code->getDeclarationMemory() -> memory (struct data) -> newMemory (function scope)
            std::shared_ptr<FutureData> data = std::make_shared<FutureData>();
            data->result = std::make_shared<ThreadResult>();
            std::vector<Command*>* program = (std::vector<Command*>*)code->getProgram();
            std::shared_ptr<Data> value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, 
                nullptr, nullptr);
            //memory->detach();
            //for(std::shared_ptr<Future> thread : memory->attached_threads)
            //    thread->getResult();
            return value;
        }
        else 
            std::cout<<operation<<" is not a method\n";
        throw Unimplemented();
    }
};

void threadExecute(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins,
                  std::shared_ptr<ThreadResult> result) {
        result->value = executeBlock(program, start, end, memory, returnSignal, allocatedBuiltins);
        //for(std::shared_ptr<Future> thread : memory->attached_threads)
        //    thread->getResult();
}


pthread_mutex_t printLock; 
std::shared_ptr<Data> inline executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory,
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
    bool returnSignalHandler = false;
    BuiltinArgs* args = allocatedBuiltins;
    if(!returnSignal) {
        returnSignal = new bool[1];
        *returnSignal = false;
        returnSignalHandler = true;
        args = new BuiltinArgs();
    }
    //std::shared_ptr<Data> prevValue;
    int cmdSize;
    std::shared_ptr<Data> value;
    for(int i=start;i<=end;i++) {
        Command* command = program->at(i);
        //std::cout << command[0]<<"\n";
        switch(command->operation) {
            case BUILTIN:
                if(command->knownLocal[0]) {
                    value = command->value; // always not immutable
                    // since locals are get and set without any restriction,
                    // setting them as immutable just prevents the actual internal 
                    // value of builtins from being modified when the object is reassigned
                }
                else {
                    value = command->value->shallowCopy(); // TODO: find ways to optimize this (adds +7% to runtime)
                }
            break;
            case FINAL:
                if(command->knownLocal[1]) {
                    std::cerr << "Cannot finalize a local variable (starting with _bb...)"<<std::endl;
                    exit(1);
                }
                value = MEMGET(memory, 1);
                value->isMutable = false;
            break;
            case PRINT:{
                std::string printing("");
                for(int i=1;i<command->nargs;i++) {
                    std::shared_ptr<Data> printable = MEMGET(memory, i);
                    if(printable) {
                        std::string out = printable->toString();
                        printing += out+" ";
                    }
                }
                pthread_mutex_lock(&printLock); 
                std::cout << printing << std::endl;
                pthread_mutex_unlock(&printLock);
            }
            break;
            case BEGIN:
            case BEGINFINAL: {
                if(command->value) {
                    Code* code = (Code*)command->value.get();
                    // it is important to make a new object here, as we need the new memory (the program, start, and end are the same)
                    value = std::make_shared<Code>(program, code->getStart(), code->getEnd(), memory);
                    i = code->getEnd();
                    if(command->operation==BEGINFINAL)
                        value->isMutable = false;
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
                if(depth>0) {
                    std::cerr << "Unclosed code block" << std::endl;
                    exit(1);
                }
                value = std::make_shared<Code>(program, i+1, pos, memory);
                if(command->operation==BEGINFINAL)
                    value->isMutable = false;
                i = pos;
                command->value = value;
            }
            break;
            case END: 
                if(returnSignalHandler) {
                    for(std::shared_ptr<Future> thread : memory->attached_threads)
                        thread->getResult();
                    delete returnSignal;
                    delete args;
                    //if(prevValue)
                    //    prevValue = prevValue->shallowCopy();
                    if(value)
                        value = value->shallowCopy();
                }
                //return prevValue;
                return value;
            break;
            case CALL: {
                // create new writable memory under the current context
                std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                std::shared_ptr<Data> context = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                std::shared_ptr<Data> execute = MEMGET(memory, 2);
                // check if the call has some context, and if so, execute it in the new memory
                if(context && context->getType()==CODE) {
                    std::shared_ptr<Code> code = std::static_pointer_cast<Code>(context);
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
                    //if(value) // show an error message if the context returned with anything other than END
                    //    std::cerr << "Code execution context should not return a value." << std::endl;
                }
                else if(context && context->getType()==STRUCT)
                    newMemory->pull(std::static_pointer_cast<Struct>(context)->getMemory());
                // 
                std::shared_ptr<Code> code = std::static_pointer_cast<Code>(execute);
                // reframe which memory is self
                if(newMemory!=nullptr)
                    newMemory->detach(code->getDeclarationMemory()); // basically reattaches the new memory on the declarati9n memory
                
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
                //std::cout<<"thread started: "<<command[3]<<"\n";
                std::shared_ptr<Future> future = std::make_shared<Future>(data);
                memory->attached_threads.push_back(future);
                value = std::move(future);
            }
            break;
            case RETURN:
                // make return copy the object (NOTE: copying only reallocates (and changes) wrapper properties like finality, not the internal value - internal memory of structs will be the same)
                value = MEMGET(memory, 1);
                if(returnSignalHandler)
                    for(std::shared_ptr<Future> thread : memory->attached_threads)
                        thread->getResult();   
                *returnSignal = true;
                if(returnSignalHandler) {
                    delete args;
                    delete returnSignal;
                }
                return value&&!value->isMutable?value->shallowCopy():value;
            break;
            case GET:
                value = MEMGET(memory, 1);
                if(value->getType()!=STRUCT) {
                    std::cerr << "Can only get fields from a struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    std::shared_ptr<Struct> obj = std::static_pointer_cast<Struct>(value);
                    //obj->lock(); TODO
                    if(command->knownLocal[2]) {
                        std::cerr << "Cannot get a field that is a local variable (starting with _bb...)"<<std::endl;
                        exit(1);
                    }
                    value = obj->getMemory()->get(command->args[2]);
                    //obj->unlock(); TODO
                    value = value?value->shallowCopy():value;
                }
            break;
            case IS:
                value = MEMGET(memory, 1);
                value = value->shallowCopy();
            break;
            case SET:
                value = MEMGET(memory, 1);
                if(value->getType()!=STRUCT) {
                    std::cerr << "Can only set fields in a struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                /*else if(!value->isMutable) {
                    std::cerr << "Can not set fields in a final struct" << std::endl;
                    value = nullptr;
                }*/
                else {
                    Struct* obj = (Struct*)value.get();
                    std::shared_ptr<Data> setValue = MEMGET(memory, 3);
                    if(command->knownLocal[2]) {
                        std::cerr << "Cannot set a field that is a local variable (starting with _bb...)"<<std::endl;
                        exit(1);
                    }
                    //obj->lock(); // TODO
                    obj->getMemory()->set(command->args[2], setValue->shallowCopy()); 
                    //obj->unlock(); // TODO
                    value = nullptr;
                }
            break;
            case WHILE: {
                Data* condition = MEMGET(memory, 1).get();
                Data* accept = MEMGET(memory, 2).get();
                if(condition->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for while condition" << std::endl;
                    exit(1);
                }
                else if(accept->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for while loop" << std::endl;
                    exit(1);
                }
                else {
                    Code* codeCondition = (Code*)condition;
                    Code* codeAccept = (Code*)accept;
                    int codeConditionStart = codeCondition->getStart();
                    int codeConditionEnd = codeCondition->getEnd();
                    int codeAcceptStart = codeAccept->getStart();
                    int codeAcceptEnd = codeAccept->getEnd();
                    while(true) {
                        std::shared_ptr<Data> check = executeBlock(program, codeConditionStart, codeConditionEnd, memory, returnSignal, args);
                        if(*returnSignal) {
                            if(returnSignalHandler) {
                                for(std::shared_ptr<Future> thread : memory->attached_threads)
                                    thread->getResult();
                                delete returnSignal;
                                delete args;
                                if(check)
                                    check = check->shallowCopy();
                            }
                            return check;
                        }
                        if(check->getType()!=BOOL) {
                            std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                            exit(1);
                            break;
                        }
                        else if(((Boolean*)check.get())->getValue()) 
                            value = std::move(executeBlock(program, codeAcceptStart, codeAcceptEnd, memory, returnSignal, args));
                        else
                            break;
                        if(*returnSignal) {
                            if(returnSignalHandler) {
                                for(std::shared_ptr<Future> thread : memory->attached_threads)
                                    thread->getResult(); 
                                delete returnSignal;
                                delete args;
                                if(value)
                                    value = value->shallowCopy();
                            }
                            return value;
                        }
                    }
                }
            }
            break;
            case IF:{
                Data* condition = MEMGET(memory, 1).get();
                Data* accept = MEMGET(memory, 2).get();
                Data*reject = command->nargs>3?(MEMGET(memory, 3).get()):nullptr;
                if(condition->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for if condition" << std::endl;
                    exit(1);
                }
                else if(accept->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for if acceptance" << std::endl;
                    exit(1);
                }
                else if(reject && reject->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for if rejection" << std::endl;
                    exit(1);
                }
                else {
                    Code* codeCondition = (Code*)condition;
                    Code* codeAccept = (Code*)accept;
                    Code* codeReject = (Code*)reject;
                    std::shared_ptr<Data> check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal, args);
                    
                    if(*returnSignal) {
                        if(returnSignalHandler){
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                            delete returnSignal;
                            delete args;
                        }
                        return check->shallowCopy();
                    }
                    if(check->getType()!=BOOL) {
                        std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                        exit(1);
                    }
                    else if(((Boolean*)check.get())->getValue()) {
                        if(codeAccept)
                            value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory, returnSignal, args);
                    }
                    else {
                        if(codeReject)
                            value = executeBlock(program, codeReject->getStart(), codeReject->getEnd(), memory, returnSignal, args);
                    }
                    if(*returnSignal) {
                        if(returnSignalHandler){
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                            delete returnSignal;
                            delete args;
                        }
                        return value->shallowCopy();
                    }
                }
            }
            break;
            case INLINE:{
                value = MEMGET(memory, 1);
                if(value->getType()==STRUCT) {
                    std::shared_ptr<Struct> code = std::static_pointer_cast<Struct>(value);
                    memory->pull(code->getMemory());
                }
                else if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    Code* code = (Code*)value.get();
                    value = executeBlock(program, code->getStart(), code->getEnd(), memory, returnSignal, args);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                            delete returnSignal;
                            delete args;
                        }
                        return value->shallowCopy();
                    }
                }
            }
            break;
            case DEFAULT:{
                value = MEMGET(memory, 1);
                if(value->getType()==STRUCT) {
                    Struct* code = (Struct*)value.get();
                    memory->replaceMissing(code->getMemory());
                }
                else if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value.get();
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, returnSignal, args);
                    memory->replaceMissing(newMemory);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                            delete returnSignal;
                            delete args;
                        }
                        return value->shallowCopy();
                    }
                }
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
                    std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                    std::shared_ptr<Struct> thisObj = std::make_shared<Struct>(newMemory);
                    thisObj->isMutable = false;
                    newMemory->set(variableManager.thisId, thisObj);
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value.get();
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
                    newMemory->detach();
                }
            }
            break;
            case TIME:{
                std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
                value = std::make_shared<Float>(std::chrono::duration_cast<std::chrono::duration<double>>(time-program_start).count());
            }
            break;
            case TOLIST:{
                std::shared_ptr<BList> list = std::make_shared<BList>();
                for(int i=1;i<command->nargs;i++)
                    list->contents->contents.push_back(MEMGET(memory, i));
                value = list;
            }
            break;
            default:
                cmdSize = command->nargs;
                args->size = cmdSize-1;
                if(cmdSize>1)
                    args->arg0 = MEMGET(memory, 1).get();
                if(cmdSize>2)
                    args->arg1 = MEMGET(memory, 2).get();
                if(cmdSize>3)
                    args->arg2 = MEMGET(memory, 3).get();
                //args->preallocResult = (command->args[0]?(command->knownLocal[0]?memory->locals[command->args[0]]:memory->getOrNull(command->args[0], true)):prevValue);
                args->preallocResult = std::move(command->knownLocal[0]?memory->getLocal(command->args[0]):memory->getOrNullShallow(command->args[0]));
                if(args->preallocResult && !args->preallocResult->isMutable) 
                    args->preallocResult = nullptr;
                value = std::move(Data::run(command->operation, args));
                if(args->preallocResult==value && value) // ignore expensive setting (this would need a redundant map insert)
                    continue;
            break;
        }
        if(command->args[0]!=variableManager.noneId) {
            /*if(i<end-1) {
                // TODO: test that this if branch does not hang programs
                if(command->knownLocal[0])
                    memory->locals[command->args[0]] = std::move(value);
                else
                    memory->set(command->args[0], std::move(value));
            }
            else {*/
                if(command->knownLocal[0])
                    memory->setLocal(command->args[0], value);
                else
                    memory->set(command->args[0], value);
            //}
        }
        //prevValue = value;
    }
    if(returnSignalHandler) {
        for(std::shared_ptr<Future> thread : memory->attached_threads)
            thread->getResult(); 
        delete returnSignal;
        delete args;
        if(value)
            value = value->shallowCopy(); // shallow copy only when not in inline execution
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
    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    //std::shared_ptr<Struct> thisObj = std::make_shared<Struct>(memory);
    //thisObj->isMutable = false;
    //memory->set(variableManager.thisId, thisObj);
    //memory->set("locals", std::make_shared<Struct>(memory));
    executeBlock(&program, 0, program.size()-1, memory, nullptr, nullptr);
    for(int i=0;i<program.size();i++)
        delete program[i];
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