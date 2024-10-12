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

// Custom error message macro
#define bberror(msg) throw BBError("\033[0m(\x1B[31m ERROR \033[0m) " + std::string(msg))
#define bbassert(expr, msg) if (!(expr)) { bberror(msg); }
#define bbverify(precondition, expr, msg) if ((precondition) && !(expr)) { std::cerr << msg << "\n"; exit(1); }

// Enumeration of data types
enum Datatype {
    FUTURE, BB_BOOL, BB_INT, BB_FLOAT, VECTOR, LIST, STRING, CODE, STRUCT, ITERATOR, FILETYPE, ERRORTYPE, MAP, SERVER
};

// Array to map datatype enums to string representations
static const char* datatypeName[] = {
    "future", "bool", "int", "float", "vector", "list", "string", "code", "struct", "iterator", "file", "error", "map", "server"
};

// Global strings for different operations
enum OperationType {
    NOT, AND, OR, EQ, NEQ, LE, GE, LT, GT, ADD, SUB, MUL, MMUL, DIV, MOD, LEN, POW, LOG,
    PUSH, POP, NEXT, PUT, AT, SHAPE, TOVECTOR, TOLIST, TOMAP, TOBB_INT, TOBB_FLOAT, TOSTR, TOBB_BOOL, TOCOPY, TOFILE,
    SUM, MAX, MIN,
    BUILTIN, BEGIN, BEGINFINAL, BEGINCACHED, END, RETURN, FINAL, IS,
    CALL, WHILE, IF, NEW, BB_PRINT, INLINE, GET, SET, SETFINAL, DEFAULT,
    TIME, TOITER, TRY, CATCH, FAIL, EXISTS, READ, CREATESERVER, AS
};

// Array mapping OperationType to string representations
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul",
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape",
    "vector", "list", "map", "int", "float", "str", "bool", "copy", "file",
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "BEGINCACHED", "END", "return", "final", "IS",
    "call", "while", "if", "new", "print", "inline", "get", "set", "setfinal", "default",
    "time", "iter", "try", "catch", "fail", "exists", "read", "server", "AS"
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
class BuiltinArgs;
class VariableManager;
class Code;

extern VariableManager variableManager;

Data* executeBlock(Code* code, BMemory* memory, bool  &returnSignal);

#define UNSAFEMEMGET(memory, arg) (command->knownLocal[arg]?memory->getOrNullShallow(command->args[arg]):memory->get(command->args[arg]))
#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getShallow(command->args[arg]):memory->get(command->args[arg]))

// Code reused when returning various data from overridden Data::implement 
#define STRING_RESULT(expr) return new BString(expr)
#define BB_BOOLEAN_RESULT(expr) return new Boolean(expr)
#define BB_INT_RESULT(expr) return new Integer(expr)
#define BB_FLOAT_RESULT(expr) return new BFloat(expr)

#endif // COMMON_H
