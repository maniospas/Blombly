#ifndef COMMON_H
#define COMMON_H

#include <string>

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

void initializeOperationMapping();
OperationType getOperationType(const std::string &str);
std::string getOperationTypeName(OperationType type);

// 

// Exception class
class Unimplemented : public std::exception {
public:
    const char* what() const noexcept override {
        return "Unimplemented method.";
    }
};

#endif // COMMON_H
