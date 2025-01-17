#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <bit>
#include <cstdint>
#include <string>


#define WHILE_WITH_CODE_BLOCKS  // this changes the while loop parsing and implementation. define for slower but more easily jitable loops

// Exception classes
class Unimplemented : public std::exception {
public:
    const char* what() const noexcept override {return "Unimplemented method.";}
};

class BBError : public std::runtime_error {
public:
    explicit BBError(const std::string& message) : std::runtime_error(message) {}
};

// Custom error message macro
#define bberror(msg) throw BBError("\033[0m(\x1B[31m ERROR \033[0m) " + std::string(msg))
#define bbassert(expr, msg) if (!(expr)) { bberror(msg); }
#define bbverify(precondition, expr, msg) if ((precondition) && !(expr)) { std::cerr << msg << "\n"; exit(1); }

// Enumeration of data types
enum Datatype {
    FUTURE, BB_INT, BB_FLOAT, VECTOR, LIST, STRING, CODE, STRUCT, ITERATOR, FILETYPE, ERRORTYPE, MAP, SERVER, SQLLITE, GRAPHICS
};

// Array to map datatype enums to string representations
static const char* datatypeName[] = {
    "future", "int", "float", "vector", "list", "string", "code", "struct", "iterator", "file", "error", "map", "server", "sqlite", "graphics"
};

// Global strings for different operations
enum OperationType {
    NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG,
    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOMAP, TOBB_INT, TOBB_FLOAT, TOSTR, TOBB_BOOL, TOCOPY, TOFILE,
    SUM, MAX, MIN,
    BUILTIN, BEGIN, BEGINFINAL, BEGINCACHE, END, RETURN, FINAL, IS,
    CALL, WHILE, IF, NEW, BB_PRINT, INLINE, GET, SET, SETFINAL, DEFAULT,
    TIME, TOITER, TRY, CATCH, FAIL, EXISTS, READ, CREATESERVER, AS, TORANGE, 
    DEFER, CLEAR, MOVE, ISCACHED, TOSQLITE, TOGRAPHICS
};

// Array mapping OperationType to string representations
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul",
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape",
    "vector", "list", "map", "int", "float", "str", "bool", "copy", "file",
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "CACHE", "END", "return", "final", "IS",
    "call", "while", "if", "new", "print", "inline", "get", "set", "setfinal", "default",
    "time", "iter", "try", "catch", "fail", "exists", "read", "server", "AS", "range",
    "defer", "clear", "move", "ISCACHED", "sqlite", "graphics"
};

// Map operations to symbols and conversely
void initializeOperationMapping();
OperationType getOperationType(const std::string& str);
std::string getOperationTypeName(OperationType type);

// Block execution declarations
#define DEFAULT_LOCAL_EXPECTATION 8
#define LOCAL_EXPECTATION_FROM_CODE(code) std::min((code->getEnd() - code->getStart()) * 2, DEFAULT_LOCAL_EXPECTATION)

class Data;
class Command;
class BMemory;
struct BuiltinArgs;
class VariableManager;
class Code;
class Result;
extern VariableManager variableManager;

// Code reused when returning various data from overridden Data::implement 
#define STRING_RESULT(expr) return std::move(Result(new BString(expr)))
#define BB_BOOLEAN_RESULT(expr) return std::move(Result(std::move(DataPtr((bool)(expr)))))
#define BB_INT_RESULT(expr) return std::move(Result(new Integer(expr)))
#define BB_FLOAT_RESULT(expr) return std::move(Result(new BFloat(expr)))










// data pointer class
//#define DataPtr Data*



#define DATTYPETYPE char
#define IS_FLOAT static_cast<DATTYPETYPE>(1)
#define IS_INT static_cast<DATTYPETYPE>(2)
#define IS_BOOL static_cast<DATTYPETYPE>(4)
#define IS_PTR static_cast<DATTYPETYPE>(8)
#define IS_PROPERTY_A static_cast<DATTYPETYPE>(16)
#define IS_PROPERTY_B static_cast<DATTYPETYPE>(32)
#define IS_NOT_FLOAT (~IS_FLOAT & ~IS_PROPERTY_A & ~IS_PROPERTY_B)
#define IS_NOT_INT (~IS_INT & ~IS_PROPERTY_A & ~IS_PROPERTY_B)
#define IS_NOT_BOOL (~IS_BOOL & ~IS_PROPERTY_A & ~IS_PROPERTY_B)
#define IS_NOT_PTR (~IS_PTR & ~IS_PROPERTY_A & ~IS_PROPERTY_B)

#define SAFETYCHECKS


/**
 * The DataPtr class has both safety and optimization features. The safety features can be
 * disabled by commenting out the lines with bberror; sefety refers to the language's executable.
 */
class Data;
struct DataPtr {
    DataPtr(double data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_FLOAT) {}
    DataPtr(Data* data) noexcept : data(std::bit_cast<int64_t>(data)), datatype(IS_PTR) {}
    DataPtr(int64_t data) noexcept : data(data), datatype(IS_INT) {}
    DataPtr(int data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_INT) {}
    DataPtr(bool data) noexcept : data(static_cast<int64_t>(data)), datatype(IS_BOOL) {}
    DataPtr(DataPtr&& other) noexcept : data(other.data), datatype(other.datatype) {
        other.data = 0;
        other.datatype = IS_PTR;
    }
    DataPtr(const DataPtr& other) : data(other.data), datatype(other.datatype) {}
    DataPtr& operator=(Data* other) {
        data = std::bit_cast<int64_t>(other);
        datatype = IS_PTR;
        return *this;
    }
    DataPtr& operator=(const DataPtr& other) {
        if (this != &other) {
            data = other.data;
            datatype = other.datatype;
        }
        return *this;
    }
    DataPtr& operator=(DataPtr&& other) noexcept {
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

    inline bool existsAndTypeEquals(Datatype type) const;

    inline bool islitorexists() const {
        if(datatype & IS_PTR) return data; 
        return true;
    }
    inline bool islit() const {
        return datatype & IS_NOT_PTR;
    }

    inline std::string torepr() const {
        if(datatype & IS_PTR) return "Data object at memory address: "+std::to_string(data);
        if(datatype & IS_FLOAT) return std::to_string(unsafe_tofloat());
        if(datatype & IS_INT) return std::to_string(unsafe_toint());
        if(datatype & IS_BOOL) return std::to_string(unsafe_tobool());
        return "Corrupted data";
    }

    inline bool operator==(const DataPtr& other) const {
        if (datatype & IS_NOT_PTR) return false;
        return data==other.data;
    }

    inline double unsafe_tofloat() const { return std::bit_cast<double>(data); }
    inline int64_t unsafe_toint() const { return data; }
    inline bool unsafe_tobool() const { return data; }

    inline bool isfloat() const { return datatype & IS_FLOAT; }
    inline bool isint() const { return datatype & IS_INT; }
    inline bool isbool() const { return datatype & IS_BOOL; }
    inline bool isptr() const { return datatype & IS_PTR; }
    inline bool isA() const { return datatype & IS_PROPERTY_A; }
    inline bool isB() const { return datatype & IS_PROPERTY_B; }
    inline void setA(bool value){if(value) datatype |= IS_PROPERTY_A; else datatype &= ~IS_PROPERTY_A;}
    inline void setB(bool value){if(value) datatype |= IS_PROPERTY_B; else datatype &= ~IS_PROPERTY_B;}
    static DataPtr NULLP;
private:
    DATTYPETYPE datatype;
    int64_t data;
};



#endif // COMMON_H
