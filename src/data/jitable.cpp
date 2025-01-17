#include "data/Jitable.h"
#include "data/Iterator.h"
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
#include <cassert>  // Required for assert

#ifdef _WIN32
#include <windows.h> // Required for LoadLibrary, GetProcAddress, and FreeLibrary
#else
#include <dlfcn.h>  // Required for dlopen and dlsym on Linux
#endif

extern BError* OUT_OF_RANGE;
extern std::recursive_mutex compileMutex;
bool allowJit = false;


std::string int2type(int type) {
    if(type==TOBB_FLOAT) return "double";
    if(type==TOBB_INT) return "int64_t";
    if(type==TOBB_BOOL) return "bool";
    if(type==TOVECTOR) return "double*";
    //if(type==TOSTR) return "char*";
    return "void";
}


// Class representing a Jitable that returns a primitive
class ReturnPrimitiveJitable : public Jitable {
private:
    DataPtr primitive;
public:
    explicit ReturnPrimitiveJitable(DataPtr primitive): primitive(primitive) {}
    virtual bool run(BMemory* memory, DataPtr& returnValue, bool &returnSignal, bool forceStayInThread) override {
        returnValue = primitive;
        returnSignal = true;
        return true;
    }
    virtual std::string toString() {return "JIT: converted to returning a primitive";}
};


// Class representing a Jitable that returns a primitive
class NextAsExistsJitable : public Jitable {
private:
    int from;
    int next;
    int as;
    int exists;
public:
    explicit NextAsExistsJitable(int from, int next, int as, int exists): from(from), next(next), as(as), exists(exists) {}
    virtual bool run(BMemory* memory, DataPtr& returnValue, bool &returnSignal, bool forceStayInThread) override {
        bberror("Internal error: runWithBooleanIntent should always be returning true for NextAsExistsJitable.");
    }

    virtual bool runWithBooleanIntent(BMemory* memory, bool &returnValue, bool forceStayInThread) override {
        auto iterator = memory->get(from);
        if(iterator->getType()!=ITERATOR) return false;
        auto it = static_cast<Iterator*>(iterator.get());
        DataPtr nextValue = it->fastNext();

        if(!nextValue.islitorexists()) {
            BuiltinArgs args;
            args.size = 1;
            args.arg0 = it;
            Result res = it->implement(NEXT, &args, memory);
            nextValue = res.get();
                
            memory->set(next, nextValue);
            returnValue = nextValue.islit() || nextValue.get()!=OUT_OF_RANGE;
            memory->set(exists, returnValue);
            memory->set(as, nextValue); // set last to optimize with set
            return true;
        }

        memory->set(next, nextValue);
        returnValue = nextValue.islit() || nextValue.get()!=OUT_OF_RANGE;
        memory->set(exists, returnValue);
        memory->set(as, nextValue); // set last to optimize with set
        return true;
    }
    virtual std::string toString() {return "JIT: the sequence of .bbvm instructions next as exists has been optimized to not store intermediate variables";}
};

int compilationCounter = 0;


#ifdef _WIN32
#include <windows.h>
#include <string>

// Helper function to convert std::string to std::wstring
std::wstring toWideString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

class Compile {
    void *handle;
    void *func;
public:
    Compile(const std::string& code, const std::string& name) {
        std::string filename = "temp" + std::to_string(compilationCounter) + ".jit.bb";
        std::ofstream(filename + ".c") << code;

        // Adjust compiler and extensions based on platform
        #ifdef _WIN32
        int ret = system(("gcc -O2 -shared -o ./" + filename + ".dll ./" + filename + ".c").c_str());
        bbassert(ret == 0, "Compilation failed");

        std::wstring wideFilename = toWideString("./" + filename + ".dll");
        handle = LoadLibraryW(wideFilename.c_str());
        bbassert(handle != nullptr, "Failed to load DLL");
        #else
        int ret = system(("gcc -O2 -march=native -shared -fPIC ./" + filename + ".c -o ./" + filename + ".so").c_str());
        bbassert(ret == 0, "Compilation failed");
        handle = dlopen(("./" + filename + ".so").c_str(), RTLD_LAZY);
        bbassert(handle != nullptr, dlerror());
        #endif

        // Cleanup temporary files
        #ifdef _WIN32
        bbassert(std::remove(("./" + filename + ".dll").c_str()) == 0, "Compilation was unable to remove a temporary file");
        #else
        bbassert(std::remove(("./" + filename + ".so").c_str()) == 0, "Compilation was unable to remove a temporary file");
        #endif
        bbassert(std::remove(("./" + filename + ".c").c_str()) == 0, "Compilation was unable to remove a temporary file");

        // Resolve symbol
        #ifdef _WIN32
        func = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name.c_str()));
        bbassert(func != nullptr, "Failed to resolve function in DLL");
        #else
        func = dlsym(handle, name.c_str());
        bbassert(func != nullptr, dlerror());
        #endif
    }

    ~Compile() {
        #ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
        #else
        dlclose(handle);
        #endif
    }

    void* get() { return func; }
};

class JitCode : public Jitable {
private:
    Compile compile;
    std::vector<Command>* program;
    int start;
    int preparationEnd;
    std::unordered_map<int, int> arguments;
    std::vector<int> argumentOrder;
    size_t expected_size;
    int returnType;
public:
    virtual std::string toString() {return "JIT: used gcc to compile away everything after line "+std::to_string(preparationEnd);}

    explicit JitCode(const std::string& code, const std::string& func, std::vector<Command>* program, int start, int preparationEnd, 
                    std::unordered_map<int, int> arguments, std::vector<int> argumentOrder, int returnType): 
        compile(code, func), program(program), start(start), preparationEnd(preparationEnd), arguments(arguments), argumentOrder(argumentOrder), returnType(returnType) {
            expected_size = 0;
            for (int symbol : argumentOrder) {
                int type = arguments[symbol];
                if(type==TOBB_BOOL) expected_size += sizeof(bool);
                else if(type==TOBB_FLOAT) expected_size += sizeof(double);
                else if(type==TOBB_INT)  expected_size += sizeof(int64_t);
                else bberror("Internal error: some jit functionality has not been implemented yet");
            }
        }
    virtual bool run(BMemory* memory, DataPtr& returnValue, bool &returnSignal, bool forceStayInThread) override {
        return false;
        // preample
        /*BuiltinArgs args;
        DataPtr value = nullptr;
        int i = start;
        try {
            for (; i <= preparationEnd; ++i) {
                handleCommand(program, i, memory, returnSignal, args, returnValue, forceStayInThread);
                if (returnSignal) return true;
            }
        } 
        catch (const BBError& e) {
            handleExecutionError(program, i, e);
        }
        
        // Transfer arguments to the JIT-compiled function
        std::vector<char> argBuffer(expected_size);
        char* argPtr = argBuffer.data();

        for (int symbol : argumentOrder) {
            int type = arguments[symbol];
            DataPtr data = memory->get(symbol);
            if (type == TOBB_BOOL) {
                bbassert(data->getType()==BB_BOOL, "Internal JIT error: Preparation failed to create a bool");
                std::memcpy(argPtr, &static_cast<Boolean*>(data.get())->value, sizeof(bool));
                argPtr += sizeof(bool);
            } else if (type == TOBB_FLOAT) {
                bbassert(data->getType()==BB_FLOAT, "Internal JIT error: Preparation failed to create a float");
                std::memcpy(argPtr, &static_cast<BFloat*>(data.get())->value, sizeof(double));
                argPtr += sizeof(double);
            } else if (type == TOBB_INT) {
                bbassert(data->getType()==BB_INT, "Internal JIT error: Preparation failed to create an integer");
                std::memcpy(argPtr, &static_cast<Integer*>(data.get())->value, sizeof(int64_t));
                argPtr += sizeof(int64_t);
            } else {
                bberror("Internal JIT error: Unhandled argument type during argument transfer.");
            }
        }

        // Call the function
        // Based on the returned type, set what is being returned
        returnSignal = true;
        if (returnType == TOBB_INT) {
            int64_t value = reinterpret_cast<int64_t (*)(void*)>(compile.get())(argBuffer.data());
            returnValue = new Integer(value);
        } else if (returnType == TOBB_FLOAT) {
            double value = reinterpret_cast<double (*)(void*)>(compile.get())(argBuffer.data());
            returnValue = new BFloat(value);
        } else if (returnType == TOBB_BOOL) {
            bool value = reinterpret_cast<bool (*)(void*)>(compile.get())(argBuffer.data());
            returnValue = value?Boolean::valueTrue:Boolean::valueFalse;
        } else {
            bberror("Unsupported return type from JIT function.");
        }
        
        // close defer statements
        if(returnSignal)
            memory->runFinally();
        return true;*/
    }
};

// Implementation of the jit function
Jitable* jit(const Code* code) {
    std::lock_guard<std::recursive_mutex> lock(compileMutex);
    std::vector<Command>* program = code->getProgram();
    int start = code->getStart();
    int end = code->getEnd();
    int size = end-start;
    if(size==2) {
        const Command& c0 = program->at(start);
        const Command& c1 = program->at(start+1);
        if(c0.operation==BUILTIN && c1.operation==RETURN && c0.result==c1.arg0) return new ReturnPrimitiveJitable(c0.value);
    }
    
    if(size==3) {
        const Command& c0 = program->at(start);
        const Command& c1 = program->at(start+1);
        const Command& c2 = program->at(start+2);
        if(c0.operation==NEXT && c1.operation==AS && c2.operation==EXISTS) {
            if(c0.result==c1.arg0 && c1.result==c2.arg0) return new NextAsExistsJitable(c0.arg0, c0.result, c1.result, c2.result);
        }
    }

    if(!allowJit) return nullptr;

    std::unordered_map<int, int> assignmentCounts;
    std::unordered_map<int, int> usageCounts;
    std::unordered_map<int, int> knownStates;
    for (int i = start; i <= end; ++i) {
        const Command& cmd = program->at(i);
        if(!cmd.nargs || cmd.result==variableManager.noneId) continue;
        int symbol = cmd.result;
        assignmentCounts[cmd.result]++;
        if(cmd.nargs>1) usageCounts[cmd.arg0]++;
        if(cmd.nargs>2) usageCounts[cmd.arg1]++;
        if(cmd.nargs>3) usageCounts[cmd.arg2]++;
        knownStates[cmd.result] = TOCOPY;  // use TOCOPY as an operator that can never be jitted
    }

    int start_jit_from = -1;
    for (int i = start; i < end; ++i) {
        const Command& cmd = program->at(i);
        if(!cmd.nargs || cmd.result==variableManager.noneId) continue;
        int prev_known_state = knownStates[cmd.result];
        knownStates[cmd.result] = cmd.operation;
        if(cmd.operation==BUILTIN && cmd.value->getType()==BB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==BUILTIN && cmd.value->getType()==BB_INT) knownStates[cmd.result] = TOBB_INT;
        if(cmd.operation==BUILTIN && cmd.value->getType()==STRING) knownStates[cmd.result] = TOSTR;
        if(cmd.operation==ADD && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==ADD && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==ADD && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==ADD && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_INT;
        if(cmd.operation==MUL && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==MUL && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==MUL && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==MUL && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_INT;
        if(cmd.operation==SUB && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==SUB && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==SUB && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==SUB && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_INT;
        if(cmd.operation==DIV && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==DIV && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==DIV && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==DIV && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==POW && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==POW && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_FLOAT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==POW && knownStates[cmd.arg0]==TOBB_FLOAT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_FLOAT;
        if(cmd.operation==POW && knownStates[cmd.arg0]==TOBB_INT && knownStates[cmd.arg1]==TOBB_INT) knownStates[cmd.result] = TOBB_INT;
        //if(cmd.operation==TOLIST) return nullptr;
        
        bool known_state_for_everything = start_jit_from==-1 || prev_known_state==knownStates[cmd.result] || prev_known_state==TOCOPY;
        if(cmd.operation==TOLIST)
            known_state_for_everything = false;
        if(known_state_for_everything)
            for (const auto& [symbol, operation] : knownStates) 
                if (operation != TOBB_FLOAT 
                    && operation != TOBB_INT 
                    && operation != TOBB_BOOL
                    && operation != TOVECTOR
                    && operation != BUILTIN 
                    && operation != TOCOPY 
                    && usageCounts[cmd.result]) {
                        known_state_for_everything = false;
                        break;
                    }

        //std::cout << "newstate recalc for "<<variableManager.getSymbol(cmd.result)<<"\n";
        //for (const auto& [symbol, operation] : knownStates) 
        //    std::cout << variableManager.getSymbol(symbol)<< " "<<int2type(operation)<<"\n";

        
        if(!known_state_for_everything)
            start_jit_from = -1;
        else if(start_jit_from==-1)
            start_jit_from = i;

        assignmentCounts[cmd.result]--;
        if(cmd.nargs>1) usageCounts[cmd.arg0]--;
        if(cmd.nargs>2) usageCounts[cmd.arg1]--;
        if(cmd.nargs>3) usageCounts[cmd.arg2]--;
    }


    // repeat the assignment and usage counts
    std::unordered_map<int, int> totalAssignmentCounts;
    assignmentCounts.clear();
    usageCounts.clear();
    for (int i = start; i <= end; ++i) {
        const Command& cmd = program->at(i);
        if(!cmd.nargs || cmd.result==variableManager.noneId) continue;
        int symbol = cmd.result;
        assignmentCounts[cmd.result]++;
        totalAssignmentCounts[cmd.result]++;
        if(cmd.nargs>1) usageCounts[cmd.arg0]++;
        if(cmd.nargs>2) usageCounts[cmd.arg1]++;
        if(cmd.nargs>3) usageCounts[cmd.arg2]++;
        // CAREFUL NOT TO AFFECT THE KNOWN STATE
    }

    if(start_jit_from!=-1 && start_jit_from<end-1) {
        //std::cout << "NEW JIT "<<start<<" to "<<end<<"\n";
        for (int i = start; i <= start_jit_from; ++i) {
            const Command& cmd = program->at(i);
            assignmentCounts[cmd.result]--;
            if(cmd.nargs>1) usageCounts[cmd.arg0]--;
            if(cmd.nargs>2) usageCounts[cmd.arg1]--;
            if(cmd.nargs>3) usageCounts[cmd.arg2]--;
            //std::cout << "    " << cmd.toString() << "\n";
        }

        std::unordered_map<int, int> arguments;
        std::vector<int> argumentOrder;
        std::string body;
        body += "  char* _bbjitargs = (char*)_bbjitargsvoid;\n";
        for (const auto& [symbol, counts] : assignmentCounts) {
            if(totalAssignmentCounts[symbol]==counts) continue;
            arguments[symbol] = knownStates[symbol];
            argumentOrder.push_back(symbol);
            std::string type = int2type(knownStates[symbol]);
            body += "  "+type+" "+variableManager.getSymbol(symbol)+"=*("+type+"*)_bbjitargs; _bbjitargs+=sizeof("+type+");\n";
        }
        for (const auto& [symbol, counts] : assignmentCounts) {
            if(totalAssignmentCounts[symbol]!=counts) continue;
            std::string type = int2type(knownStates[symbol]);
            body += "  "+type+" "+variableManager.getSymbol(symbol)+";\n";
        }

        int returnType = TOCOPY;
        for (int i = start_jit_from+1; i<end; ++ i) {
            const Command& com = program->at(i);
            if(com.operation==ADD) body += "  "+variableManager.getSymbol(com.result)+"="+variableManager.getSymbol(com.arg0)+"+"+variableManager.getSymbol(com.arg1)+";\n";
            else if(com.operation==MUL) body += "  "+variableManager.getSymbol(com.result)+"="+variableManager.getSymbol(com.arg0)+"*"+variableManager.getSymbol(com.arg1)+";\n";
            else if(com.operation==SUB) body += "  "+variableManager.getSymbol(com.result)+"="+variableManager.getSymbol(com.arg0)+"-"+variableManager.getSymbol(com.arg1)+";\n";
            else if(com.operation==BUILTIN) body += "  "+variableManager.getSymbol(com.result)+"="+com.value->toString(nullptr)+";\n";
            else if(com.operation==RETURN){
                std::string type = int2type(knownStates[com.arg0]);
                //body += "  *_bbjitisreturning="+std::to_string(knownStates[com.arg0])+";\n";
                //body += "  "+type+"* _bbjitret=("+type+"*)malloc(sizeof("+type+"));\n";
                //body += "  *_bbjitret = "+variableManager.getSymbol(com.arg0)+";\n";
                //body += "  return _bbjitret;\n";
                body += "  return "+variableManager.getSymbol(com.arg0)+";\n";
                if(returnType!=TOCOPY && returnType!=knownStates[com.arg0]) return nullptr;  // if different types are returned per case, we cannot JIT
                returnType = knownStates[com.arg0];
            }
            else {
                std::cerr << "Internal error: cannot jit command: " << com.toString() << "\nThis does not indicate critical failure (and will be fixed in the future)\n";
                return nullptr;
            }    
            //body += "    "+com.toString()+"\n";
        }
        body = "#include <stdint.h>\n"+int2type(returnType)+" call(void* _bbjitargsvoid) {\n"+body+"}";
        //std::cout << body << "\n\n";
        //return nullptr;
        try {
            return new JitCode(body, "call", program, start, start_jit_from, arguments, argumentOrder, returnType);
        }
        catch(BBError& e) {
            std::cerr << e.what() << " - JIT is hereby disabled (you might experience slowdowns)\n";
            allowJit = false;
            return nullptr;
        }
    }




    return nullptr;
}
