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

#define MEMGET(memory, arg) (arg=="LAST"?prevValue:memory->get(arg))

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


class Command {
public:
    std::vector<std::string> args;
    OperationType operation;
    Command(std::string command) {
        std::string accumulate;
        int pos = 0;
        bool inString = false;
        while(pos<command.size()){
            if(command[pos]=='"')
                inString = !inString;
            if(!inString && (command[pos]==' ' || pos==command.size()-1)){
                if(command[pos]!=' ')
                    accumulate += command[pos];
                args.push_back(accumulate);
                accumulate = "";
            }
            else
                accumulate += command[pos];
            pos += 1;
        }
        operation = getOperationType(args[0]);
    }
};


std::shared_ptr<Data> executeBlock(std::vector<std::shared_ptr<Command>>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory, 
                  bool *returnSignal);


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
    std::string toString() const override {return "struct";}
    std::shared_ptr<Memory>& getMemory() {return memory;}
    void lock() {memory->lock();}
    void unlock(){memory->unlock();}
    std::shared_ptr<Data> shallowCopy() const override {return std::make_shared<Struct>(memory);}
    std::shared_ptr<Data> implement(const OperationType operation_, const BuiltinArgs& args_) override {
        if(args_.size==1 && args_.arg0->getType()==STRUCT && operation_==TOCOPY)
            return std::make_shared<Struct>(memory);
        std::string operation = getOperationTypeName(operation_);
        std::shared_ptr<Data> implementation = memory->getOrNull(operation, true);
        if(implementation==nullptr)
            throw Unimplemented();
        std::shared_ptr<List> args = std::make_shared<List>();
         // TODO: investigate if shallow copy is needed bellow
        if(args_.size>0 && args_.arg0.get()!=this)
            args->contents->contents.push_back(args_.arg0); 
        if(args_.size>1 && args_.arg1.get()!=this)
            args->contents->contents.push_back(args_.arg1); 
        if(args_.size>2 && args_.arg2.get()!=this)
            args->contents->contents.push_back(args_.arg2); 
        if(implementation->getType()==CODE) {
            std::shared_ptr<Code> code = std::static_pointer_cast<Code>(implementation);
            std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
            newMemory->set("locals", std::make_shared<Struct>(newMemory));
            newMemory->set("args", args);
            newMemory->detach(code->getDeclarationMemory());
            std::shared_ptr<FutureData> data = std::make_shared<FutureData>();
            data->result = std::make_shared<ThreadResult>();
            std::vector<std::shared_ptr<Command>>* program = (std::vector<std::shared_ptr<Command>>*)code->getProgram();
            std::shared_ptr<Data> value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr);
            //for(std::shared_ptr<Future> thread : memory->attached_threads)
            //    thread->getResult();
            return value;
        }
        else 
            std::cout<<operation<<" is not a method\n";
        throw Unimplemented();
    }
};

void threadExecute(std::vector<std::shared_ptr<Command>>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory,
                  bool *returnSignal,
                  std::shared_ptr<ThreadResult> result) {
        result->value = executeBlock(program, start, end, memory, returnSignal);
        //for(std::shared_ptr<Future> thread : memory->attached_threads)
        //    thread->getResult();
}


pthread_mutex_t printLock; 
std::shared_ptr<Data> executeBlock(std::vector<std::shared_ptr<Command>>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory,
                  bool *returnSignal) {
    /**
     * Executes a block of code from within a list of commands.
     * @param program The list of command pointers.
     * @param start The position from which to start the interpreting.
     * @param end The position at which to stop the interpreting (inclusive).
     * @param memory A pointer to the read and write memory.
     * @return A Data shared pointer holding the code block's outcome. This is nullptr if nothing is returned.
    */
    bool returnSignalHandler = false;
    if(!returnSignal) {
        returnSignal = new bool[1];
        *returnSignal = false;
        returnSignalHandler = true;
    }
    std::shared_ptr<Data> prevValue;
    BuiltinArgs args;
    int cmdSize;
    for(int i=start;i<=end;i++) {
        auto command = program->at(i)->args;
        OperationType operation = program->at(i)->operation;
        std::shared_ptr<Data> value;
        //std::cout << command[0]<<"\n";
        switch(operation) {
            case BUILTIN:
                if(command[2][0]=='"')
                    value = std::make_shared<BString>(command[2].substr(1, command[2].size()-2));
                else if(command[2][0]=='I')
                    value = std::make_shared<Integer>(std::atoi(command[2].substr(1).c_str()));
                else if(command[2][0]=='F')
                    value = std::make_shared<Float>(std::atof(command[2].substr(1).c_str()));
                else if(command[2][0]=='B') 
                    value = std::make_shared<Boolean>(command[2]=="Btrue");
                else {
                    std::cerr << "Unable to understand builtin value: " << command[2] << std::endl;
                    exit(1);
                }
            break;
            case FINAL:
                MEMGET(memory, command[2])->isMutable = false;
            break;
            case PRINT:{
                for(int i=2;i<command.size();i++) {
                    std::shared_ptr<Data> printable = MEMGET(memory, command[i]);
                    if(printable) {
                        std::string out = printable->toString();
                        pthread_mutex_lock(&printLock);
                        std::cout << out << " ";
                        pthread_mutex_unlock(&printLock);
                    }
                }
                pthread_mutex_lock(&printLock); 
                std::cout << "\n";
                pthread_mutex_unlock(&printLock);
            }
            break;
            case BEGIN:
            case BEGINFINAL: {
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
                if(operation==BEGINFINAL)
                    value->isMutable = false;
                i = pos;
            }
            break;
            case END: 
                if(returnSignalHandler)
                    for(std::shared_ptr<Future> thread : memory->attached_threads)
                        thread->getResult(); 
                if(returnSignalHandler)
                    delete returnSignal;
                return prevValue?prevValue->shallowCopy():prevValue;
            break;
            case CALL: {
                // create new writable memory under the current context
                std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                newMemory->set("locals", std::make_shared<Struct>(newMemory));
                std::shared_ptr<Data> context = command[2]=="#"?nullptr:MEMGET(memory, command[2]);
                std::shared_ptr<Data> execute = MEMGET(memory, command[3]);
                // check if the call has some context, and if so, execute it in the new memory
                if(context && context->getType()==CODE) {
                    std::shared_ptr<Code> code = std::static_pointer_cast<Code>(context);
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr);
                    //if(value) // show an error message if the context returned with anything other than END
                    //    std::cerr << "Code execution context should not return a value." << std::endl;
                }
                else if(context && context->getType()==STRUCT)
                    newMemory->pull(std::static_pointer_cast<Struct>(context)->getMemory());
                // 
                std::shared_ptr<Code> code = std::static_pointer_cast<Code>(execute);
                // reframe which memory is self
                if(newMemory!=nullptr)
                    newMemory->detach(code->getDeclarationMemory());
                
                std::shared_ptr<FutureData> data = std::make_shared<FutureData>();
                data->result = std::make_shared<ThreadResult>();
                data->thread = std::thread(threadExecute, 
                                            program, 
                                            code->getStart(), 
                                            code->getEnd(), 
                                            newMemory, 
                                            nullptr, 
                                            data->result);
                //std::cout<<"thread started: "<<command[3]<<"\n";
                std::shared_ptr<Future> future = std::make_shared<Future>(data);
                memory->attached_threads.push_back(future);
                value = future;
            }
            break;
            case RETURN:
                // make return copy the object (NOTE: copying only reallocates (and changes) wrapper properties like finality, not the internal value - internal memory of structs will be the same)
                value = MEMGET(memory, command[2]);
                if(returnSignalHandler)
                    for(std::shared_ptr<Future> thread : memory->attached_threads)
                        thread->getResult();   
                *returnSignal = true;
                if(returnSignalHandler)
                    delete returnSignal;
                return value?value->shallowCopy():value;
            break;
            case GET:
                value = MEMGET(memory, command[2]);
                if(value->getType()!=STRUCT) {
                    std::cerr << "Can only get fields from a struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    std::shared_ptr<Struct> obj = std::static_pointer_cast<Struct>(value);
                    //obj->lock(); TODO
                    value = obj->getMemory()->get(command[3]);
                    //obj->unlock(); TODO
                    value = value?value->shallowCopy():value;
                }
            break;
            case IS:
                value = MEMGET(memory, command[2]);
                memory->set(command[1], value->shallowCopy());
            break;
            case SET:
                value = MEMGET(memory, command[2]);
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
                    std::shared_ptr<Struct> obj = std::static_pointer_cast<Struct>(value);
                    std::shared_ptr<Data> setValue = MEMGET(memory, command[4]);
                    //obj->lock(); // TODO
                    obj->getMemory()->set(command[3], setValue);
                    //obj->unlock(); // TODO
                    value = nullptr;
                }
            break;
            case WHILE: {
                std::shared_ptr<Data> condition = MEMGET(memory, command[2]);
                std::shared_ptr<Data> accept = MEMGET(memory, command[3]);
                if(condition->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for while condition" << std::endl;
                    exit(1);
                }
                else if(accept->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block for while loop" << std::endl;
                    exit(1);
                }
                else {
                    std::shared_ptr<Code> codeCondition = std::static_pointer_cast<Code>(condition);
                    std::shared_ptr<Code> codeAccept = accept?std::static_pointer_cast<Code>(accept):nullptr;
                    while(true) {
                        std::shared_ptr<Data> check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal);
                        if(*returnSignal) {
                            if(returnSignalHandler)
                                for(std::shared_ptr<Future> thread : memory->attached_threads)
                                    thread->getResult(); 
                            if(returnSignalHandler)
                                delete returnSignal;
                            return check->shallowCopy();
                        }
                        if(check->getType()!=BOOL) {
                            std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                            exit(1);
                            break;
                        }
                        else if(std::static_pointer_cast<Boolean>(check)->getValue()) 
                            value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory, returnSignal);
                        else
                            break;
                        if(*returnSignal) {
                            if(returnSignalHandler)
                                for(std::shared_ptr<Future> thread : memory->attached_threads)
                                    thread->getResult(); 
                            if(returnSignalHandler)
                                delete returnSignal;
                            return value->shallowCopy();
                        }
                    }
                }
            }
            break;
            case IF:{
                std::shared_ptr<Data> condition = MEMGET(memory, command[2]);
                std::shared_ptr<Data> accept = MEMGET(memory, command[3]);
                std::shared_ptr<Data> reject = command.size()>4?(command[4]=="#"?nullptr:MEMGET(memory, command[4])):nullptr;
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
                    std::shared_ptr<Code> codeCondition = std::static_pointer_cast<Code>(condition);
                    std::shared_ptr<Code> codeAccept = accept?std::static_pointer_cast<Code>(accept):nullptr;
                    std::shared_ptr<Code> codeReject = reject?std::static_pointer_cast<Code>(reject):nullptr;
                    std::shared_ptr<Data> check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal);
                    if(*returnSignal) {
                        if(returnSignalHandler)
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                        if(returnSignalHandler)
                            delete returnSignal;
                        return check->shallowCopy();
                    }
                    if(check->getType()!=BOOL) {
                        std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                        exit(1);
                    }
                    else if(std::static_pointer_cast<Boolean>(check)->getValue()) {
                        if(codeAccept)
                            value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory, returnSignal);
                    }
                    else {
                        if(codeReject)
                            value = executeBlock(program, codeReject->getStart(), codeReject->getEnd(), memory, returnSignal);
                    }
                    if(*returnSignal) {
                        if(returnSignalHandler)
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                        if(returnSignalHandler)
                            delete returnSignal;
                        return value->shallowCopy();
                    }
                }
            }
            break;
            case INLINE:{
                value = MEMGET(memory, command[2]);
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
                    std::shared_ptr<Code> code = std::static_pointer_cast<Code>(value);
                    value = executeBlock(program, code->getStart(), code->getEnd(), memory, returnSignal);
                    if(*returnSignal){
                        if(returnSignalHandler)
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                        if(returnSignalHandler)
                            delete returnSignal;
                        return value->shallowCopy();
                    }
                }
            }
            break;
            case DEFAULT:{
                value = MEMGET(memory, command[2]);
                if(value->getType()==STRUCT) {
                    std::shared_ptr<Struct> code = std::static_pointer_cast<Struct>(value);
                    memory->replaceMissing(code->getMemory());
                }
                else if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                    newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    std::shared_ptr<Code> code = std::static_pointer_cast<Code>(value);
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, returnSignal);
                    memory->replaceMissing(newMemory);
                    if(*returnSignal){
                        if(returnSignalHandler)
                            for(std::shared_ptr<Future> thread : memory->attached_threads)
                                thread->getResult(); 
                        if(returnSignalHandler)
                            delete returnSignal;
                        return value->shallowCopy();
                    }
                }
            }
            break;
            case NEW:{
                value = MEMGET(memory, command[2]);
                if(value->getType()!=CODE) {
                    std::cerr << "Can only inline a non-called code block" << std::endl;
                    exit(1);
                    value = nullptr;
                }
                else {
                    std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
                    std::shared_ptr<Struct> thisObj = std::make_shared<Struct>(newMemory);
                    thisObj->isMutable = false;
                    newMemory->set("this", thisObj);
                    newMemory->set("locals", std::make_shared<Struct>(newMemory));
                    std::shared_ptr<Code> code = std::static_pointer_cast<Code>(value);
                    value = executeBlock(program, code->getStart(), code->getEnd(), newMemory, nullptr);
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
                std::shared_ptr<List> list = std::make_shared<List>();
                for(int i=2;i<command.size();i++)
                    list->contents->contents.push_back(MEMGET(memory, command[i]));
                value = list;
            }
            break;
            default:
                cmdSize = command.size();
                args.size = cmdSize-2;
                if(cmdSize>2)
                    args.arg0 = MEMGET(memory, command[2]);
                if(cmdSize>3)
                    args.arg1 = MEMGET(memory, command[3]);
                if(cmdSize>4)
                    args.arg2 = MEMGET(memory, command[4]);
                value = Data::run(operation, args);
            break;
        }
        memory->set(command[1], value);
        prevValue = value;
    }
    if(returnSignalHandler)
        for(std::shared_ptr<Future> thread : memory->attached_threads)
            thread->getResult(); 
    if(returnSignalHandler)
        delete returnSignal;
    return prevValue?prevValue->shallowCopy():prevValue;
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
    std::vector<std::shared_ptr<Command>> program;
    if (!inputFile.is_open())  {
        std::cerr << "Unable to open file: " << fileName << std::endl;
        exit(1);
        return 1;
    }

    // organizes each line to a new assembly command
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(std::make_shared<Command>(line));
    inputFile.close();

    // initialize memory and execute the assembly commands
    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    std::shared_ptr<Struct> thisObj = std::make_shared<Struct>(memory);
    thisObj->isMutable = false;
    memory->set("this", thisObj);
    memory->set("locals", std::make_shared<Struct>(memory));
    executeBlock(&program, 0, program.size()-1, memory, nullptr);
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