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

#ifndef BCOMMON_H
#define BCOMMON_H

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <bit>
#include <cstdint>
#include <string>
#include <sstream>

class BBError : public std::runtime_error {public: explicit BBError(const std::string& message) : std::runtime_error(message) {}};
#define bberror(msg) [[unlikely]] throw BBError("\033[0m(\x1B[31m ERROR \033[0m) " + std::string(msg))
#define bbcascade(msg1, msg) [[unlikely]] throw BBError(std::string(msg1) + "\n\033[0m(\x1B[31m ERROR \033[0m) " + std::string(msg))
#define bbcascade1(msg) bberror(msg)
#define bbassert(expr, msg) if (!(expr)) bberror(msg);


#define bbassertexplain(expr, msg, explain, postfix) {if (!(expr)) bberrorexplain(msg, explain, postfix);}
void bberrorexplain(const std::string& msg, const std::string& explanation, const std::string& postfix);

enum Datatype {FUTURE, VECTOR, LIST, STRING, CODE, STRUCT, ITERATOR, FILETYPE, ERRORTYPE, MAP, SERVER, SQLLITE, GRAPHICS};
static const char* datatypeName[] = {"future", "vector", "list", "string", "code", "struct", "iterator", "file", "error", "map", "server", "sqlite", "graphics"};

enum OperationType {
    NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG,
    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOMAP, TOBB_INT, TOBB_FLOAT, TOSTR, TOBB_BOOL, TOFILE,
    SUM, MAX, MIN,
    BUILTIN, BEGIN, BEGINFINAL, BEGINCACHE, END, RETURN, FINAL, IS,
    CALL, WHILE, IF, NEW, BB_PRINT, INLINE, GET, SET, SETFINAL, DEFAULT,
    TIME, TOITER, TRY, CATCH, FAIL, EXISTS, READ, CREATESERVER, AS, TORANGE, 
    DEFER, CLEAR, MOVE, ISCACHED, TOSQLITE, TOGRAPHICS, RANDOM,
    RANDVECTOR, ZEROVECTOR, ALLOCVECTOR, LISTELEMENT, LISTGATHER
};
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul",
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape",
    "vector", "list", "map", "int", "float", "str", "bool", "file",
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "CACHE", "END", "return", "final", "IS",
    "call", "while", "if", "new", "print", "inline", "get", "set", "setfinal", "default",
    "time", "iter", "do", "catch", "fail", "exists", "read", "server", "AS", "range",
    "defer", "clear", "move", "ISCACHED", "sqlite", "graphics", "random",
    "vector::consume", "vector::zero", "vector::alloc", "list::element", "list::gather"
};

void initializeOperationMapping();
OperationType getOperationType(const std::string& str);
std::string getOperationTypeName(OperationType type);

#define DEFAULT_LOCAL_EXPECTATION (size_t)32
#define LOCAL_EXPECTATION_FROM_CODE(codeContext) std::min(1+(codeContext->getEnd() - codeContext->getStart()), DEFAULT_LOCAL_EXPECTATION)

class Data;
class Command;
class BMemory;
struct BuiltinArgs;
class VariableManager;
class Code;
class Result;
extern VariableManager variableManager;

// Code reused when returning various data from overridden Data::implement 
#define STRING_RESULT(expr) return RESMOVE(Result(new BString(expr)))
#define BB_BOOLEAN_RESULT(expr) return RESMOVE(Result((bool)(expr)))
#define BB_INT_RESULT(expr) return RESMOVE(Result((int64_t)(expr)))
#define BB_FLOAT_RESULT(expr) return RESMOVE(Result((double)(expr)))

#define RESMOVE(value) (value)








// data pointer class
//#define DataPtr Data*

#define DATTYPETYPE char

constexpr DATTYPETYPE IS_FLOAT = static_cast<DATTYPETYPE>(1);
constexpr DATTYPETYPE IS_INT = static_cast<DATTYPETYPE>(2);
constexpr DATTYPETYPE IS_BOOL = static_cast<DATTYPETYPE>(4);
constexpr DATTYPETYPE IS_PTR = static_cast<DATTYPETYPE>(8);
constexpr DATTYPETYPE IS_FUTURE = static_cast<DATTYPETYPE>(16);
constexpr DATTYPETYPE IS_ERROR = static_cast<DATTYPETYPE>(32);
constexpr DATTYPETYPE IS_PROPERTY_A = static_cast<DATTYPETYPE>(64);
constexpr DATTYPETYPE IS_PROPERTY_B = static_cast<DATTYPETYPE>(128);
constexpr DATTYPETYPE IS_FLOAT_OR_INT = static_cast<DATTYPETYPE>(IS_FLOAT | IS_INT);

// Compute negations and combinations at compile-time
constexpr DATTYPETYPE IS_NOT_FLOAT = static_cast<DATTYPETYPE>(~IS_FLOAT & ~IS_PROPERTY_A & ~IS_PROPERTY_B);
constexpr DATTYPETYPE IS_NOT_INT = static_cast<DATTYPETYPE>(~IS_INT & ~IS_PROPERTY_A & ~IS_PROPERTY_B);
constexpr DATTYPETYPE IS_NOT_BOOL = static_cast<DATTYPETYPE>(~IS_BOOL & ~IS_PROPERTY_A & ~IS_PROPERTY_B);
constexpr DATTYPETYPE IS_NOT_PTR = static_cast<DATTYPETYPE>(~IS_PTR & ~IS_PROPERTY_A & ~IS_PROPERTY_B);
constexpr DATTYPETYPE IS_NOT_FUTURE = static_cast<DATTYPETYPE>(~IS_FUTURE & ~IS_PROPERTY_A & ~IS_PROPERTY_B);
constexpr DATTYPETYPE IS_LIT = static_cast<DATTYPETYPE>(IS_FLOAT | IS_INT | IS_BOOL);
constexpr DATTYPETYPE IS_NOT_PROPERTY_A = static_cast<DATTYPETYPE>(~IS_PROPERTY_A);
constexpr DATTYPETYPE IS_NOT_PROPERTY_B = static_cast<DATTYPETYPE>(~IS_PROPERTY_B);
constexpr DATTYPETYPE REMOVE_PROPERTIES = static_cast<DATTYPETYPE>(~(IS_PROPERTY_A | IS_PROPERTY_B));
constexpr DATTYPETYPE IS_NOT_ERROR = static_cast<DATTYPETYPE>(~IS_ERROR);


//#define SAFETYCHECKS


/**
 * The DataPtr class has both safety and optimization features. The safety features can be
 * disabled by commenting out the lines with bberror; sefety refers to the language's executable.
 */
class Data;
class Future;
class BError;

struct DataPtr {
private:
    int64_t data;
    DATTYPETYPE datatype; // second to optimize for alignment
public:
    inline bool isset() const {return datatype;}
    DataPtr(double data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_FLOAT) {}
    DataPtr(Data* data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_PTR) {}
    //DataPtr(BError* data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_PTR|IS_ERROR) {}
    DataPtr(int64_t data) noexcept : data(data), datatype(IS_INT) {}
    DataPtr(int data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_INT) {}
    DataPtr(bool data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_BOOL) {}
    DataPtr(DataPtr&& other) noexcept : data(other.data), datatype(other.datatype) {
        //other.data = 0;
        //other.datatype = IS_PTR;
    }
    DataPtr(void* data, DATTYPETYPE datatype) noexcept : data(std::bit_cast<int64_t>(data)), datatype(datatype) {}
    DataPtr(const DataPtr& other) : data(other.data), datatype(other.datatype) {}
    inline DataPtr& operator=(Data* other) {
        data = std::bit_cast<int64_t>(other);
        datatype = IS_PTR;
        return *this;
    }
    inline DataPtr& operator=(const DataPtr& other) {
        if (this != &other) {
            data = other.data;
            datatype = other.datatype;
        }
        return *this;
    }
    inline DataPtr& operator=(DataPtr&& other) noexcept {
        if (this != &other) {
            data = other.data;
            datatype = other.datatype;
            other.data = 0;
            other.datatype = IS_PTR;
        }
        return *this;
    }
    DataPtr(): data(0), datatype(IS_PTR) {}

    inline Data* operator->() const {
        //if (datatype & IS_NOT_PTR) bberror("Internal error: trying to -> on a builtin that is not stored as a data type");
        return std::bit_cast<Data*>(data);
    }

    inline double tofloat() const {
        #ifdef SAFETYCHECKS
        if (datatype & IS_PTR) bberror("Internal error: cannot run `tobool` for a data structure implicitly.");
        #endif
        if (datatype & IS_NOT_FLOAT) return data;
        return std::bit_cast<double>(data);
    }

    inline int64_t toint() const {
        #ifdef SAFETYCHECKS
        if (datatype & IS_PTR) bberror("Internal error: cannot run `toint` for a data structure implicitly.");
        if (datatype & IS_FLOAT) return std::bit_cast<double>(data);
        #endif
        return data;
    }

    inline bool tobool() const {
        #ifdef SAFETYCHECKS
        if (datatype & IS_PTR) bberror("Internal error: cannot run `tobool` for a data structure implicitly.");
        #endif
        return data;
    }

    inline Future* tofuture() const {
        #ifdef SAFETYCHECKS
        if (datatype & IS_NOT_FUTURE) bberror("Internal error: cannot run `tofuture` for a data structure implicitly.");
        #endif
        return std::bit_cast<Future*>(data);
    }

    inline Future* unsafe_tofuture() const {
        return std::bit_cast<Future*>(data);
    }

    inline Data* get() const {
        #ifdef SAFETYCHECKS
        if (datatype & IS_NOT_PTR) bberror("Internal error: trying to `get` on a builtin that is not stored as a data type");
        if (!data)  bberror("Internal error: Trying to `get` a null pointer. This should not happen as everything should be guarded by testing whether the pointer `exists` first."); 
        #endif
        return std::bit_cast<Data*>(data);
    }

    inline bool exists() const {
        if(datatype & IS_NOT_PTR) return false; //exists is always a check for pointers
        return data;
    }
    
    inline bool notexists() const {
        if(datatype & IS_PTR) return !data; //exists is always a check for pointers
        return false;
    }



    inline bool existsAndTypeEquals(Datatype type) const;
    inline void existsAddOwner() const;
    inline void existsRemoveFromOwner();
    inline bool isSame(const DataPtr& other) const;
    inline std::string torepr() const;

    inline void clear() {
        datatype = IS_PTR;
        data = 0;
    }

    inline bool islitorexists() const {
        if(datatype & IS_PTR) return data; 
        return true;
    }

    inline bool notislitorexists() const {
        if(datatype & IS_PTR) return !data; 
        return false;
    }
    
    inline bool islit() const {return datatype & IS_LIT;}
    inline bool operator==(const DataPtr& other) const {
        if (datatype & IS_NOT_PTR) return false;
        return data==other.data;
    }

    inline double unsafe_tofloat() const { return std::bit_cast<double>(data); }
    inline int64_t unsafe_toint() const { return data; }
    inline bool unsafe_tobool() const { return data; }


    inline bool isintint(const DataPtr& other) {return datatype & IS_INT & other.datatype;}
    inline bool isfloatfloat(const DataPtr& other) {return datatype & IS_FLOAT & other.datatype;}
    inline bool iserror() const { return datatype & IS_ERROR; }
    inline bool isfloat() const { return datatype & IS_FLOAT; }
    inline bool isfloatorint() const { return datatype & IS_FLOAT_OR_INT; }
    inline bool isint() const { return datatype & IS_INT; }
    inline bool isbool() const { return datatype & IS_BOOL; }
    inline bool isfuture() const { return datatype & IS_FUTURE; }
    inline bool isptr() const { return datatype & IS_PTR; }
    inline bool isA() const { return datatype & IS_PROPERTY_A; }
    inline bool isB() const { return datatype & IS_PROPERTY_B; }
    inline void setA(bool value){if(value) datatype |= IS_PROPERTY_A; else datatype &= ~IS_PROPERTY_A;}
    inline void setAFalse(){datatype &= IS_NOT_PROPERTY_A;}
    inline void setB(bool value){if(value) datatype |= IS_PROPERTY_B; else datatype &= ~IS_PROPERTY_B;}
    inline void setBFalse(){datatype &= IS_NOT_PROPERTY_B;}
    static DataPtr NULLP;
};



#endif // BCOMMON_H
