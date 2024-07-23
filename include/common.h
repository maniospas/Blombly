#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstring>
#include <stdexcept>

// Exception classes
class Unimplemented : public std::exception {
public:
    const char* what() const noexcept override {
        return "Unimplemented method.";
    }
};

class BBError : public std::runtime_error {
public:
    explicit BBError(const std::string& message) : std::runtime_error(message) {}
};

// custom error messages
#define bberror(msg) throw BBError(std::string(" \033[0m(\x1B[31m ERROR \033[0m) ")+(msg)) //std::cout<<" \033[0m(\x1B[31m ERROR \033[0m) "<<(msg)<<"\n";exit(1); 
#define bbassert(expr, msg) if(!(expr)) {bberror(msg);}
#define bbverify(precondition, expr, msg) if(precondition && !(expr)) {std::cerr<<msg<<"\n";exit(1);}

// Enumeration of data types
enum Datatype {FUTURE, BOOL, INT, FLOAT, VECTOR, LIST, STRING, CODE, STRUCT, ITERATOR, FILETYPE, ERRORTYPE};

// Array to map datatype enums to string representations
static const char* datatypeName[] = { 
    "future", "bool", "int", "float", "vector", "list", "string", "code", "struct", "iterator", "file", "error"
};

// Global strings for different operations
enum OperationType {NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG, 
                    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOINT, TOFLOAT, TOSTR, TOBOOL, TOCOPY, TOFILE,
                    SUM, MAX, MIN,
                    BUILTIN, BEGIN, BEGINFINAL, BEGINCACHED, END, RETURN, FINAL, IS, 
                    CALL, WHILE, IF, NEW, PRINT, INLINE, GET, SET, SETFINAL, DEFAULT,
                    TIME, TOITER, TRY, CATCH, FAIL};
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul", 
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape", 
    "vector", "list", "int", "float", "str", "bool", "copy", "file",
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "BEGINCACHED", "END", "return", "final", "IS", 
    "call", "while", "if", "new", "print", "inline", "get", "set", "setfinal", "default",
    "time", "iter", "try", "catch", "fail"
};

// map operations to symbols and conversely
void initializeOperationMapping();
OperationType getOperationType(const std::string &str);
std::string getOperationTypeName(OperationType type);


// block execution declarations
#define DEFAULT_LOCAL_EXPECTATION 8
#define LOCAL_EXPACTATION_FROM_CODE(code) std::min((code->getEnd()-code->getStart())*2, DEFAULT_LOCAL_EXPECTATION)

class Data;
class Command;
class BMemory;
class BuiltinArgs;
class VariableManager;
extern VariableManager variableManager;
Data* executeBlock(std::vector<Command*>* program,
                  int start, 
                  int end,
                  const std::shared_ptr<BMemory>& memory, 
                  bool *returnSignal,
                  BuiltinArgs* allocatedBuiltins
                  );


// code reused when returning various data from overriden Data::implement to not reallicate memory

#define STRING_RESULT(expr) if(args->preallocResult && args->preallocResult->getType()==STRING) { \
                    ((BString*)args->preallocResult)->value = expr; \
                    return args->preallocResult; \
                } \
                return new BString(expr)

#define BOOLEAN_RESULT(expr) if(args->preallocResult && args->preallocResult->getType()==BOOL) { \
                    ((Boolean*)args->preallocResult)->value = expr; \
                    return args->preallocResult; \
                } \
                return new Boolean(expr)
                

#define INT_RESULT(expr) if(args->preallocResult && args->preallocResult->getType()==INT) { \
                    ((Integer*)args->preallocResult)->value = expr; \
                    return args->preallocResult; \
                } \
                return new Integer(expr)


#define FLOAT_RESULT(expr) if(args->preallocResult && args->preallocResult->getType()==FLOAT) { \
                    ((BFloat*)args->preallocResult)->value = expr; \
                    return args->preallocResult; \
                } \
                return new BFloat(expr)



#endif // COMMON_H
