#include "data/Jitable.h"
#include "data/Iterator.h"
#include "data/Boolean.h"
#include "data/BError.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "BMemory.h"
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "common.h"
#include <unordered_set>
#include <map>
#include <fstream>  // Required for std::ofstream
#include <iostream> // Required for std::cerr and std::cout
#include <cstdlib>  // Required for system()
#include <dlfcn.h>  // Required for dlopen and dlsym
#include <cassert>  // Required for assert

extern BError* OUT_OF_RANGE;


std::string int2type(int type) {
    if(type==TOBB_FLOAT) return "double";
    if(type==TOBB_INT) return "long long int";
    if(type==TOBB_BOOL) return "bool";
    if(type==TOSTR) return "char*";
    return "void";
}


// Class representing a Jitable that returns a primitive
class ReturnPrimitiveJitable : public Jitable {
private:
    Data* primitive;
public:
    explicit ReturnPrimitiveJitable(Data* primitive): primitive(primitive) {}
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override {
        returnValue = primitive;
        returnSignal = true;
        return true;
    }
};


// Class representing a Jitable that returns a primitive
class NextAsExistsJitable : public Jitable {
private:
    int from;
    int next;
    int as;
    int exists;
    bool setNext;
    bool setExists;
public:
    explicit NextAsExistsJitable(int from, int next, int as, int exists, bool setNext, bool setExists): from(from), next(next), as(as), exists(exists), setNext(setNext), setExists(setExists) {}
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override {
        bberror("Internal error: runWithBooleanIntent should always be returning true for NextAsExistsJitable.");
    }

    virtual bool runWithBooleanIntent(BMemory* memory, bool &returnValue) {
        auto iterator = memory->get(from);
        if(iterator->getType()!=ITERATOR) // optimize only for iterators
            return false;
        auto it = static_cast<Iterator*>(iterator);
        Data* nextValue = it->fastNext();
        //if(!nextValue) return false;
        if(!nextValue) {
            BuiltinArgs args;
            args.size = 1;
            args.arg0 = it;
            Result res = it->implement(NEXT, &args, memory);
            nextValue = res.get();
                
            if(setNext) memory->unsafeSet(next, nextValue, nullptr);
            returnValue = nextValue!=OUT_OF_RANGE;
            if(setExists) memory->unsafeSet(exists, returnValue?Boolean::valueTrue:Boolean::valueFalse, nullptr);
            memory->unsafeSet(as, nextValue, nullptr); // set last to optimize with unsafeSet
            return true;
        }
        if(setNext) memory->unsafeSet(next, nextValue, nullptr);
        returnValue = nextValue!=OUT_OF_RANGE;
        if(setExists) memory->unsafeSet(exists, returnValue?Boolean::valueTrue:Boolean::valueFalse, nullptr);
        memory->unsafeSet(as, nextValue, nullptr); // set last to optimize with unsafeSet
        return true;
    }
};




class Compile {
    void *handle;
    void *func;
public:
    Compile(const std::string& code, const std::string& name) {
        std::string filename = "temp.jit.bb";
        std::ofstream(filename+".c") << code;
        int ret = system(("gcc -shared -fPIC ./"+filename+".c -o ./"+filename+".so -O2 -march=native").c_str());
        bbassert(ret == 0, "Compilation failed");
        handle = dlopen(("./"+filename+".so").c_str(), RTLD_LAZY);
        bbassert(handle != nullptr, dlerror());
        bbassert(std::remove(("./"+filename+".so").c_str())==0, "Failed to remove a temporary file");
        bbassert(std::remove(("./"+filename+".c").c_str())==0, "Failed to remove a temporary file");
        func = dlsym(handle, name.c_str());
        bbassert(func != nullptr, dlerror());
    }
    ~Compile() {dlclose(handle);}
    void* get() {return func;}
};


class JitCode : public Jitable {
private:
    Compile compile;
    std::vector<Command*>* program;
    int start;
    int preparationEnd;
    std::map<int, int> arguments;
    size_t expected_size;
public:
    explicit JitCode(const std::string& code, const std::string& func, std::vector<Command*>* program, int start, int preparationEnd, std::map<int, int> arguments): 
        compile(code, func), program(program), start(start), preparationEnd(preparationEnd), arguments(arguments) {
            expected_size = 0;
            for (const auto& [symbol, type] : arguments)
                if(type==TOBB_BOOL) expected_size += sizeof(bool);
                else if(type==TOBB_FLOAT) expected_size += sizeof(double);
                else if(type==TOBB_INT)  expected_size += sizeof(int64_t);
                else bberror("Internal error: some jit functionality has not been implemented yet");
        }
    virtual bool run(BMemory* memory, Data*& returnValue, bool &returnSignal) override {
        // preample
        BuiltinArgs args;
        Data* value = nullptr;
        int i = start;
        try {
            for (; i <= preparationEnd; ++i) {
                handleCommand(program, i, memory, returnSignal, args, returnValue);
                if (returnSignal) return true;
            }
        } 
        catch (const BBError& e) {
            handleExecutionError(program, i, e);
        }
        
        // Transfer arguments to the JIT-compiled function
        std::vector<char> argBuffer(expected_size);
        char* argPtr = argBuffer.data();

        for (const auto& [symbol, type] : arguments) {
            Data* data = memory->get(symbol);
            if (type == TOBB_BOOL) {
                bbassert(data->getType()==BB_BOOL, "Preparation failed to create a bool");
                bool value = static_cast<Boolean*>(data)->getValue();
                std::memcpy(argPtr, &value, sizeof(bool));
                argPtr += sizeof(bool);
            } else if (type == TOBB_FLOAT) {
                bbassert(data->getType()==BB_FLOAT, "Preparation failed to create a float");
                double value = static_cast<BFloat*>(data)->getValue();
                std::memcpy(argPtr, &value, sizeof(double));
                argPtr += sizeof(double);
            } else if (type == TOBB_INT) {
                bbassert(data->getType()==BB_INT, "Preparation failed to create an integer");
                int64_t value = static_cast<Integer*>(data)->getValue();
                std::memcpy(argPtr, &value, sizeof(int64_t));
                argPtr += sizeof(int64_t);
            } else {
                bberror("Unhandled argument type during argument transfer.");
            }
        }

        // Call the function
        int isReturning;
        void* resultPtr = reinterpret_cast<void* (*)(int*, void*)>(compile.get())(&isReturning, argBuffer.data());

        returnSignal = !resultPtr;
        // Based on the returned type, set what is being returned
        if (isReturning == TOBB_INT) {
            int64_t value;
            std::memcpy(&value, resultPtr, sizeof(int64_t));
            returnValue = new Integer(value);
        } else if (isReturning == TOBB_FLOAT) {
            double value;
            std::memcpy(&value, resultPtr, sizeof(double));
            returnValue = new BFloat(value);
        } else if (isReturning == TOBB_BOOL) {
            bool value;
            std::memcpy(&value, resultPtr, sizeof(bool));
            returnValue = value ? Boolean::valueTrue : Boolean::valueFalse;
        } else {
            bberror("Unsupported return type from JIT function.");
        }

        // Free the allocated memory by the JIT function
        std::free(resultPtr);

        
        // close defer statements
        if(returnSignal)
            memory->runFinally();
        return true;
    }
};

// Implementation of the jit function
Jitable* jit(const Code* code) {
    std::vector<Command*>* program = code->getProgram();
    int start = code->getStart();
    int end = code->getEnd();
    int size = end-start;
    if(size==2) {
        Command* c0 = program->at(start);
        Command* c1 = program->at(start+1);
        if(c0->operation==BUILTIN && c1->operation==RETURN && c0->args[0]==c1->args[1]) 
            return new ReturnPrimitiveJitable(c0->value);
    }
    
    if(size==3) {
        Command* c0 = program->at(start);
        Command* c1 = program->at(start+1);
        Command* c2 = program->at(start+2);
        if(c0->operation==NEXT && c1->operation==AS && c2->operation==EXISTS){
            if(c0->args[0]==c1->args[1] && c1->args[0]==c2->args[1])  
                return new NextAsExistsJitable(c0->args[1], c0->args[0], c1->args[0], c2->args[0], !c0->knownLocal[0], !c2->knownLocal[0]);
        }
    }


    std::unordered_map<int, int> assignmentCounts;
    std::unordered_map<int, int> usageCounts;
    std::unordered_map<int, int> knownStates;
    for (int i = start; i <= end; ++i) {
        Command* cmd = program->at(i);
        if(!cmd->args.size() || cmd->args[0]==variableManager.noneId) continue;
        int symbol = cmd->args[0];
        assignmentCounts[cmd->args[0]]++;
        for(int j=1;j<cmd->args.size();++j) usageCounts[cmd->args[j]]++;
        knownStates[cmd->args[0]] = TOCOPY;  // use TOCOPY as an operator that can never be jitted
    }

    int start_jit_from = -1;
    for (int i = start; i < end; ++i) {
        Command* cmd = program->at(i);
        if(!cmd->args.size() || cmd->args[0]==variableManager.noneId) continue;
        int prev_known_state = knownStates[cmd->args[0]];
        knownStates[cmd->args[0]] = cmd->operation;
        if(cmd->operation==BUILTIN && cmd->value->getType()==BB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==BUILTIN && cmd->value->getType()==BB_INT) knownStates[cmd->args[0]] = TOBB_INT;
        if(cmd->operation==BUILTIN && cmd->value->getType()==STRING) knownStates[cmd->args[0]] = TOSTR;
        if(cmd->operation==ADD && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==ADD && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==ADD && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==ADD && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_INT;
        if(cmd->operation==MUL && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==MUL && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==MUL && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==MUL && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_INT;
        if(cmd->operation==SUB && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==SUB && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==SUB && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==SUB && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_INT;
        if(cmd->operation==DIV && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==DIV && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==DIV && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==DIV && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==POW && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==POW && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_FLOAT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==POW && knownStates[cmd->args[1]]==TOBB_FLOAT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_FLOAT;
        if(cmd->operation==POW && knownStates[cmd->args[1]]==TOBB_INT && knownStates[cmd->args[2]]==TOBB_INT) knownStates[cmd->args[0]] = TOBB_INT;

        bool known_state_for_everything = start_jit_from==-1 || prev_known_state==knownStates[cmd->args[0]] || prev_known_state==TOCOPY;
        if(known_state_for_everything)
            for (const auto& [symbol, operation] : knownStates) 
                if (operation != TOBB_FLOAT 
                    && operation != TOBB_INT 
                    && operation != TOBB_BOOL
                    && operation != TOSTR
                    && operation != BUILTIN 
                    && operation != TOCOPY 
                    && usageCounts[cmd->args[0]]) {
                        known_state_for_everything = false;
                        break;
                    }

        //std::cout << "newstate recalc for "<<variableManager.getSymbol(cmd->args[0])<<"\n";
        //for (const auto& [symbol, operation] : knownStates) 
        //    std::cout << variableManager.getSymbol(symbol)<< " "<<int2type(operation)<<"\n";

        
        if(!known_state_for_everything)
            start_jit_from = -1;
        else if(start_jit_from==-1)
            start_jit_from = i;

        assignmentCounts[cmd->args[0]]--;
        for(int j=1;j<cmd->args.size();++j) usageCounts[cmd->args[j]]--;
    }


    // repeat the assignment and usage counts
    std::unordered_map<int, int> totalAssignmentCounts;
    assignmentCounts.clear();
    usageCounts.clear();
    for (int i = start; i <= end; ++i) {
        Command* cmd = program->at(i);
        if(!cmd->args.size() || cmd->args[0]==variableManager.noneId) continue;
        int symbol = cmd->args[0];
        assignmentCounts[cmd->args[0]]++;
        totalAssignmentCounts[cmd->args[0]]++;
        for(int j=1;j<cmd->args.size();++j) usageCounts[cmd->args[j]]++;
        // CAREFUL NOT TO AFFECT THE KNOWN STATE
    }

    if(start_jit_from!=-1 && start_jit_from<end-1) {
        //std::cout << "NEW JIT "<<start<<" to "<<end<<"\n";
        for (int i = start; i <= start_jit_from; ++i) {
            Command* cmd = program->at(i);
            assignmentCounts[cmd->args[0]]--;
            for(int j=1;j<cmd->args.size();++j) usageCounts[cmd->args[j]]--;
            //std::cout << "    " << cmd->toString() << "\n";
        }

        std::map<int, int> arguments;
        std::string body = "#include <stdlib.h>\nvoid* call(int *_bbjitisreturning, void* _bbjitargsvoid) {\n";
        body += "  char* _bbjitargs = (char*)_bbjitargsvoid;\n";
        for (const auto& [symbol, counts] : assignmentCounts) {
            if(totalAssignmentCounts[symbol]==counts) continue;
            arguments[symbol] = knownStates[symbol];
            std::string type = int2type(knownStates[symbol]);
            body += "  "+type+" "+variableManager.getSymbol(symbol)+"=*("+type+"*)_bbjitargs; _bbjitargs+=sizeof("+type+");\n";
        }
        for (const auto& [symbol, counts] : assignmentCounts) {
            if(totalAssignmentCounts[symbol]!=counts) continue;
            std::string type = int2type(knownStates[symbol]);
            body += "  "+type+" "+variableManager.getSymbol(symbol)+";\n";
        }


        for (int i = start_jit_from+1; i<end; ++ i) {
            Command* com = program->at(i);
            if(com->operation==ADD) body += "  "+variableManager.getSymbol(com->args[0])+"="+variableManager.getSymbol(com->args[1])+"+"+variableManager.getSymbol(com->args[2])+";\n";
            else if(com->operation==MUL) body += "  "+variableManager.getSymbol(com->args[0])+"="+variableManager.getSymbol(com->args[1])+"*"+variableManager.getSymbol(com->args[2])+";\n";
            else if(com->operation==SUB) body += "  "+variableManager.getSymbol(com->args[0])+"="+variableManager.getSymbol(com->args[1])+"-"+variableManager.getSymbol(com->args[2])+";\n";
            else if(com->operation==BUILTIN) body += "  "+variableManager.getSymbol(com->args[0])+"="+com->value->toString(nullptr)+";\n";
            else if(com->operation==RETURN){
                std::string type = int2type(knownStates[com->args[1]]);
                body += "  *_bbjitisreturning="+std::to_string(knownStates[com->args[1]])+";\n";
                body += "  "+type+"* _bbjitret=("+type+"*)malloc(sizeof("+type+"));\n";
                body += "  *_bbjitret = "+variableManager.getSymbol(com->args[1])+";\n";
                body += "  return _bbjitret;\n";
            }
            else {
                //std::cout << "cannot jit command: " << com->toString() << "\n";
                return nullptr;
            }    
            //body += "    "+com->toString()+"\n";
        }
        body += "    return 0;\n}";
        //std::cout << body << "\n\n";
        //return nullptr;
        return new JitCode(body, "call", program, start, start_jit_from, arguments);
    }




    return nullptr;
}
