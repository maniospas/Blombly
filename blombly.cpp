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
#include "include/BFloat.h"
#include "include/Vector.h"
#include "include/BString.h"
#include "include/List.h"
#include "include/Future.h"
#include "include/Code.h"
#include "include/BMemory.h"


std::chrono::steady_clock::time_point program_start;
VariableManager variableManager; // .lastId will always be 0 (currently it is disabled)
//#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getLocal(command->args[arg]):memory->get(command->args[arg]))

#define MEMGET(memory, arg) memory->get(command->args[arg])


class Command {
public:
    OperationType operation;
    int* args;
    bool* knownLocal;
    int nargs;
    Data* value;

    ~Command() {
        delete args;
        delete value;
        delete knownLocal;
    }

    Command(std::string command) {
        value = nullptr;
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
                value = new BString(raw.substr(1, raw.size()-2));
            else if(raw[0]=='I')
                value = new Integer(std::atoi(raw.substr(1).c_str()));
            else if(raw[0]=='F')
                value = new BFloat(std::atof(raw.substr(1).c_str()));
            else if(raw[0]=='B') 
                value = new Boolean(raw=="Btrue");
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
            //knownLocal[i] = false;
            args[i] = variableManager.getId(argNames[i+1]);
        }

    }
};


Data* executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory, 
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins
                  );


class Struct: public Data {
private:
    std::shared_ptr<BMemory> memory;
public:
    Struct(const std::shared_ptr<BMemory>& mem) {
        memory = mem;
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
            Data* repr = Data::run(TOSTR, &args);
            return repr->toString();
        }
        catch(Unimplemented){
        }
        return "struct";
    }
    std::shared_ptr<BMemory>& getMemory() {return memory;}
    void lock() {memory->lock();}
    void unlock(){memory->unlock();}
    Data* shallowCopy() const override {return new Struct(memory);}
    Data* implement(const OperationType operation_, BuiltinArgs* args_) override {
        if(args_->size==1 && args_->arg0->getType()==STRUCT && operation_==TOCOPY)
            return new Struct(memory);
        std::string operation = getOperationTypeName(operation_);
        Data* implementation = memory->getOrNull(variableManager.getId(operation), true);
        if(!implementation)
            throw Unimplemented();
        BList* args = new BList();;
        args->contents->contents.reserve(4);
         // TODO: investigate if shallow copy is needed bellow (update: needed for new version of BuiltinArgs)
        if(args_->size>0)
            args->contents->contents.push_back(args_->arg0->shallowCopy()); 
        if(args_->size>1)
            args->contents->contents.push_back(args_->arg1->shallowCopy()); 
        if(args_->size>2)
            args->contents->contents.push_back(args_->arg2->shallowCopy()); 
        if(implementation->getType()==CODE) {
            Code* code = (Code*)implementation;
            std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory);
            newMemory->set(variableManager.argsId, args);
            //memory->detach(code->getDeclarationMemory()); // MEMORY HIERARCHY FROM PARENT TO CHILD: code->getDeclarationMemory() -> memory (struct data) -> newMemory (function scope)
            std::vector<Command*>* program = (std::vector<Command*>*)code->getProgram();
            Data* value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
            //memory->detach();
            //for(std::shared_ptr<Future> thread : memory->attached_threads)
            //    thread->getResult();
            return value;
        }
        else {
            std::cerr<<operation<<" is not a method\n";
            exit(1);
        }
        throw Unimplemented();
    }
};

void threadExecute(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory,
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins,
                  std::shared_ptr<ThreadResult> result) {
        Data *ret = executeBlock(program, start, end, memory, returnSignal, allocatedBuiltins);
        if(ret)
            ret = ret->shallowCopy();
        result->value = ret;
}


pthread_mutex_t printLock; 
Data* executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory,
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
    Data* value;
    for(int i=start;i<=end;i++) {
        Command* command = program->at(i);
        //std::cout << command[0]<<"\n";
        switch(command->operation) {
            case BUILTIN:
                {
                    //if(command->value->couldBeShallowCopy(memory->getOrNullShallow(command->args[0])))
                    //    continue;
                    if(command->value==memory->getOrNullShallow(command->args[0]))
                        continue;
                    value = command->value->shallowCopy();
                    delete command->value;
                    command->value = value;
                }
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
                    //if(command->value==memory->getOrNullShallow(command->args[0])))
                    //    break;
                    
                    Code* code = (Code*)command->value;
                    Code* codeCommand = (Code*)command->value;
                    if(command->operation==BEGINFINAL 
                        && code->getProgram()==codeCommand->getProgram()
                        && code->getStart()==codeCommand->getStart()
                        && code->getEnd()==codeCommand->getEnd()
                        && code->getDeclarationMemory()==memory) {
                        value = command->value;
                    }
                    else {
                        value = new Code(code->getProgram(), code->getStart(), code->getEnd(), memory);
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
                if(depth>0) {
                    std::cerr << "Unclosed code block" << std::endl;
                    exit(1);
                }
                value = new Code(program, i+1, pos, memory);
                if(command->operation==BEGINFINAL)
                    value->isMutable = false;
                i = pos;
                command->value = value->shallowCopy();
            }
            break;
            case END: 
                if(returnSignalHandler) {
                    for(Future* thread : memory->attached_threads)
                        thread->getResult();
                    memory->attached_threads.clear();
                    delete returnSignal;
                    delete args;
                }
                //return prevValue;
                return value;
            break;
            case CALL: {
                // create new writable memory under the current context
                std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory);
                //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                Data* context = command->args[1]==variableManager.noneId?nullptr:MEMGET(memory, 1);
                Data* execute = MEMGET(memory, 2);
                // check if the call has some context, and if so, execute it in the new memory
                if(context && context->getType()==CODE) {
                    Code* code = (Code*)context;
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
                    //if(value) // show an error message if the context returned with anything other than END
                    //    std::cerr << "Code execution context should not return a value." << std::endl;
                }
                else if(context && context->getType()==STRUCT)
                    newMemory->pull(((Struct*)context)->getMemory());
                // 
                Code* code = (Code*)execute;
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
                Future* future = new Future(data);
                memory->attached_threads.push_back(future);
                value = future;
                
                //value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
            }
            break;
            case RETURN:
                // make return copy the object (NOTE: copying only reallocates (and changes) wrapper properties like finality, not the internal value - internal memory of structs will be the same)
                if(command->args[1]==variableManager.noneId)
                    value = nullptr;
                else
                    value = MEMGET(memory, 1);
                if(returnSignalHandler) {
                    for(Future* thread : memory->attached_threads)
                        thread->getResult(); 
                    memory->attached_threads.clear();
                    delete args;
                    delete returnSignal;
                }
                else  
                    *returnSignal = true;
                return value;
            break;
            case GET:
                value = MEMGET(memory, 1);
                if(value->getType()!=STRUCT) {
                    std::cerr << "Can only get fields from a struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    Struct* obj = (Struct*)value;
                    if(command->knownLocal[2]) {
                        std::cerr << "Cannot get a field that is a local variable (starting with _bb...)"<<std::endl;
                        exit(1);
                    }
                    value = obj->getMemory()->get(command->args[2]);
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
                    Struct* obj = (Struct*)value;
                    Data* setValue = MEMGET(memory, 3);
                    if(command->knownLocal[2]) {
                        std::cerr << "Cannot set a field that is a local variable (starting with _bb...)"<<std::endl;
                        exit(1);
                    }
                    //obj->lock(); // TODO
                    obj->getMemory()->set(command->args[2], setValue->shallowCopy()); 
                    //obj->unlock(); // TODO
                    value = nullptr;
                    continue;
                }
            break;
            case WHILE: {
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
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
                        Data* check = executeBlock(program, codeConditionStart, codeConditionEnd, memory, returnSignal, args);
                        if(*returnSignal) {
                            if(returnSignalHandler) {
                                for(Future* thread : memory->attached_threads)
                                    thread->getResult();
                                memory->attached_threads.clear();
                                delete returnSignal;
                                delete args;
                            }
                            return check;
                        }
                        if(check==nullptr) {
                            //td::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                            //exit(1);
                            break;
                        }
                        else if(check->getType()!=BOOL || ((Boolean*)check)->getValue()) 
                            value = executeBlock(program, codeAcceptStart, codeAcceptEnd, memory, returnSignal, args);
                        else
                            break;
                        if(*returnSignal) {
                            if(returnSignalHandler) {
                                for(Future* thread : memory->attached_threads)
                                    thread->getResult(); 
                                memory->attached_threads.clear();
                                delete returnSignal;
                                delete args;
                            }
                            return value;
                        }
                    }
                }
            }
            break;
            case IF:{
                Data* condition = MEMGET(memory, 1);
                Data* accept = MEMGET(memory, 2);
                Data*reject = command->nargs>3?MEMGET(memory, 3):nullptr;
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
                    Data* check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal, args);
                    
                    if(*returnSignal) {
                        if(returnSignalHandler){
                            for(Future* thread : memory->attached_threads)
                                thread->getResult(); 
                            memory->attached_threads.clear();
                            delete returnSignal;
                            delete args;
                        }
                        return check;
                    }
                    if(check==nullptr) {
                        //std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                        //exit(1);
                        if(codeReject)
                            value = executeBlock(program, codeReject->getStart(), codeReject->getEnd(), memory, returnSignal, args);
                    }
                    else if(check->getType()!=BOOL || ((Boolean*)check)->getValue()) {
                        if(codeAccept)
                            value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory, returnSignal, args);
                    }
                    else {
                        if(codeReject)
                            value = executeBlock(program, codeReject->getStart(), codeReject->getEnd(), memory, returnSignal, args);
                    }
                    if(*returnSignal) {
                        if(returnSignalHandler){
                            for(Future* thread : memory->attached_threads)
                                thread->getResult(); 
                            memory->attached_threads.clear();
                            delete returnSignal;
                            delete args;
                        }
                        return value;
                    }
                }
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
                    value = executeBlock(program, code->getStart(), code->getEnd(), memory, returnSignal, args);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            for(Future* thread : memory->attached_threads)
                                thread->getResult(); 
                            memory->attached_threads.clear();
                            delete returnSignal;
                            delete args;
                        }
                        return value;
                    }
                }
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
                    std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory);
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value;
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, returnSignal, args);
                    memory->replaceMissing(newMemory);
                    if(*returnSignal){
                        if(returnSignalHandler){
                            for(Future* thread : memory->attached_threads)
                                thread->getResult(); 
                            memory->attached_threads.clear();
                            delete returnSignal;
                            delete args;
                        }
                        return value;
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
                    std::shared_ptr<BMemory> newMemory = std::make_shared<BMemory>(memory);
                    Struct* thisObj = new Struct(newMemory);
                    thisObj->isMutable = false;
                    newMemory->set(variableManager. thisId, thisObj);
                    //newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    Code* code = (Code*)value;
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr, nullptr);
                    value = value->shallowCopy();
                    newMemory->detach();
                }
            }
            break;
            case TIME:{
                std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
                value = new BFloat(std::chrono::duration_cast<std::chrono::duration<double>>(time-program_start).count());
            }
            break;
            case TOLIST:{
                BList* list = new BList();
                for(int i=1;i<command->nargs;i++)
                    list->contents->contents.push_back(MEMGET(memory, i)->shallowCopy());
                value = list;
            }
            break;
            default:
                cmdSize = command->nargs;
                args->size = cmdSize-1;
                if(cmdSize>1)
                    args->arg0 = MEMGET(memory, 1);
                if(cmdSize>2)
                    args->arg1 = MEMGET(memory, 2);
                if(cmdSize>3)
                    args->arg2 = MEMGET(memory, 3);
                //args->preallocResult = (command->args[0]?(command->knownLocal[0]?memory->locals[command->args[0]]:memory->getOrNull(command->args[0], true)):prevValue);
                args->preallocResult = memory->getOrNullShallow(command->args[0]);
                if(args->preallocResult && !args->preallocResult->isMutable) 
                    args->preallocResult = nullptr;
                value = Data::run(command->operation, args);
                //if(value && value==args->preallocResult)
                //    continue;
            break;
        }
        if(command->args[0]==variableManager.noneId){
            if(value)
                delete value;
        }
        else {
            memory->set(command->args[0], value);
        }
    }
    if(returnSignalHandler) {
        for(Future* thread : memory->attached_threads)
            thread->getResult(); 
        memory->attached_threads.clear();
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
    std::shared_ptr<BMemory> memory = std::make_shared<BMemory>(nullptr);
    //std::shared_ptr<Struct> thisObj = std::make_shared<Struct>(memory);
    //thisObj->isMutable = false;
    //memory->set(variableManager.thisId, thisObj);
    //memory->set("locals", std::make_shared<Struct>(memory));
    executeBlock(&program, 0, program.size()-1, memory, nullptr, nullptr);
    //for(int i=0;i<program.size();i++)
    //    delete program[i];
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