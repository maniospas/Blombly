/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
#include "data/BString.h"
#include "data/BError.h"
#include "data/RestServer.h"
#include "data/Database.h"
#include "data/BFile.h"
#include "data/Jitable.h"
#include "data/Graphics.h"
#include "interpreter/Command.h"
#include "interpreter/functional.h"
#include "interpreter/thread.h"
#include "math.h"

extern BError* NO_TRY_INTERCEPT;
std::chrono::steady_clock::time_point program_start;
double wallclock_start;
std::vector<SymbolWorries> symbolUsage;
std::mutex ownershipMutex;
std::recursive_mutex printMutex;
std::recursive_mutex compileMutex;
BMemory cachedData(0, nullptr, 32);
extern BError* OUT_OF_RANGE;

#define DISPATCH_LITERAL(expr) {int carg = command.args[0]; result=DataPtr(expr); if(carg!=variableManager.noneId) [[likely]] memory.unsafeSetLiteral(carg, result); continue;}
#define DISPATCH_RESULT(expr) {int carg = command.args[0]; result=DataPtr(expr); if(carg!=variableManager.noneId) [[likely]] memory.set(carg, result); continue;}
#define DISPATCH_OUTCOME(expr) {int carg = command.args[0]; Result res(expr); result=res.get(); if(carg!=variableManager.noneId) [[likely]] memory.set(carg, result); continue;}
#define DISPATCH_COMPUTED_RESULT {int carg = command.args[0]; if(carg!=variableManager.noneId) [[likely]] memory.set(carg, result); continue;}
#define RUN_IF_RETURN(expr) {if (expr.returnSignal) [[unlikely]] {memory.runFinally();return expr;} continue;}

#define DISPATCH(OPERATION) goto *dispatch_table[OPERATION]
void initialize_dispatch_table() {}

void ExecutionInstance::runNoReturn(Code* code) {
    const auto& program = *code->getProgram();
    auto returnedValue = run(program, code->getStart(), code->getOptimizedEnd());
    bbassert(!returnedValue.returnSignal, "The virtual machine cannot return a value.");
}

ExecutionInstanceRunReturn ExecutionInstance::run(Code* code) {
    const auto& program = *code->getProgram();
    return run(program, code->getStart(), code->getOptimizedEnd());
}

ExecutionInstanceRunReturn ExecutionInstance::run(const std::vector<Command>& program, size_t i, size_t end) {
    DataPtr arg0, arg1;
    for(;i<=end;++i) {
    const Command& command = program[i];
    try {

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

    //Use labels-as-values on non-MSVC (the latter does not support dynamic dispatch)
    #ifndef _MSC_VER
    static const void* dispatch_table[] = {
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
        &&DO_RANDOM,
        &&DO_RANDVECTOR,
        &&DO_ZEROVECTOR,
        &&DO_ALLOCVECTOR,
        &&DO_LISTELEMENT,
        &&DO_GATHER
    };
    DISPATCH(command.operation);
    #else
    switch (command.operation) {                                         \
        case 0:  goto DO_NOT;                                    \
        case 1:  goto DO_AND;                                    \
        case 2:  goto DO_OR;                                     \
        case 3:  goto DO_EQ;                                     \
        case 4:  goto DO_NEQ;                                    \
        case 5:  goto DO_LE;                                     \
        case 6:  goto DO_GE;                                     \
        case 7:  goto DO_LT;                                     \
        case 8:  goto DO_GT;                                     \
        case 9:  goto DO_ADD;                                    \
        case 10: goto DO_SUB;                                    \
        case 11: goto DO_MUL;                                    \
        case 12: goto DO_MMUL;                                   \
        case 13: goto DO_DIV;                                    \
        case 14: goto DO_MOD;                                    \
        case 15: goto DO_LEN;                                    \
        case 16: goto DO_POW;                                    \
        case 17: goto DO_LOG;                                    \
        case 18: goto DO_PUSH;                                   \
        case 19: goto DO_POP;                                    \
        case 20: goto DO_NEXT;                                   \
        case 21: goto DO_PUT;                                    \
        case 22: goto DO_AT;                                     \
        case 23: goto DO_SHAPE;                                  \
        case 24: goto DO_TOVECTOR;                               \
        case 25: goto DO_TOLIST;                                 \
        case 26: goto DO_TOMAP;                                  \
        case 27: goto DO_TOBB_INT;                               \
        case 28: goto DO_TOBB_FLOAT;                             \
        case 29: goto DO_TOSTR;                                  \
        case 30: goto DO_TOBB_BOOL;                              \
        case 31: goto DO_TOFILE;                                 \
        case 32: goto DO_SUM;                                    \
        case 33: goto DO_MAX;                                    \
        case 34: goto DO_MIN;                                    \
        case 35: goto DO_BUILTIN;                               \
        case 36: goto DO_BEGIN;                                  \
        case 37: goto DO_BEGINFINAL;                             \
        case 38: goto DO_BEGINCACHE;                             \
        case 39: goto DO_END;                                    \
        case 40: goto DO_RETURN;                                 \
        case 41: goto DO_FINAL;                                  \
        case 42: goto DO_IS;                                     \
        case 43: goto DO_CALL;                                   \
        case 44: goto DO_WHILE;                                  \
        case 45: goto DO_IF;                                     \
        case 46: goto DO_NEW;                                    \
        case 47: goto DO_BB_PRINT;                               \
        case 48: goto DO_INLINE;                                 \
        case 49: goto DO_GET;                                    \
        case 50: goto DO_SET;                                    \
        case 51: goto DO_SETFINAL;                               \
        case 52: goto DO_DEFAULT;                                \
        case 53: goto DO_TIME;                                   \
        case 54: goto DO_TOITER;                                 \
        case 55: goto DO_TRY;                                    \
        case 56: goto DO_CATCH;                                  \
        case 57: goto DO_FAIL;                                   \
        case 58: goto DO_EXISTS;                                 \
        case 59: goto DO_READ;                                   \
        case 60: goto DO_CREATESERVER;                           \
        case 61: goto DO_AS;                                     \
        case 62: goto DO_TORANGE;                                \
        case 63: goto DO_DEFER;                                  \
        case 64: goto DO_CLEAR;                                  \
        case 65: goto DO_MOVE;                                   \
        case 66: goto DO_ISCACHED;                               \
        case 67: goto DO_TOSQLITE;                               \
        case 68: goto DO_TOGRAPHICS;                             \
        case 69: goto DO_RANDOM;                             \
        case 70: goto DO_RANDVECTOR;                         \
        case 71: goto DO_ZEROVECTOR;                         \
        case 72: goto DO_ALLOCVECTOR;                        \
        case 73: goto DO_LISTELEMENT;                        \
        case 74: goto DO_GATHER;                             \
        default: throw std::runtime_error("Invalid operation");  \
    }
    #endif


    DO_ADD: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()+arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()+arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()+arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()+arg1.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->add(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->add(&memory, arg0));
        bberror("There was no implementation for add("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_SUB: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()-arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()-arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()-arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()-arg1.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->sub(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->rsub(&memory, arg0));
        bberror("There was no implementation for sub("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_MUL: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()*arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()*arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()*arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()*arg1.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->mul(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->mul(&memory, arg0));
        bberror("There was no implementation for mul("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_DIV: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()/(double)arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()/arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL((double)(arg0.unsafe_toint()/arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL((double)(arg0.unsafe_tofloat()/arg1.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->div(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->rdiv(&memory, arg0));
        bberror("There was no implementation for div("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_POW: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(std::pow(arg0.unsafe_toint(), arg1.unsafe_toint()));
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(std::pow(arg0.unsafe_tofloat(), arg1.unsafe_tofloat()));
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(std::pow(arg0.unsafe_toint(), arg1.unsafe_tofloat()));
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(std::pow(arg0.unsafe_tofloat(), arg1.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->pow(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->rpow(&memory, arg0));
        bberror("There was no implementation for pow("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_MOD: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint() % arg1.unsafe_toint());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->mod(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->rmod(&memory, arg0));
        bberror("There was no implementation for mod("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_LT: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()<arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()<arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()<arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<arg1.unsafe_toint());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->lt(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->ge(&memory, arg0));
        bberror("There was no implementation for lt("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_GT: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()>arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()>arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()>arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>arg1.unsafe_toint());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->gt(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->le(&memory, arg0));
        bberror("There was no implementation for gt("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_LE: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()<=arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()<=arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()<=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()<=arg1.unsafe_toint());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->le(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->gt(&memory, arg0));
        bberror("There was no implementation for le("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_GE: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()>=arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()>=arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()>=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()>=arg1.unsafe_toint());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->ge(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->lt(&memory, arg0));
        bberror("There was no implementation for ge("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_EQ: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()==arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()==arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()==arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()==arg1.unsafe_toint());
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool()==arg1.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->eq(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->neq(&memory, arg0));
        bberror("There was no implementation for eq("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_NEQ: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isintint(arg1)) DISPATCH_LITERAL(arg0.unsafe_toint()!=arg1.unsafe_toint());
        if(arg0.isfloatfloat(arg1)) DISPATCH_LITERAL(arg0.unsafe_tofloat()!=arg1.unsafe_tofloat());
        if(arg0.isint() && arg1.isfloat()) DISPATCH_LITERAL(arg0.unsafe_toint()!=arg1.unsafe_tofloat());
        if(arg0.isfloat() && arg1.isint()) DISPATCH_LITERAL(arg0.unsafe_tofloat()!=arg1.unsafe_toint());
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool()!=arg1.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->neq(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->eq(&memory, arg0));
        bberror("There was no implementation for neq("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_AND: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool() && arg1.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->opand(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->opand(&memory, arg0));
        bberror("There was no implementation for and("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_OR: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.isbool() && arg1.isbool()) DISPATCH_LITERAL(arg0.unsafe_tobool() || arg1.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->opor(&memory, arg1));
        if(arg1.exists()) DISPATCH_OUTCOME(arg1->opor(&memory, arg0));
        bberror("There was no implementation for or("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_MMUL: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->mmul(&memory, arg1));
        bberror("There was no implementation for mmul("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_BUILTIN: {
        DISPATCH_RESULT(command.value);
    }
    DO_TOVECTOR: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.existsAndTypeEquals(VECTOR)) DISPATCH_RESULT(arg0);
        if(arg0.existsAndTypeEquals(LIST)) DISPATCH_RESULT(static_cast<BList*>(arg0.get())->toVector(&memory));
        /*if(arg0.existsAndTypeEquals(ITERATOR)) {
            if(command.args.size()>2) {
                int id2 = command.args[2];
                arg1 = memory.get(id2);
                bbassert(arg1.isint(), "vector(iter, int) did not have an int second argument");
                DISPATCH_RESULT(static_cast<Iterator*>(arg0.get())->toVector(&memory, arg1.unsafe_toint()));
            }
            DISPATCH_RESULT(static_cast<Iterator*>(arg0.get())->toVector(&memory));
        }*/
        if(arg0.isint()) bberror("Missing operation: vector(int). Did you mean one of: `vector::zero(int)`, `vector::alloc(int)` or `vector(iter, int)`.");
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bberror("Only lists or vectors can be casted to vector.");
    }
    DO_ZEROVECTOR: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_RESULT(new Vector(arg0.unsafe_toint(), true));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bberror("1vector::zero` can only have an int size argument.");
    }
    DO_RANDVECTOR: {
        bberror("vector::consume not implemented yet");
    }
    DO_ALLOCVECTOR: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_RESULT(new Vector(arg0.unsafe_toint()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bberror("vector::alloc can only have an int size argument");
    }
    DO_LISTELEMENT: {
        int n = command.nargs;
        auto list = new BList(n-1);
        for(int j=1;j<n;j++) {
            const DataPtr& element = memory.get(command.args[j]);
            if(element.existsAndTypeEquals(ERRORTYPE)) {
                delete list;
                throw BBError(static_cast<BError*>(element.get())->consume()->toString(nullptr));
            }
            if(element.exists()) element->addOwner();
            list->contents.push_back(element);
        }
        DISPATCH_RESULT(list);
    }
    DO_LOG: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint())DISPATCH_LITERAL(log(arg0.unsafe_toint()));
        if(arg0.isfloat())DISPATCH_LITERAL(log(arg0.unsafe_tofloat()));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: log("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->logarithm(&memory));
    }
    DO_TOBB_INT: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_LITERAL((int64_t)arg0.unsafe_toint());
        if(arg0.isfloat()) DISPATCH_LITERAL((int64_t)arg0.unsafe_tofloat());
        if(arg0.isbool()) DISPATCH_LITERAL((int64_t)arg0.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_LITERAL(arg0->toInt(&memory));
        bberror("There was no implementation for int("+arg0.torepr()+")");
    }
    DO_TOBB_FLOAT: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_LITERAL((double)arg0.unsafe_toint());
        if(arg0.isfloat()) DISPATCH_LITERAL((double)arg0.unsafe_tofloat());
        if(arg0.isbool()) DISPATCH_LITERAL((double)arg0.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_LITERAL(arg0->toFloat(&memory));
        bberror("There was no implementation for int("+arg0.torepr()+")");
    }
    DO_TOBB_BOOL: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_LITERAL((bool)arg0.unsafe_toint());
        if(arg0.isfloat()) DISPATCH_LITERAL((bool)arg0.unsafe_tofloat());
        if(arg0.isbool()) DISPATCH_LITERAL((bool)arg0.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_LITERAL(arg0->toBool(&memory));
        bberror("There was no implementation for int("+arg0.torepr()+")");
    }
    DO_BB_PRINT:{ 
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        std::string printing = arg0.exists()?arg0->toString(&memory):arg0.torepr();
        printing = printing;
        printing += "\n";
        std::lock_guard<std::recursive_mutex> lock(printMutex);
        std::cout << printing;
        result = DataPtr::NULLP;
        continue;
    }
    DO_READ:{
        std::string printing;
        if(command.nargs>1) {
            int id1 = command.args[1];
            arg0 = memory.get(id1);
            if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
            if(arg0.islitorexists()) {
                std::string out = arg0.exists()?arg0->toString(&memory):arg0.torepr();
                printing += out+" ";
            }
        }
        {
            std::lock_guard<std::recursive_mutex> lock(printMutex);
            std::cout << printing;
            printing = "";
            if (!std::getline(std::cin, printing)) bberror("Failed to read from the console");
        }
        DISPATCH_RESULT(new BString(printing));
    }
    DO_END: {
        continue;
    }
    DO_INLINE: {
        const auto& source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(CODE)) {
            auto code = static_cast<Code*>(source.get());
            auto returnedValue = run(code);
            RUN_IF_RETURN(returnedValue);
        }
        if(source.existsAndTypeEquals(STRUCT)) {
            static_cast<Struct*>(source.get())->transferToMemory(&memory);
            DISPATCH_RESULT(source);
        }
        if(source.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(source.get())->consume()->toString(nullptr));
        bberrorexplain("Unexpected value: "+arg0.torepr(), "Only code blocks or structs can be inlined.", "");
    }
    DO_TOSTR: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.isint()) DISPATCH_RESULT(new BString(std::to_string(arg0.unsafe_toint())));
        if(arg0.isfloat()) DISPATCH_RESULT(new BString(std::to_string(arg0.unsafe_tofloat())));
        if(arg0.isbool()) DISPATCH_RESULT(new BString(arg0.unsafe_tobool()?"true":"false"));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg0.exists()) DISPATCH_RESULT(new BString(arg0->toString(&memory)));
        bberrorexplain("Unexpected value: "+arg0.torepr(), "This value cannot be converted to string. However, this message can appear only due to an internal error.", "");
    }
    DO_RETURN: {
        arg0 = command.args[1] == variableManager.noneId ? DataPtr::NULLP : memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        return ExecutionInstanceRunReturn(true, Result(arg0));
    }
    DO_ISCACHED: {
        if(command.value.islitorexists()) {
            DISPATCH_RESULT(command.value);
        }
        result = cachedData.getOrNullShallow(command.args[1]);
        command.value = result.get();
        command.value.existsAddOwner();
        bbassertexplain(result.islitorexists(), "Missing cache value:" + variableManager.getSymbol(command.args[1]), "Cache values are created by blombly's optimization. This message can appear only due to an internal error.", "");
        DISPATCH_COMPUTED_RESULT;
    }
    DO_IS: {
        memory.directTransfer(command.args[0], command.args[1]);
        continue;
    }
    DO_AS: {
        result = memory.getOrNull(command.args[1], true);
        if(result.existsAndTypeEquals(ERRORTYPE)) static_cast<BError*>(result.get())->consume();
        DISPATCH_COMPUTED_RESULT;
    }
    DO_EXISTS: {
        const auto& res = memory.getOrNull(command.args[1], true);
        if(res.isptr() && !res.exists()) DISPATCH_LITERAL(false);
        DISPATCH_RESULT(!res.existsAndTypeEquals(ERRORTYPE));
    }
    DO_SET: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(!arg0.existsAndTypeEquals(STRUCT)) [[unlikely]] {
            if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        }
        auto structObj = static_cast<Struct*>(arg0.get());
        std::lock_guard<std::recursive_mutex> lock(structObj->memoryLock);
        auto setValue = memory.get(command.args[3]);
        if(setValue.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(setValue.get())->consume()->toString(nullptr));
        if(setValue.existsAndTypeEquals(CODE)) setValue = static_cast<Code*>(setValue.get())->copy();
        structObj->set(command.args[2], setValue);//structmemory.getOrNullShallow(command.args[2]));
        result = nullptr;
        continue;
    }
    DO_SETFINAL: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(!arg0.existsAndTypeEquals(STRUCT)) bberror(arg0.existsAndTypeEquals(ERRORTYPE)?arg0->toString(nullptr):"Can only set fields in a struct.");
        bberror("Cannot set final fields in a struct using field access operators (. or \\). This also ensures that finals can only be set during `new` statements.");
        continue;
    } 
    DO_WHILE: {
        int id1 = command.args[1];
        int id2 = command.args[2];
        arg0 = memory.get(id1);
        arg1 = memory.get(id2);
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg1.existsAndTypeEquals(CODE), "Unexpected value: "+arg1.torepr(), "While body can only be a code block.", "");
        bbassertexplain(arg0.existsAndTypeEquals(CODE), "Unexpected value: "+arg0.torepr(), "While condition can only be a code block.", "");
        auto codeBody = static_cast<Code*>(arg1.get());
        auto codeCondition = static_cast<Code*>(arg0.get());
        Jitable* jitableCondition = codeCondition->jitable;
        //Jitable* jitableBody = codeBody->jitable;
        bool checkValue(true);
        int codeBodyStart = codeBody->getStart();
        int codeBodyEnd = codeBody->getOptimizedEnd();
        int codeConditionStart = codeCondition->getStart();
        int codeConditiionEnd = codeCondition->getOptimizedEnd();
        while(checkValue) {
            if(!jitableCondition || !jitableCondition->runWithBooleanIntent(&memory, checkValue, forceStayInThread)) {
                auto returnedValue = run(program, codeConditionStart, codeConditiionEnd);
                const auto& check = returnedValue.get();
                if(check.existsAndTypeEquals(ERRORTYPE)) throw BBError(check->toString(nullptr));
                bbassertexplain(check.isbool(), "Unexpected value: "+arg0.torepr(), "While condition can only evaluate to bool.", "");
                checkValue = check.unsafe_tobool();
                if (returnedValue.returnSignal) [[unlikely]] {
                    memory.runFinally();
                    return returnedValue;
                }
            }
            if(!checkValue) break;
            /*if(jitableBody) {
                bool shouldReturn;
                jitableBody->run(&memory, result, shouldReturn, forceStayInThread);
                continue;
            }*/
            auto returnedValueFromBody = run(program, codeBodyStart, codeBodyEnd);
            if(returnedValueFromBody.get().existsAndTypeEquals(ERRORTYPE)) throw BBError(returnedValueFromBody.get()->toString(nullptr));
            RUN_IF_RETURN(returnedValueFromBody);
        }
        continue;
    }
    DO_IF: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.isbool(), "Unexpected value: "+arg0.torepr(), "If condition can only evaluate to bool.", "");

        if(arg0.unsafe_tobool()) {
            const auto& accept = memory.get(command.args[2]);
            if(accept.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(accept.get())->consume()->toString(nullptr));
            if(accept.existsAndTypeEquals(CODE)) {
                Code* code = static_cast<Code*>(accept.get());
                auto returnedValue = run(program, code->getStart(), code->getOptimizedEnd());
                RUN_IF_RETURN(returnedValue);
            }
            else bberrorexplain("Unexpected value: "+accept.torepr(), "If body can only be a code block. Did you mean to return from it?", "");
        } 
        else if(command.nargs>3) {
            const auto& reject = memory.get(command.args[3]);
            if(reject.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(reject.get())->consume()->toString(nullptr));
            if (reject.existsAndTypeEquals(CODE)) {
                Code* code = static_cast<Code*>(reject.get());
                auto returnedValue = run(program, code->getStart(), code->getOptimizedEnd());
                RUN_IF_RETURN(returnedValue);
            }
            else bberrorexplain("Unexpected value: "+reject.torepr(), "Else body can only be a code block. Did you mean to return from it?", "");
        }
        continue;
    }
    DO_CREATESERVER: {
        const auto& port = memory.get(command.args[1]);
        if(port.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(port.get())->consume()->toString(nullptr));
        bbassertexplain(port.isint(), "Unexpected value: "+port.torepr(), "The server's port can only be an integer.", "");
        auto res = new RestServer(&memory, port.unsafe_toint());
        DISPATCH_RESULT(res);
    }
    DO_FINAL: {
        memory.setFinal(command.args[1]);
        continue;
    }
    DO_TRY: {
        const auto& condition = memory.get(command.args[1]);
        if(condition.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(condition.get())->consume()->toString(nullptr));
        bbassertexplain(condition.existsAndTypeEquals(CODE), "Unexpected value: "+condition.torepr(), "Can only inline a non-called code block for try condition", "");
        auto codeCondition = static_cast<Code*>(condition.get());
        try {
            auto returnedValue = run(codeCondition);
            const auto& ret = returnedValue.returnSignal?returnedValue.get():new BError(std::move(enrichErrorDescription(command, NO_TRY_INTERCEPT->toString(nullptr))));
            //if(ret.existsAndTypeEquals(ERRORTYPE)) static_cast<BError*>(ret.get())->consume();
            DISPATCH_OUTCOME(ret);
        }
        catch (const BBError& e) {
            auto ret = new BError(std::move(enrichErrorDescription(command, e.what())));
            //ret->consume();
            DISPATCH_RESULT(ret);
        }
    }
    DO_CATCH: {
        const auto& condition = memory.getOrNull(command.args[1], true); //(command.knownLocal[1]?memory.getOrNullShallow(command.args[1]):memory.getOrNull(command.args[1], true)); //memory.get(command.args[1]);
        const auto& accept = memory.get(command.args[2]);
        const auto& reject = command.nargs>3?memory.get(command.args[3]):DataPtr::NULLP;
        if(accept.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(accept.get())->consume()->toString(nullptr));
        if(reject.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(reject.get())->consume()->toString(nullptr));
        bbassertexplain(accept.existsAndTypeEquals(CODE), "Unexpected value: "+accept.torepr(), "Can only inline a code block for catch acceptance.", "");
        bbassertexplain(reject==DataPtr::NULLP || reject.existsAndTypeEquals(CODE), "Unexpected value: "+reject.torepr(), "Can only inline a code block for catch else.", "");
        auto codeAccept = static_cast<Code*>(accept.get());
        auto codeReject = static_cast<Code*>(reject.get());
        
        if(condition.isptr() && (condition==DataPtr::NULLP || condition->getType()==ERRORTYPE)) { //&& !((BError*)condition)->isConsumed()) {
            if(condition.exists()) static_cast<BError*>(condition.get())->consume();
            if(codeAccept) {
                auto returnValue = run(codeAccept);
                RUN_IF_RETURN(returnValue);
            }
        }
        else if(codeReject) {
            auto returnValue = run(codeReject);
            RUN_IF_RETURN(returnValue);
        }
        continue;
    }
    DO_FAIL: {
        const auto& result = memory.get(command.args[1]);
        if(result.existsAndTypeEquals(ERRORTYPE)) throw BBError(result->toString(&memory));
        bberror(std::move(result->toString(&memory)));
        continue;
    }
    DO_DEFER: {
        const auto& source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(source.get())->consume()->toString(nullptr));
        bbassertexplain(source.existsAndTypeEquals(CODE), "Unexpected value: "+source.torepr(), "Defer can only inline a code block.", "");
        memory.addFinally(static_cast<Code*>(source.get()));
        continue;
    }
    DO_DEFAULT: {
        const auto& source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(source.get())->consume()->toString(nullptr));
        bbassertexplain(source.existsAndTypeEquals(CODE), "Unexpected value: "+source.torepr(), "Can only call `default` on a code block.", "");
        auto code = static_cast<Code*>(source.get());
        BMemory newMemory(depth, &memory, LOCAL_EXPECTATION_FROM_CODE(code));
        ExecutionInstance executor(depth, code, &newMemory, forceStayInThread);
        auto returnedValue = executor.run(code);
        if(returnedValue.returnSignal)  bberrorexplain("Unexpected command.", "Cannot return from within a `default` statement", "");
        memory.replaceMissing(&newMemory);
        continue;
    }
    DO_NEW: {
        DataPtr source = memory.get(command.args[1]);
        if(source.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(source.get())->consume()->toString(nullptr));
        bbassertexplain(source.existsAndTypeEquals(CODE), "Unexpected value: "+source.torepr(), "Can only create a new struct from a code block.", "");
        auto code = static_cast<Code*>(source.get());
        BMemory newMemory(depth, &memory, LOCAL_EXPECTATION_FROM_CODE(code));
        auto thisObj = new Struct(); 
        newMemory.set(variableManager.thisId, thisObj);
        //newMemory->setFinal(variableManager.thisId);
        ExecutionInstance executor(depth, code, &newMemory, forceStayInThread);
        try {
            auto returnedValue = executor.run(code);
            if(thisObj==returnedValue.get().get()) newMemory.directTransfer(thisObj);
            DISPATCH_OUTCOME(returnedValue.get());
        }
        catch (const BBError& e) {
            // here we interrupt exceptions thrown during new statements, which would leak the memory being created normally
            handleExecutionError(program[i], e);
        }
        continue;
    }
    DO_TOLIST: {
        if(command.args.size()<=1) DISPATCH_RESULT(new BList(0));
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.existsAndTypeEquals(LIST)) DISPATCH_RESULT(arg0);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        // TODO: maybe it's not a good idea to allow this kind of typecasting
        auto list = new BList(1);
        arg0.existsAddOwner();
        list->contents.push_back(arg0);
        DISPATCH_RESULT(list);

        //bberrorexplain("Unexpected number of arguments.", "`list` can only be cast from another list or be created from no arguments. Use `list::gather` to create a list from an iterator, `list::element` to create a list of single element, or explicit parentheses if you want to create a list as a function argument like `foo((1,2))`.", "");
    }
    DO_GATHER: {
        int id1 = command.args[1];
        arg0 = memory.get(id1);
        if(arg0.existsAndTypeEquals(LIST)) bberrorexplain("Unexpected argument.", "`list::gather(A)` for list A is not allowed, as this would pop all elements A by calling `next` internally; this would be a hidden state modification. Use `list::gather(A|iter)` to copy the list or `A|move` to consume and transfer its contents.", "");
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(!arg0.exists()) bberrorexplain("Unexpected number of arguments.", "`list::gather(A)` is not available for literals A. Maybe you meant to use `list::element(A)` to create a list of one element?", "");
        
        Data* it = static_cast<Data*>(arg0.get());
        BList* ret = new BList();
        try {
            Result next = it->next(&memory);
            arg1 = next.get();
            while(arg1.get()!=OUT_OF_RANGE) {
                if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
                next.get().existsAddOwner();
                ret->contents.push_back(arg1);
                next = it->next(&memory);
            }
        }
        catch (const BBError& e) {
            delete ret;
            throw e;
        }
        DISPATCH_RESULT(ret);
    }
    DO_TOMAP: {
        int n = command.nargs;
        if(n==1) DISPATCH_RESULT(new BHashMap());
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(MAP)) DISPATCH_RESULT(arg0);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.existsAndTypeEquals(LIST), "Unexpected value: "+arg0.torepr(), "Maps can only be created from no arguments or from lists of (key, value) pairs.", "");
        DISPATCH_RESULT(static_cast<BList*>(arg0.get())->toMap());
    }
    DO_TIME: {
        result = DataPtr(wallclock_start+static_cast<double>(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()-program_start).count()));
        DISPATCH_COMPUTED_RESULT;
    }
    DO_RANDOM: {
        bbassertexplain(command.nargs>=0, "Expecting argument.", "Random requires at least one argument to serve as seed, such as the output of `time()`. There is no default to make sure that replicable tests can be created.", "");
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.isfloat() || arg0.isint(), "Unexpected value: "+arg0.torepr(), "Random requires an int or float argument that serves as seed, such as the output of `time()`.", "");
        DISPATCH_RESULT(new RandomGenerator(arg0.isint()?(unsigned int)arg0.unsafe_toint():(unsigned int)arg0.unsafe_tofloat()));  // cast instead of byte cast to maintain expected behavior
    }
    DO_PUSH: {
        arg0 = memory.get(command.args[1]);
        if(!arg0.exists()) bbcascade1("Cannot push to this data type: "+arg0.torepr());
        arg1 = memory.get(command.args[2]);
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        DISPATCH_OUTCOME(arg0->push(&memory, arg1));
    }
    DO_PUT: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(!arg0.exists()) bbcascade1("Cannot put to this data type: "+arg0.torepr());
        arg1 = memory.get(command.args[2]);
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        const auto& arg2 = memory.get(command.args[3]);
        if(arg2.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg2.get())->consume()->toString(nullptr));
        DISPATCH_OUTCOME(arg0->put(&memory, arg1, arg2));
    }
    DO_TOGRAPHICS: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.existsAndTypeEquals(STRING), "Unexpected value: "+arg0.torepr(), "Can only create graphics from string window names as the first argument.", "");
        arg1 = memory.get(command.args[2]);
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        bbassertexplain(arg1.isint(), "Unexpected value: "+arg1.torepr(), "Second graphics argument is the window width and should be an int.", "");
        const auto& arg2 = memory.get(command.args[3]);
        if(arg2.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg2.get())->consume()->toString(nullptr));
        bbassertexplain(arg2.isint(), "Unexpected value: "+arg2.torepr(), "Second graphics argument is the window height and should be an int.", "");
        DISPATCH_RESULT(new Graphics(static_cast<BString*>(arg0.get())->toString(nullptr), arg1.unsafe_toint(), arg2.unsafe_toint()));
    }
    DO_AT: {
        arg0 = memory.get(command.args[1]);
        arg1 = memory.get(command.args[2]);
        if(arg0.exists()) DISPATCH_OUTCOME(arg0->at(&memory, arg1));
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(arg1.existsAndTypeEquals(STRING)) {
            if(arg0.isfloat()) DISPATCH_RESULT(new BString(__python_like_float_format(arg0.unsafe_tofloat(), arg1->toString(&memory))));
            if(arg0.isint()) DISPATCH_RESULT(new BString(__python_like_int_format(arg0.unsafe_toint(), arg1->toString(&memory))));
        }
        bberror("Did not find builtin operation: at("+arg0.torepr()+", "+arg1.torepr()+")");
    }
    DO_SUM: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: sum("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->sum(&memory));
    }
    DO_NOT: {
        arg0 = memory.get(command.args[1]);
        if(arg0.isbool()) DISPATCH_RESULT(!arg0.unsafe_tobool());
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: not("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->opnot(&memory));
    }
    DO_LEN: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: len("+arg0.torepr()+")");
        DISPATCH_RESULT(arg0->len(&memory));
    }
    DO_MAX: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: max("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->max(&memory));
    }
    DO_MIN: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: min("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->min(&memory));
    }
    DO_POP: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: len("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->pop(&memory));
    }
    DO_NEXT: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.isptr(), "Did not find builtin operation: next("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->next(&memory));
    }
    DO_MOVE: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(arg0.islit()) DISPATCH_RESULT(arg0);
        bbassert(arg0.exists(), "Did not find builtin operation: clear("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->move(&memory));
    }
    DO_CLEAR: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.exists(), "Did not find builtin operation: clear("+arg0.torepr()+")");
        arg0->clear(&memory);
        continue;
    }
    DO_SHAPE: 
    DO_TOFILE: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(FILETYPE)) DISPATCH_RESULT(arg0);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.existsAndTypeEquals(STRING), "Unexpected value: "+arg0.torepr(), "Can only create files from string paths.", "");
        DISPATCH_RESULT(new BFile(static_cast<BString*>(arg0.get())->toString(nullptr)));
    }
    DO_TOSQLITE: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(SQLLITE)) DISPATCH_RESULT(arg0);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassertexplain(arg0.existsAndTypeEquals(STRING), "Unexpected value: "+arg0.torepr(), "Can only create databases from string paths.", "");
        DISPATCH_RESULT(new Database(static_cast<BString*>(arg0.get())->toString(nullptr)));
    }
    DO_TOITER: {
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        bbassert(arg0.exists(), "Did not find builtin operation: iter("+arg0.torepr()+")");
        DISPATCH_OUTCOME(arg0->iter(&memory));
    }
    DO_TORANGE: {
        bbassertexplain(command.nargs>=0, "Expecting arguments.", "Range requires at least one argument.", "");
        arg0 = memory.get(command.args[1]);
        if(arg0.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg0.get())->consume()->toString(nullptr));
        if(command.nargs<=2 && arg0.isint()) DISPATCH_RESULT(new IntRange(0, arg0.unsafe_toint(), 1));
        arg1 = memory.get(command.args[2]);
        if(arg1.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg1.get())->consume()->toString(nullptr));
        if(command.nargs<=3 && arg0.isintint(arg1)) DISPATCH_RESULT(new IntRange(arg0.unsafe_toint(), arg1.unsafe_toint(), 1));
        const auto& arg2 = memory.get(command.args[3]);
        if(arg2.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(arg2.get())->consume()->toString(nullptr));
        if(command.nargs<=4 && arg0.isintint(arg1) && arg2.isint()) DISPATCH_RESULT(new IntRange(arg0.unsafe_toint(), arg1.unsafe_toint(), arg2.unsafe_toint()));
        if(command.nargs<=4 && arg0.isfloatfloat(arg1) && arg2.isfloat()) DISPATCH_RESULT(new IntRange(arg0.unsafe_tofloat(), arg1.unsafe_tofloat(), arg2.unsafe_tofloat()));
        bberrorexplain("Unexpected arguments.", "Range can take as arguments up to three integers or exactly three floats.", "");
    }
    DO_GET: {
        const DataPtr& objFound = memory.getOrNull(command.args[1], true);
        if(!objFound.exists()) {
            bbassert(command.args[1]==variableManager.thisId, "Missing value: " + variableManager.getSymbol(command.args[1]));
            result = memory.get(command.args[2]);
        }
        else {
            if(objFound.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(objFound.get())->consume()->toString(nullptr));
            bbassertexplain(objFound.existsAndTypeEquals(STRUCT), "Unexpected value: "+objFound->toString(&memory), "Can only get fields from structs, but instead found this value.", "");
            auto obj = static_cast<Struct*>(objFound.get());
            std::lock_guard<std::recursive_mutex> lock(obj->memoryLock);
            result = obj->getOrNull(command.args[2]);
            if(!result.islitorexists()) {
                bbassertexplain(command.args[1]==variableManager.thisId, "Missing field: " + variableManager.getSymbol(command.args[2]), "The struct "+variableManager.getSymbol(command.args[1])+" does exist, but does not contain the field in question.", "");
                result = memory.get(command.args[2]);
            }
            if(result.existsAndTypeEquals(CODE)) memory.codeOwners[static_cast<Code*>(result.get())] = obj;
        }
        DISPATCH_COMPUTED_RESULT;
    }
    DO_BEGINCACHE: {
        size_t pos = i + 1;
        int depth = 0;
        OperationType command_type(END);
        while(pos <= program.size()) {
            command_type = program[pos].operation;
            if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
            if(command_type == END) {
                if (depth == 0) break;
                depth--;
            }
            pos++;
        }
        bbassertexplain(depth >= 0, "Syntax error.", "Cache declaration never ended. These declarations are typically created from the optimizer. This error should not normally appear.", "");
        auto cache = new Code(&program, i + 1, pos, command_type == END?(pos-1):pos);
        BMemory cacheMemory(0, nullptr, 16);
        ExecutionInstance cacheExecutor(depth, cache, &cacheMemory, forceStayInThread);
        auto ret = cacheExecutor.run(cache);
        cacheMemory.await();
        cachedData.pull(&cacheMemory);
        result = nullptr;
        bbassertexplain(!ret.returnSignal, "Unexpected return.", "Cache declaration cannot return a value. These declarations are typically created from the optimizer. This error should not normally appear.", "");
        continue;
    }
    DO_BEGIN: {
        result = command.value;
        int carg = command.args[0];
        memory.set(carg, result);
        i = static_cast<Code*>(command.value.get())->getEnd();
        continue;
    }
    DO_BEGINFINAL: {
        result = command.value;
        int carg = command.args[0];
        memory.set(carg, result);
        memory.setFinal(command.args[0]); // WE NEED TO SET FINALS ONLY AFTER THE VARIABLE IS SET
        i = static_cast<Code*>(command.value.get())->getEnd();
        continue;
    }
    DO_CALL: {
        // find out what to call
        const auto context = command.args[1] == variableManager.noneId ? DataPtr::NULLP : memory.get(command.args[1]);
        if(context.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(context.get())->consume()->toString(nullptr));
        bbassert(!context.exists() || context.existsAndTypeEquals(CODE), "Function arguments must be packed into a code block");
        DataPtr called = memory.get(command.args[2]);
        if(called.existsAndTypeEquals(STRUCT)) {
            auto strct = static_cast<Struct*>(called.get());
            auto val = strct->get(variableManager.callId);
            if(val.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(val.get())->consume()->toString(nullptr));
            bbassert(val.existsAndTypeEquals(CODE), "Struct was called like a method but has no implemented code for `call`.");
            memory.codeOwners[static_cast<Code*>(val.get())] = static_cast<Struct*>(called.get());
            called = (val);
        }
        if(called.existsAndTypeEquals(ERRORTYPE)) throw BBError(static_cast<BError*>(called.get())->consume()->toString(nullptr));
        bbassert(called.existsAndTypeEquals(CODE), "Calling a function with non-codeblock type: "+called.torepr());
        Code* code = static_cast<Code*>(called.get());
        Code* callCode = context.exists()?static_cast<Code*>(context.get()):nullptr;
    
        // run prample
        BMemory newMemory(depth, &memory, LOCAL_EXPECTATION_FROM_CODE(code)+(callCode?LOCAL_EXPECTATION_FROM_CODE(callCode):0));
        if(callCode) {
            ExecutionInstance executor(depth, callCode, &newMemory, forceStayInThread);
            auto returnedValue = executor.run(callCode);
            if(returnedValue.returnSignal) DISPATCH_RESULT(returnedValue.get());
        }
        
        // switch the memory to the new context
        const auto& it = memory.codeOwners.find(code);
        const auto& thisObj = it != memory.codeOwners.end() ? DataPtr(it->second) : memory.getOrNull(variableManager.thisId, true);
        bool thisObjExists = thisObj.exists();
        if(thisObjExists) newMemory.set(variableManager.thisId, thisObj);
        newMemory.parent = memory.getParentWithFinals();  
        newMemory.allowMutables = false;
        
        // run the called block
        ExecutionInstance executor(depth, code, &newMemory, thisObjExists);
        auto returnedValue = executor.run(code);
        result = returnedValue.get();
        newMemory.await();
        DataPtr args = newMemory.getOrNullShallow(variableManager.argsId);
        if(args.existsAndTypeEquals(LIST) && static_cast<BList*>(args.get())->len(&newMemory)) bberrorexplain("Too many arguments.", "The function was successfully called but there are "+std::to_string(static_cast<BList*>(args.get())->len(&newMemory))+" leftover args.", "");
        DISPATCH_COMPUTED_RESULT;

    }

    }//end try 
    catch (const BBError& e) {
        std::string err = enrichErrorDescription(program[i], e.what());
        int carg = command.args.size()?command.args[0]:variableManager.noneId; 
        if(command.operation==IS) {
            BError* berror = new BError(std::move(err));
            //memory.consumeAllErrors();
            result = DataPtr(berror); 
            return ExecutionInstanceRunReturn(true, Result(result));
        }


        if(command.operation!=RETURN)
        if(carg==variableManager.noneId) {
            //err += "\n \033[33m !!! \033[0mAt this point, the error is returned because it is not assigned to"
            //       "\n      a variable and would have been ignored otherwise.\033[0m";
            BError* berror = new BError(std::move(err));
            //memory.consumeAllErrors();
            result = DataPtr(berror);
            return ExecutionInstanceRunReturn(true, Result(result));
        }
        /*if(command.operation==IS) {
            err += "\n \033[33m !!! \033[0mAt this point, the error is returned instead of being copied.\033[0m";
            BError* berror = new BError(std::move(err));
            result = DataPtr(berror);
            //memory.set(carg, berror); 
            //memory.consumeAllErrors();
            return ExecutionInstanceRunReturn(true, Result(result));
        }*/
        if(command.operation!=RETURN)
        if(command.operation==SET 
            || command.operation==SETFINAL 
            || command.operation==PUT 
            || command.operation==POP  
            || command.operation==PUSH 
            || command.operation==NEXT
            || command.operation==FAIL
            || command.operation==MOVE
            || command.operation==CLEAR
            || command.operation==FINAL
            || command.operation==BB_PRINT
            || command.operation==READ) {
            BError* berror = new BError(std::move(err));
            result = DataPtr(berror);
            memory.consumeAllErrors();
            return ExecutionInstanceRunReturn(true, Result(result));
        }
        BError* berror = new BError(std::move(err));
        try {
            result = DataPtr(berror); 
            if(command.operation==RETURN) {
                memory.consumeAllErrors();
                return ExecutionInstanceRunReturn(true, Result(result));
            }
            else memory.set(carg, berror); 
        }
        catch (const BBError& e) {
            result = DataPtr::NULLP;
            memory.set(carg, result); 
            delete berror;
            throw BBError(err);
        }
    }
    }//end loop
    return ExecutionInstanceRunReturn(false, Result(result));
}
