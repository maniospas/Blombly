#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// custom error messages
#define bbassert(expr, msg) if(!(expr)) {std::cerr<<msg<<"\n";exit(1);}
#define bbverify(precondition, expr, msg) if(precondition && !(expr)) {std::cerr<<msg<<"\n";exit(1);}

// Enumeration of data types
enum Datatype { FUTURE, BOOL, INT, FLOAT, VECTOR, LIST, STRING, CODE, STRUCT, ITERATOR};

// Array to map datatype enums to string representations
static const char* datatypeName[] = { 
    "future", "bool", "int", "float", "vector", "list", "string", "code", "struct", "iterator"
};

// Global strings for different operations
enum OperationType {NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG, 
                    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOINT, TOFLOAT, TOSTR, TOBOOL, TOCOPY,
                    SUM, MAX, MIN,
                    BUILTIN, BEGIN, BEGINFINAL, END, RETURN, FINAL, IS, 
                    CALL, WHILE, IF, NEW, PRINT, INLINE, GET, SET, DEFAULT,
                    TIME, TOITER};
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul", 
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape", 
    "Vector", "List", "int", "float", "str", "bool", "copy", 
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "END", "return", "final", "IS", 
    "call", "while", "if", "new", "print", "inline", "get", "set", "default",
    "time", "iter"
};

// map operations to symbols and conversely
void initializeOperationMapping();
OperationType getOperationType(const std::string &str);
std::string getOperationTypeName(OperationType type);

// Exception class
class Unimplemented : public std::exception {
public:
    const char* what() const noexcept override {
        return "Unimplemented method.";
    }
};


// block execution declarations
#define DEFAULT_LOCAL_EXPECTATION 16
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



#define STRING_RESULT(expr) if(args->preallocResult && args->preallocResult->getType()==STRING) { \
                    ((BString*)args->preallocResult)->value = expr; \
                    return args->preallocResult; \
                } \
                return new BString(expr)

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
