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
#include "utils/stringtrim.cpp"
#include "utils/builtins.cpp"
#include "utils/parser.cpp"



class Command {
public:
    std::vector<std::string> args;
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
    }
};

class ThreadReturn {
public:
    std::shared_ptr<Data> value;
    ThreadReturn(std::shared_ptr<Data> val){value = val;}
};


class Memory {
private:
    std::shared_ptr<Memory> parent;
    std::unordered_map<std::string, std::shared_ptr<Data>> data;
    bool allowMutables;
    pthread_mutex_t memoryLock; 
public:
    Memory() {
        parent=nullptr;
        allowMutables=true;
        if (pthread_mutex_init(&memoryLock, NULL) != 0) 
            std::cerr << "Failed to create a mutex for memory read/write" << std::endl;
    }
    Memory(std::shared_ptr<Memory> par): parent(par) {
        allowMutables=true;
        if (pthread_mutex_init(&memoryLock, NULL) != 0) 
            std::cerr << "Failed to create a mutex for struct read/write" << std::endl;
    }
    void lock() {
        pthread_mutex_lock(&memoryLock);
        if(parent)
            parent->lock();
    }
    void unlock(){
        pthread_mutex_unlock(&memoryLock);
        if(parent)
            parent->unlock();
    }
    std::shared_ptr<Data> get(std::string item) {
        return get(item, true);
    }
    void pull(std::shared_ptr<Memory> other) {
        for (auto it = other->data.begin(); it != other->data.end(); it++)
            //if(it->first!="this")
                set(it->first, it->second);
    }
    void replaceMissing(std::shared_ptr<Memory> other) {
        for (auto it = other->data.begin(); it != other->data.end(); it++)
            //if(it->first!="this")
                if(!data[it->first])
                    set(it->first, it->second);
    }
    std::shared_ptr<Data> get(std::string item, bool allowMutable) {
        if(item=="#")
            return nullptr;
        std::shared_ptr<Data> ret;
        lock();
        ret = data[item];
        unlock();
        if(ret && ret->getType() == FUTURE) {
            //ThreadReturn* result;
            //int thread = std::static_pointer_cast<Future>(ret)->getThread();
            //threads->join(thread, (void**)&result);
            //pthread_join(std::static_pointer_cast<Future>(ret)->getThread(), (void**)&result);
            //if(result && result->value)
            //    result->value->isMutable = ret->isMutable;
            //if(result)
             //   ret = result->value;
            /*else {
                std::cerr << "Method failed to return anything\n";
                ret = nullptr;
            }*/
            ret = std::static_pointer_cast<Future>(ret)->getResult();
            lock();
            data[item] = ret;
            unlock();
            //if(result)
            //    delete result;
        }
        if(ret && !allowMutable && ret->isMutable) {
            std::cerr << "Mutable symbol can not be accessed from nested block (make it final or access it from an object): " + item << std::endl;
            return nullptr;
        }
        if(!ret && parent)
            ret = parent->get(item, allowMutables);
        if(!ret)
            std::cerr << "Missing value: " + item << std::endl;
        return ret;
    }
    void set(std::string item, std::shared_ptr<Data> value) {
        if(item=="#") 
            return;
        lock();
        if(data[item]!=nullptr && !data[item]->isMutable) {
            bool couldBeShallowCopy = data[item]->couldBeShallowCopy(value);
            unlock();
            if(!couldBeShallowCopy)
                std::cerr << "Cannot overwrite final value: " + item << std::endl;
            return;
        } 
        data[item] = value;
        unlock();
    }
    void detach() {
        allowMutables = false;
        parent = nullptr;
    }
    void detach(std::shared_ptr<Memory> par) {
        allowMutables = false;
        parent = par;
    }
};

class ThreadData{
public:
    ThreadData(){}
    std::shared_ptr<Memory> memory;
    int start;
    int end;
    std::vector<std::shared_ptr<Command>>* program;
};


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
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(all.size()==1 && all[0]->getType()==STRUCT && operation=="copy")
            return std::make_shared<Struct>(memory);
        throw Unimplemented();
    }
};



std::shared_ptr<Data> executeBlock(std::vector<std::shared_ptr<Command>>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory, 
                  bool *returnSignal);


void* thread_function(void *args){
    pthread_t self_id = pthread_self();
    ThreadData* data = (ThreadData*)args;
    std::shared_ptr<Data> value = executeBlock(
        data->program, 
        data->start, 
        data->end, 
        data->memory,
        nullptr);
    delete data;
    ThreadReturn* ret = new ThreadReturn(value);
    //pthread_exit(ret);
    return ret;
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
    for(int i=start;i<=end;i++) {
        auto command = program->at(i)->args;
        std::shared_ptr<Data> value;
        if(command[0]==BUILTIN) {
            if(command[2][0]=='"')
                value = std::make_shared<BString>(command[2].substr(1, command[2].size()-2));
            else if(command[2][0]=='I')
                value = std::make_shared<Integer>(std::atoi(command[2].substr(1).c_str()));
            else if(command[2][0]=='F')
                value = std::make_shared<Float>(std::atof(command[2].substr(1).c_str()));
            else if(command[2][0]=='B') 
                value = std::make_shared<Boolean>(command[2]=="Btrue");
            else
                std::cerr << "Unable to understand builtin value: " << command[2] << std::endl;
            
        }
        else if(command[0]==FINAL) {
            MEMGET(memory, command[2])->isMutable = false;
        }
        else if(command[0]==PRINT)  {
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
        else if(command[0]==BEGIN || command[0]==BEGINFINAL) {
            int pos = i+1;
            int depth = 0;
            std::string command_type;
            while(pos<=end) {
                command_type = program->at(pos)->args[0];
                if(command_type==BEGIN || command_type==BEGINFINAL)
                    depth += 1;
                if(command_type==END) {
                    if(depth==0)
                        break;
                    depth -= 1;
                }
                pos += 1;
            }
            if(depth>0)
                std::cerr << "Unclosed code block" << std::endl;
            value = std::make_shared<Code>(i+1, pos, memory);
            if(command[0]==BEGINFINAL)
                value->isMutable = false;
            i = pos;
        }
        else if(command[0]==END) {
            //return std::make_shared<MacroResult>(memory);
            break;
        }/*
        else if(command[0]=="CALL") {
            // create new writable memory under the current context
            std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
            std::shared_ptr<Data> context = MEMGET(memory, command[2]);
            std::shared_ptr<Data> execute = MEMGET(memory, command[3]);
            // check if the call has some context, and if so, execute it in the new memory
            if(context && context->getType()=="code") {
                std::shared_ptr<Code> code = std::static_pointer_cast<Code>(context);
                value = executeBlock(program, code->getStart(), code->getEnd(), newMemory);
                if(value) // show an erroe message if the context returned with anything other than END
                    std::cerr << "Code execution context should not return a value." << std::endl;
            }
            // execute the called code in the new memory
            std::shared_ptr<Code> code = std::static_pointer_cast<Code>(execute);
            value = executeBlock(program, code->getStart(), code->getEnd(), newMemory);
        }*/
        else if(command[0]==CALL) {
            // create new writable memory under the current context
            std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
            newMemory->set("locals", std::make_shared<Struct>(newMemory));
            std::shared_ptr<Data> context = MEMGET(memory, command[2]);
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
            newMemory->detach(code->getDeclarationMemory());
            //newMemory->set("this", std::make_shared<Struct>(code->getDeclarationMemory()));
            
            // execute the called code in the new memory
            //pthread_t thread_id;
            //ThreadData* data = new ThreadData();
            //data->start = code->getStart();
            //data->end = code->getEnd();
            //data->program = program;
            //data->memory = newMemory;
            //thread_id = threads->enqueue(&thread_function, data);
            //pthread_create(&thread_id, NULL, &thread_function, data);
            std::future<std::shared_ptr<Data>> val = std::async(executeBlock, program, code->getStart(), code->getEnd(), newMemory, nullptr);
            std::shared_ptr<FutureData> futureData = std::make_shared<FutureData>(std::move(val));
            value = std::make_shared<Future>(futureData);
        }
        else if(command[0]==RETURN) {
            // make return copy the object (NOTE: copying only reallocates (and changes) wrapper properties like finality, not the internal value - internal memory of structs will be the same)
            value = MEMGET(memory, command[2]);
            *returnSignal = true;
            if(returnSignalHandler)
                delete returnSignal;
            return value?value->shallowCopy():value;
        }
        else if(command[0]==GET) {
            value = MEMGET(memory, command[2]);
            if(value->getType()!=STRUCT) {
                std::cerr << "Can only get fields from a struct" << std::endl;
                value = nullptr;
            }
            else {
                std::shared_ptr<Struct> obj = std::static_pointer_cast<Struct>(value);
                //obj->lock(); TODO
                value = obj->getMemory()->get(command[3]);
                //obj->unlock(); TODO
                value = value?value->shallowCopy():value;
            }
        }
        else if(command[0]==IS) {
            value = MEMGET(memory, command[1]);
            memory->set(command[2], value);
        }
        else if(command[0]==SET) {
            value = MEMGET(memory, command[2]);
            if(value->getType()!=STRUCT) {
                std::cerr << "Can only set fields in a struct" << std::endl;
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
        }
        else if(command[0]==WHILE) {
            std::shared_ptr<Data> condition = MEMGET(memory, command[2]);
            std::shared_ptr<Data> accept = MEMGET(memory, command[3]);
            if(condition->getType()!=CODE) 
                std::cerr << "Can only inline a non-called code block for while condition" << std::endl;
            else if(accept->getType()!=CODE) 
                std::cerr << "Can only inline a non-called code block for while loop" << std::endl;
            else {
                std::shared_ptr<Code> codeCondition = std::static_pointer_cast<Code>(condition);
                std::shared_ptr<Code> codeAccept = accept?std::static_pointer_cast<Code>(accept):nullptr;
                while(true) {
                    std::shared_ptr<Data> check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal);
                    if(*returnSignal) {
                        if(returnSignalHandler)
                            delete returnSignal;
                        return check->shallowCopy();
                    }
                    if(check->getType()!=BOOL) {
                        std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
                        break;
                    }
                    else if(std::static_pointer_cast<Boolean>(check)->getValue()) 
                        value = executeBlock(program, codeAccept->getStart(), codeAccept->getEnd(), memory, returnSignal);
                    else
                        break;
                    if(*returnSignal) {
                        if(returnSignalHandler)
                            delete returnSignal;
                        return value->shallowCopy();
                    }
                }
            }
        }
        else if(command[0]==IF) {
            std::shared_ptr<Data> condition = MEMGET(memory, command[2]);
            std::shared_ptr<Data> accept = MEMGET(memory, command[3]);
            std::shared_ptr<Data> reject = command.size()>4?MEMGET(memory, command[4]):nullptr;
            if(condition->getType()!=CODE) 
                std::cerr << "Can only inline a non-called code block for if condition" << std::endl;
            else if(accept->getType()!=CODE) 
                std::cerr << "Can only inline a non-called code block for if acceptance" << std::endl;
            else if(reject && reject->getType()!=CODE) 
                std::cerr << "Can only inline a non-called code block for if rejection" << std::endl;
            else {
                std::shared_ptr<Code> codeCondition = std::static_pointer_cast<Code>(condition);
                std::shared_ptr<Code> codeAccept = accept?std::static_pointer_cast<Code>(accept):nullptr;
                std::shared_ptr<Code> codeReject = reject?std::static_pointer_cast<Code>(reject):nullptr;
                std::shared_ptr<Data> check = executeBlock(program, codeCondition->getStart(), codeCondition->getEnd(), memory, returnSignal);
                if(*returnSignal) {
                    if(returnSignalHandler)
                        delete returnSignal;
                    return check->shallowCopy();
                }
                if(check->getType()!=BOOL)
                    std::cerr << "Logical condition failed to evaluate to bool" << std::endl;
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
                        delete returnSignal;
                    return value->shallowCopy();
                }
            }
        }
        else if(command[0]==INLINE) {
            value = MEMGET(memory, command[2]);
            if(value->getType()==STRUCT) {
                std::shared_ptr<Struct> code = std::static_pointer_cast<Struct>(value);
                memory->pull(code->getMemory());
            }
            else if(value->getType()!=CODE) {
                std::cerr << "Can only inline a non-called code block or struct" << std::endl;
                value = nullptr;
            }
            else {
                std::shared_ptr<Code> code = std::static_pointer_cast<Code>(value);
                value = executeBlock(program, code->getStart(), code->getEnd(), memory, returnSignal);
                if(*returnSignal){
                    if(returnSignalHandler)
                        delete returnSignal;
                    return value->shallowCopy();
                }
            }
        }
        else if(command[0]==DEFAULT) {
            value = MEMGET(memory, command[2]);
            if(value->getType()==STRUCT) {
                std::shared_ptr<Struct> code = std::static_pointer_cast<Struct>(value);
                memory->replaceMissing(code->getMemory());
            }
            else if(value->getType()!=CODE) {
                std::cerr << "Can only inline a non-called code block or struct" << std::endl;
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
                        delete returnSignal;
                    return value->shallowCopy();
                }
            }
        }
        else if(command[0]==NEW) {
            value = MEMGET(memory, command[2]);
            if(value->getType()!=CODE) {
                std::cerr << "Can only inline a non-called code block" << std::endl;
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
        else if(command[0]=="List") {
            std::shared_ptr<List> list = std::make_shared<List>();
            for(int i=2;i<command.size();i++)
                list->contents->contents.push_back(MEMGET(memory, command[i]));
            value = list;
        }
        else  {
            std::vector<std::shared_ptr<Data>> args;
            for(int i=2;i<command.size();i++)
                args.push_back(MEMGET(memory, command[i]));
            value = Data::run(command[0], args);
        }
        memory->set(command[1], value);
        prevValue = value;
    }
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

    // run the assembly file in the virtual machine
    return vm(fileName, threads);
}