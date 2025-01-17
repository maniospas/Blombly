#include <iostream>
#include "common.h"
#include <memory>

#include "common.h"
#include "data/Struct.h"
#include "data/Code.h"
#include "data/Future.h"
#include "data/BFile.h"
#include "data/List.h"
#include "data/Iterator.h"
#include "data/Vector.h"
#include "data/BHashMap.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "data/BError.h"
#include "data/Integer.h"
#include "data/RestServer.h"
#include "data/Jitable.h"
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"

#define WHILE_WITH_CODE_BLOCKS

extern BError* NO_TRY_INTERCEPT;

std::string replaceEscapeSequences(const std::string& input) {
    std::string output;
    size_t pos = 0;
    while (pos < input.size()) {
        // Look for \e[ in the string
        size_t found = input.find("\\e[", pos);
        if (found != std::string::npos) {
            // Copy everything up to the \e[
            output.append(input, pos, found - pos);

            // Replace \e with the ANSI escape code \033
            output += '\033';

            // Find the end of the ANSI sequence (looking for 'm')
            size_t end = input.find('m', found);
            if (end != std::string::npos) {
                // Copy the rest of the ANSI sequence including 'm'
                output.append(input, found + 2, end - found - 2 + 1);
                pos = end + 1;
            } else {
                // If no 'm' is found, treat it as an incomplete sequence
                output += input.substr(found + 2);
                break;
            }
        } else {
            // No more \e[ found, copy the rest of the string
            output.append(input, pos, input.size() - pos);
            break;
        }
    }
    return output;
}


std::chrono::steady_clock::time_point program_start;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;
std::unordered_map<int, DataPtr> cachedData;

#define DISPATCH_LITERAL(expr) {int carg = command.args[0]; result=DataPtr(expr); if(carg!=variableManager.noneId) memory.unsafeSetLiteral(carg, result); goto SKIP_ASSIGNMENT;}
#define DISPATCH_RESULT(expr) {int carg = command.args[0]; result=DataPtr(expr); if(carg!=variableManager.noneId) memory.set(carg, result); goto SKIP_ASSIGNMENT;}
#define DISPATCH_COMPUTED_RESULT {int carg = command.args[0]; if(carg!=variableManager.noneId) memory.set(carg, result); goto SKIP_ASSIGNMENT;}

#define DISPATCH(OPERATION) goto *dispatch_table[OPERATION]
void initialize_dispatch_table() {}


struct CompiledCommand {
    OperationType operation;
};



Result ExecutionInstance::run(Code* code) {
    //bbassert(code->getProgram()==&program, "Internal error: it should be impossible to change the global program pointer.");
    auto& program = *code->getProgram();
    int end = code->getEnd();
    int i = code->getStart();
    try {
    for(;i<=end;++i) {
    const Command& command = program[i];

    //std::cout << command.toString() << "\n";
    
    /*
    NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG,
    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOMAP, TOBB_INT, TOBB_FLOAT, TOSTR, TOBB_BOOL, TOFILE,
    SUM, MAX, MIN,
    BUILTIN, BEGIN, BEGINFINAL, BEGINCACHE, END, RETURN, FINAL, IS,
    CALL, WHILE, IF, NEW, BB_PRINT, INLINE, GET, SET, SETFINAL, DEFAULT,
    TIME, TOITER, TRY, CATCH, FAIL, EXISTS, READ, CREATESERVER, AS, TORANGE, 
    DEFER, CLEAR, MOVE, ISCACHED, TOSQLITE, TOGRAPHICS
    */

    static void* dispatch_table[] = {
        &&DO_NOT,
        &&DO_AND,
        &&DO_OR,
        &&DO_EQ,
        &&DO_NEQ,
        &&DO_LE,
        &&DO_GE,
        &&DO_LT,
        &&DO_GT,
        &&DO_ADD,
        &&DO_SUB,
        &&DO_MUL,
        &&DO_MMUL,
        &&DO_DIV,
        &&DO_MOD,
        &&DO_LEN,
        &&DO_POW,
        &&DO_LOG,
        &&DO_PUSH,
        &&DO_POP,
        &&DO_NEXT,
        &&DO_PUT,
        &&DO_AT,
        &&DO_SHAPE,
        &&DO_TOVECTOR,
        &&DO_TOLIST,
        &&DO_TOMAP,
        &&DO_TOBB_INT,
        &&DO_TOBB_FLOAT,
        &&DO_TOSTR,
        &&DO_TOBB_BOOL,
        &&DO_TOFILE,
        &&DO_SUM,
        &&DO_MAX,
        &&DO_MIN,
        &&DO_BUILTIN,
        &&DO_BEGIN,
        &&DO_BEGINFINAL,
        &&DO_BEGINCACHE,
        &&DO_END,
        &&DO_RETURN,
        &&DO_FINAL,
        &&DO_IS,
        &&DO_CALL,
        &&DO_WHILE,
        &&DO_IF,
        &&DO_NEW,
        &&DO_BB_PRINT,
        &&DO_INLINE,
        &&DO_GET,
        &&DO_SET,
        &&DO_SETFINAL,
        &&DO_DEFAULT,
        &&DO_TIME,
        &&DO_TOITER,
        &&DO_TRY,
        &&DO_CATCH,
        &&DO_FAIL,
        &&DO_EXISTS,
        &&DO_READ,
        &&DO_CREATESERVER,
        &&DO_AS,
        &&DO_TORANGE,
        &&DO_DEFER,
        &&DO_CLEAR,
        &&DO_MOVE,
        &&DO_ISCACHED,
        &&DO_TOSQLITE,
        &&DO_TOGRAPHICS,
    };


    DISPATCH(command.operation);
    DO_ADD: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()+arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()+arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()+arg1.unsafe_toint()));
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()+arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for add("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_SUB: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()-arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()-arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()-arg1.unsafe_toint()));
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()-arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for sub("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_MUL: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()*arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()*arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()*arg1.unsafe_toint()));
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()*arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for mul("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_DIV: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()/(double)arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()/arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()/arg1.unsafe_toint()));
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()/arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for div("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_POW: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(std::pow(arg0.unsafe_toint(), arg1.unsafe_toint()));
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(std::pow(arg0.unsafe_toint(), arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(std::pow(arg0.unsafe_tofloat(), arg1.unsafe_toint()));
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(std::pow(arg0.unsafe_tofloat(), arg1.unsafe_tofloat()));
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for pow("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_MOD: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint() % arg1.unsafe_toint());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for mod("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_LT: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()<arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()<arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for lt("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_GT: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()>arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()>arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for gt("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_LE: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()<=arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()<=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<=arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<=arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for le("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_GE: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()>=arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()>=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>=arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>=arg1.unsafe_tofloat());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for ge("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_EQ: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()==arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()==arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()==arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()==arg1.unsafe_tofloat());
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool()==arg1.unsafe_tobool());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for eq("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_NEQ: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isint() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_toint()!=arg1.unsafe_toint());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()!=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()!=arg1.unsafe_toint());
        if(arg0.isfloat() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_tofloat()!=arg1.unsafe_tofloat());
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool()!=arg1.unsafe_tobool());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for neq("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_AND: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool() && arg1.unsafe_tobool());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for and("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_OR: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool() && arg1.unsafe_tobool());
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for or("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_MMUL: {
        const auto& arg0 = memory.get(command.args[1]);
        const auto& arg1 = memory.get(command.args[2]);
        if(arg0.islit() && arg1.islit()) bberror("There was no implementation for mmul("+arg0.torepr()+", "+arg1.torepr()+")");
        args.arg0 = arg0;
        args.arg1 = arg1;
        args.size = 2;
        goto FALLBACK;
    }
    DO_BUILTIN: {
        DISPATCH_RESULT(command.value);
    }
    DO_TOVECTOR: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.isint()) DISPATCH_RESULT(new Vector(arg0.unsafe_toint()));
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_LOG: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.isfloat()) DISPATCH_RESULT(std::log(arg0.unsafe_tofloat()));
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_TOBB_INT: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.isint()) DISPATCH_LITERAL(arg0);
        if(arg0.isfloat()) DISPATCH_LITERAL((int64_t)arg0.unsafe_tofloat());
        if(arg0.isbool()) DISPATCH_LITERAL((int64_t)arg0.unsafe_tobool());
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_TOBB_FLOAT: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.isint()) DISPATCH_LITERAL((double)arg0.unsafe_toint());
        if(arg0.isfloat()) DISPATCH_LITERAL(arg0);
        if(arg0.isbool()) DISPATCH_LITERAL((double)arg0.unsafe_tobool());
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_TOBB_BOOL: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.isint()) DISPATCH_LITERAL((bool)arg0.unsafe_toint());
        if(arg0.isfloat()) DISPATCH_LITERAL((bool)arg0.unsafe_tofloat());
        if(arg0.isbool()) DISPATCH_LITERAL(arg0);
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_BB_PRINT:{
        const auto& printable = memory.get(command.args[1]);
        std::string printing = printable.exists()?printable->toString(&memory):printable.torepr();
        printing = replaceEscapeSequences(printing);
        printing += "\n";
        std::lock_guard<std::recursive_mutex> lock(printMutex);
        std::cout << printing;
        result = nullptr;
        goto SKIP_ASSIGNMENT;
    }
    DO_READ:{
        std::string printing;
        if(command.nargs>1) {
            DataPtr printable = memory.get(command.args[1]);
            if(printable.exists()) {
                std::string out = printable.exists()?printable->toString(&memory):printable.torepr();
                printing += out+" ";
            }
        }
        {
            std::lock_guard<std::recursive_mutex> lock(printMutex);
            std::cout << printing;
            printing = "";
            std::getline(std::cin, printing);
        }
        DISPATCH_RESULT(new BString(printing));
    }
    DO_END: {
        goto SKIP_ASSIGNMENT;
    }
    DO_INLINE: {
        const auto& source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(CODE)) {
            auto code = static_cast<Code*>(source.get());
            Result returnedValue = run(code);
            DISPATCH_RESULT(returnedValue.get());
        }
        if(source.existsAndTypeEquals(STRUCT)) {
            memory.pull(static_cast<Struct*>(source.get())->getMemory());
            DISPATCH_RESULT(source);
        }
        bberror("Can only inline a code block or struct");
    }
    DO_TOSTR: {
        const auto& arg0 = memory.get(command.args[1]);
        if(arg0.islit()) DISPATCH_RESULT(new BString(arg0.torepr()));
        if(arg0.exists()) DISPATCH_RESULT(new BString(arg0->toString(&memory)));
        bberror("Internal error: failed to convert to string. This error should never appear.");
        args.arg0 = arg0;
        args.size = 1;
        goto FALLBACK;
    }
    DO_RETURN: {
        returnSignal = true;
        DISPATCH_RESULT(command.args[1] == variableManager.noneId ? DataPtr::NULLP : memory.get(command.args[1]));
    }
    DO_ISCACHED: {
        result = cachedData[command.args[1]];
        bbassert(result.exists(), "Missing cache value (typically cached due to optimization):" + variableManager.getSymbol(command.args[1]));
        DISPATCH_COMPUTED_RESULT;
    }
    DO_IS: {
        result = memory.get(command.args[1], true);
        if(result.existsAndTypeEquals(ERRORTYPE)) bberror(enrichErrorDescription(command, result->toString(&memory)));
        DISPATCH_COMPUTED_RESULT;
    }
    DO_AS: {
        result = memory.getOrNull(command.args[1], true);
        if(result.existsAndTypeEquals(ERRORTYPE)) static_cast<BError*>(result.get())->consume();
        DISPATCH_COMPUTED_RESULT;
    }
    DO_EXISTS: {
        const auto& res = memory.getOrNull(command.args[1], true);
        DISPATCH_RESULT(!res.existsAndTypeEquals(ERRORTYPE));
    }
    DO_SET: {
        const auto& obj = memory.get(command.args[1]);
        bbassert(obj.existsAndTypeEquals(STRUCT), "Can only set fields in a struct.");
        auto structObj = static_cast<Struct*>(obj.get());
        std::lock_guard<std::recursive_mutex> lock(structObj->memoryLock);
        auto setValue = memory.getOrNullShallow(command.args[3]);
        if(setValue.existsAndTypeEquals(CODE)) setValue = static_cast<Code*>(setValue.get())->copy();
        auto structMemory = structObj->getMemory();
        structMemory->set(command.args[2], setValue);//structmemory.getOrNullShallow(command.args[2]));
        result = nullptr;
        goto SKIP_ASSIGNMENT;
    }
    DO_SETFINAL: {
        const auto& obj = memory.get(command.args[1]);
        bbassert(obj.existsAndTypeEquals(STRUCT), "Can only set fields in a struct.");
        bberror("Cannot set final fields in a struct using field access operators (. or \\). This also ensures that finals can only be set during `new` statements.");
        goto SKIP_ASSIGNMENT;
    } 
    DO_WHILE: {
        const DataPtr& condition = memory.get(command.args[1]);
        const DataPtr& body = memory.get(command.args[2]);
        bbassert(body.existsAndTypeEquals(CODE), "While body can only be a code block.");
        bbassert(body.existsAndTypeEquals(CODE), "While condition can only be a code block.");
        auto codeBody = static_cast<Code*>(body.get());
        auto codeCondition = static_cast<Code*>(condition.get());
        Jitable* jitableCondition = codeCondition->jitable;
        bool checkValue(true);
        while(checkValue) {
            if(!jitableCondition || !jitableCondition->runWithBooleanIntent(&memory, checkValue, forceStayInThread)) {
                Result returnedValue = run(codeCondition);
                const auto& check = returnedValue.get();
                if (returnSignal) DISPATCH_RESULT(check);  // {result = check;SET_RESULT;break;}
                bbassert(check.isbool(), "While condition did not evaluate to bool");
                checkValue = check.unsafe_tobool();
            }
            if(!checkValue) break;
            Result returnedValueFromBody = run(codeBody);
            if (returnSignal) DISPATCH_RESULT(returnedValueFromBody.get());
        }
        result = nullptr;
        goto SKIP_ASSIGNMENT;
    }
    DO_IF: {
        const auto& condition = memory.get(command.args[1]);
        bbassert(condition.isbool(), "If condition did not evaluate to bool");

        if(condition.unsafe_tobool()) {
            const auto& accept = memory.get(command.args[2]);
            if(accept.existsAndTypeEquals(CODE)) {
                Code* code = static_cast<Code*>(accept.get());
                Result returnedValue = run(code);
                DISPATCH_RESULT(returnedValue.get());
            }
            else DISPATCH_RESULT(accept);
        } 
        else if(command.nargs>3) {
            const auto& reject = memory.get(command.args[3]);
            if (reject.existsAndTypeEquals(CODE)) {
                Code* code = static_cast<Code*>(reject.get());
                Result returnedValue = run(code);
                DISPATCH_RESULT(returnedValue.get());
            }
            else DISPATCH_RESULT(reject);
        }
        goto SKIP_ASSIGNMENT;
    }
    DO_CREATESERVER: {
        const auto& port = memory.get(command.args[1]);
        bbassert(port.isint(), "The server's port must be an integer.");
        auto res = new RestServer(port.unsafe_toint());
        res->runServer();
        DISPATCH_RESULT(res);
    }
    DO_FINAL: {
        memory.setFinal(command.args[1]);
        goto SKIP_ASSIGNMENT;
    }
    DO_TRY: {
        memory.detach(memory.parent);
        bool prevReturnSignal = returnSignal;
        try {
            const auto& condition = memory.get(command.args[1]);
            bbassert(condition.existsAndTypeEquals(CODE), "Can only inline a non-called code block for try condition");
            auto codeCondition = static_cast<Code*>(condition.get());
            Result returnedValue = run(codeCondition);
            memory.detach(memory.parent);
            if(!returnSignal) result = NO_TRY_INTERCEPT;
            returnSignal = prevReturnSignal;
            DISPATCH_RESULT(returnedValue.get());
        }
        catch (const BBError& e) {
            returnSignal = prevReturnSignal;
            DISPATCH_RESULT(new BError(std::move(enrichErrorDescription(command, e.what()))));
        }
    }
    DO_CATCH: {
        const auto& condition = memory.getOrNull(command.args[1], true); //(command.knownLocal[1]?memory.getOrNullShallow(command.args[1]):memory.getOrNull(command.args[1], true)); //memory.get(command.args[1]);
        const auto& accept = memory.get(command.args[2]);
        const auto& reject = command.nargs>3?memory.get(command.args[3]):nullptr;
        bbverify(accept.exists(), !accept.exists() || accept.existsAndTypeEquals(CODE), "Can only inline a code block for catch acceptance");
        bbverify(reject.exists(), !reject.exists() || reject.existsAndTypeEquals(CODE), "Can only inline a code block for catch rejection");
        auto codeAccept = static_cast<Code*>(accept.get());
        auto codeReject = static_cast<Code*>(reject.get());
        
        if(condition.existsAndTypeEquals(ERRORTYPE)) { //&& !((BError*)condition)->isConsumed()) {
            static_cast<BError*>(condition.get())->consume();
            if(codeAccept) {
                Result returnValue = run(codeAccept);
                DISPATCH_RESULT(returnValue.get());
            }
        }
        else if(codeReject) {
            Result returnValue = run(codeReject);
            DISPATCH_RESULT(returnValue.get());
        }
        goto SKIP_ASSIGNMENT;
    }
    DO_FAIL: {
        const auto& result = memory.get(command.args[1]);
        bberror(std::move(enrichErrorDescription(command, result->toString(&memory))));
        goto SKIP_ASSIGNMENT;
    }
    DO_DEFER: {
        const auto& source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(CODE)) bberror("Finally can only inline a code block or struct");
        memory.addFinally(static_cast<Code*>(source.get()));
        goto SKIP_ASSIGNMENT;
    }
    DO_DEFAULT: {
        const auto& source = memory.get(command.args[1]);
        bbassert(source.existsAndTypeEquals(CODE), "Can only call `default` on a code block");
        auto code = static_cast<Code*>(source.get());
        BMemory newMemory(&memory, LOCAL_EXPECTATION_FROM_CODE(code));
        ExecutionInstance executor(code, &newMemory, forceStayInThread);
        Result returnedValue = executor.run(code);
        if(executor.hasReturned())  bberror("Cannot return from within a `default` statement");
        memory.replaceMissing(&newMemory);
        goto SKIP_ASSIGNMENT;
    }
    DO_NEW: {
        DataPtr source = memory.get(command.args[1]);
        bbassert(source.existsAndTypeEquals(CODE), "Can only call `new` on a code block");
        auto code = static_cast<Code*>(source.get());
        auto newMemory = new BMemory(&memory, LOCAL_EXPECTATION_FROM_CODE(code));
        auto thisObj = new Struct(newMemory); 
        newMemory->set(variableManager.thisId, thisObj);
        newMemory->setFinal(variableManager.thisId);
        ExecutionInstance executor(code, newMemory, forceStayInThread);
        Result returnedValue = executor.run(code);
        newMemory->detach(nullptr);
        if(returnedValue.get().get()!=thisObj) {
            if(command.args[0]!=variableManager.noneId) memory.set(command.args[0], result);
            thisObj->removeFromOwner(); // do this after setting
            goto SKIP_ASSIGNMENT;
        }
        DISPATCH_COMPUTED_RESULT;
    }
    DO_TOLIST: {
        int n = command.nargs;
        auto list = new BList(n-1);
        for(int j=1;j<n;j++) {
            const DataPtr& element = memory.get(command.args[j]);
            if(element.existsAndTypeEquals(ERRORTYPE)) bberror("Cannot push an error to a list");
            if(element.exists()) element->addOwner();
            list->contents.push_back(element);
        }
        DISPATCH_RESULT(list);
    }
    DO_TOMAP: {
        DISPATCH_RESULT(new BHashMap());
    }
    DO_TIME: {
        result = DataPtr(static_cast<double>(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count()));
        DISPATCH_COMPUTED_RESULT;
    }
    DO_PUSH: {
        args.arg0 = memory.get(command.args[1]);
        args.arg1 = memory.get(command.args[2]);
        args.size = 2;
        goto FALLBACK;
    }
    DO_PUT: {
        args.arg0 = memory.get(command.args[1]);
        args.arg1 = memory.get(command.args[2]);
        args.size = 2;
        goto FALLBACK;
    }
    DO_TOGRAPHICS: {
        args.arg0 = memory.get(command.args[1]);
        args.arg1 = memory.get(command.args[2]);
        args.arg2 = memory.get(command.args[3]);
        args.size = 3;
        goto FALLBACK;
    }
    DO_SUM:
    DO_NOT: 
    DO_LEN: 
    DO_MAX:
    DO_MIN: 
    DO_POP: 
    DO_NEXT: 
    DO_MOVE: 
    DO_CLEAR: 
    DO_SHAPE: 
    DO_AT: 
    DO_TOFILE: 
    DO_TOSQLITE:
    DO_TOITER: {
        args.arg0 = memory.get(command.args[1]);
        args.size = 1;
        goto FALLBACK;
    }
    DO_TORANGE: {
        bbassert(command.nargs>=0, "Range requires at least one argument");
        const auto& arg0 = memory.get(command.args[1]);
        if(command.nargs<=2 && arg0.isint()) DISPATCH_RESULT(new IntRange(0, arg0.unsafe_toint(), 1));
        const auto& arg1 = memory.get(command.args[2]);
        if(command.nargs<=3 && arg0.isint() && arg1.isint()) DISPATCH_RESULT(new IntRange(arg0.unsafe_toint(), arg1.unsafe_toint(), 1));
        const auto& arg2 = memory.get(command.args[3]);
        if(command.nargs<=4 && arg0.isint() && arg1.isint() && arg2.isint()) DISPATCH_RESULT(new IntRange(arg0.unsafe_toint(), arg1.unsafe_toint(), arg2.unsafe_toint()));
        if(command.nargs<=4 && arg0.isfloat() && arg1.isfloat() && arg2.isfloat()) DISPATCH_RESULT(new IntRange(arg0.unsafe_tofloat(), arg1.unsafe_tofloat(), arg2.unsafe_tofloat()));
        bberror("Invalid range arguments: up to three integers are needed or exactly three floats");
    }
    DO_GET: {
        BMemory* from(nullptr);
        const auto& objFound = memory.getOrNull(command.args[1], true);
        if(!objFound.exists()) {
            bbassert(command.args[1]==variableManager.thisId, "Missing value: " + variableManager.getSymbol(command.args[1]));
            from = &memory;
            result = from->get(command.args[2]);
        }
        else {
            bbassert(objFound.existsAndTypeEquals(STRUCT), "Can only get elements from structs, not"+std::to_string(objFound->getType()));
            auto obj = static_cast<Struct*>(objFound.get());
            std::lock_guard<std::recursive_mutex> lock(obj->memoryLock);
            from = obj->getMemory();
            result = from->get(command.args[2]);
            if(result.existsAndTypeEquals(CODE)) memory.codeOwners[static_cast<Code*>(result.get())] = obj;
        }
        DISPATCH_COMPUTED_RESULT;
    }
    DO_BEGINCACHE: {
        int pos = i + 1;
        int depth = 0;
        OperationType command_type;
        while(pos <= program.size()) {
            command_type = program[pos].operation;
            if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
            if(command_type == END) {
                if (depth == 0) break;
                depth--;
            }
            pos++;
        }
        bbassert(depth >= 0, "Cache declaration never ended.");
        auto cache = new Code(&program, i + 1, pos);
        BMemory cacheMemory(nullptr, 16, nullptr);
        ExecutionInstance cacheExecutor(cache, &cacheMemory, forceStayInThread);
        cacheExecutor.run(cache);
        cacheMemory.await();
        bbassert(!cacheExecutor.hasReturned(), "Cache declaration cannot return a value");
        /*for (const DataPtr& key : cacheMemory.contents) {
            bbassert(!obj.existsAndTypeEquals(STRUCT), "Structs cannot be cached");
            obj->addOwner();
            cachedData[key] = obj;
        }*/
        result = nullptr;
        goto SKIP_ASSIGNMENT;
    }
    DO_BEGIN:
    DO_BEGINFINAL: {
        // Start a block of code
        if(command.value.exists()) {
            auto code = static_cast<Code*>(command.value.get());
            result = code;
            int carg = command.args[0];
            if(carg!=variableManager.noneId) memory.set(carg, result);
            if (command.operation == BEGINFINAL) memory.setFinal(command.args[0]); // WE NEED TO SET FINALS ONLY AFTER THE VARIABLE IS SET
            i = code->getEnd();
            goto SKIP_ASSIGNMENT;
        }
        // Find the matching END for this block
        int pos = i + 1;
        int depth = 0;
        OperationType command_type;
        while(pos <= program.size()) {
            command_type = program[pos].operation;
            if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
            if(command_type == END) {
                if(depth == 0) break;
                depth--;
            }
            pos++;
        }
        bbassert(depth >= 0, "Code block never ended.");
        auto cache = new Code(&program, i + 1, pos);
        cache->addOwner();
        cache->jitable = jit(cache);
        command.value = cache;
        result = cache;
        i = pos;
        int carg = command.args[0];
        if(carg!=variableManager.noneId) memory.set(carg, result);
        if (command.operation == BEGINFINAL) memory.setFinal(command.args[0]); // WE NEED TO SET FINALS ONLY AFTER THE VARIABLE IS SET
        i = cache->getEnd();
        goto SKIP_ASSIGNMENT;
    }
    DO_CALL: {
        // Function or method call
        const auto& context = command.args[1] == variableManager.noneId ? nullptr : memory.get(command.args[1]);
        DataPtr called = memory.get(command.args[2]);
        bbassert(called.exists(), "Cannot call a missing value or literal.");
        if(called->getType()!=CODE && called->getType()!=STRUCT) {
            args.size = 2;
            args.arg0 = called;
            args.arg1 = context;
            Result returnValue = Data::run(command.operation, &args, &memory);
            result = returnValue.get();
            DISPATCH_COMPUTED_RESULT;
        }
        if(called->getType()==STRUCT) {
            auto strct = static_cast<Struct*>(called.get());
            auto val = strct->getMemory()->getOrNullShallow(variableManager.callId);
            bbassert(val.existsAndTypeEquals(CODE), "Struct was called like a method but has no implemented code for `call`.");
            //static_cast<Code*>(val)->scheduleForParallelExecution = false; // struct calls are never executed in parallel
            memory.codeOwners[static_cast<Code*>(val.get())] = static_cast<Struct*>(called.get());
            called = (val);
        }
        auto code = static_cast<Code*>(called.get());
        if(true || forceStayInThread || !code->scheduleForParallelExecution || !Future::acceptsThread()) {
            BMemory newMemory(&memory, LOCAL_EXPECTATION_FROM_CODE(code));
            if(context.exists()) {
                bbassert(context.existsAndTypeEquals(CODE), "Call context must be a code block.");
                Code* callCode = static_cast<Code*>(context.get());
                ExecutionInstance executor(callCode, &newMemory, forceStayInThread);
                Result returnedValue = executor.run(callCode);
                if(executor.hasReturned()) DISPATCH_RESULT(returnedValue.get());
            }
            auto it = memory.codeOwners.find(code);
            const auto& thisObj = (it != memory.codeOwners.end() ? it->second->getMemory() : &memory)->getOrNull(variableManager.thisId, true);
            if(thisObj.exists()) newMemory.set(variableManager.thisId, thisObj);
            std::unique_lock<std::recursive_mutex> executorLock;
            if(thisObj.exists()) {
                bbassert(thisObj.existsAndTypeEquals(STRUCT), "Internal error: `this` was neither a struct nor missing (in the last case it would have been replaced by the scope)");
                //if(!forceStayInThread) 
                executorLock = std::unique_lock<std::recursive_mutex>(static_cast<Struct*>(thisObj.get())->memoryLock);
            }
            newMemory.allowMutables = false;
            bool forceStayInThread = thisObj.exists(); // overwrite the option
            ExecutionInstance executor(code, &newMemory, forceStayInThread);
            Result returnedValue = executor.run(code);
            result = returnedValue.get();
            if(thisObj.exists()) newMemory.set(variableManager.thisId, nullptr);
            DISPATCH_COMPUTED_RESULT;
        } 
        else { // 
            auto newMemory = new BMemory(&memory, LOCAL_EXPECTATION_FROM_CODE(code));
            if(context.exists()) {
                bbassert(context.existsAndTypeEquals(CODE), "Call context must be a code block.");
                ExecutionInstance executor(static_cast<Code*>(context.get()), newMemory, forceStayInThread);
                Result returnedValue = executor.run(static_cast<Code*>(context.get()));
                if(executor.hasReturned()) {
                    result = returnedValue.get();
                    DISPATCH_COMPUTED_RESULT;
                }
            }
            //newmemory.detach(memory);
            auto it = memory.codeOwners.find(code);
            const auto& thisObj = (it != memory.codeOwners.end() ? it->second->getMemory() : &memory)->getOrNull(variableManager.thisId, true);
            if(thisObj.exists()) newMemory->set(variableManager.thisId, thisObj);
            newMemory->allowMutables = false;
            auto futureResult = new ThreadResult();
            auto future = new Future(futureResult);
            future->addOwner();//the attached_threads are also an owner
            memory.attached_threads.insert(future);
            futureResult->start(code, newMemory, futureResult, command, thisObj);
            result = future;
            DISPATCH_COMPUTED_RESULT;
        }
    }

    /*
    *  CASES END HERE
    */

    FALLBACK: {
        Result returnValue = Data::run(command.operation, &args, &memory);
        DISPATCH_RESULT(returnValue.get());
    }

    SKIP_ASSIGNMENT:

    if (returnSignal) {
        Result res(result);
        memory.runFinally();
        return std::move(res);
    }
    }//end loop
    }//end try 
    catch (const BBError& e) {handleExecutionError(program[i], e);}
    return std::move(Result(result));
}
