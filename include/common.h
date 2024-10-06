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
    CALL, WHILE, IF, NEW, PRBB_INT, INLINE, GET, SET, SETFINAL, DEFAULT,
    TIME, TOITER, TRY, CATCH, FAIL, EXISTS, READ, CREATESERVER
};

// Array mapping OperationType to string representations
static const std::string OperationTypeNames[] = {
    "not", "and", "or", "eq", "neq", "le", "ge", "lt", "gt", "add", "sub", "mul", "mmul",
    "div", "mod", "len", "pow", "log", "push", "pop", "next", "put", "at", "shape",
    "vector", "list", "map", "int", "float", "str", "bool", "copy", "file",
    "sum", "max", "min",
    "BUILTIN", "BEGIN", "BEGINFINAL", "BEGINCACHED", "END", "return", "final", "IS",
    "call", "while", "if", "new", "print", "inline", "get", "set", "setfinal", "default",
    "time", "iter", "try", "catch", "fail", "exists", "read", "server"
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

std::shared_ptr<Data> executeBlock(const std::shared_ptr<Code>& code,
                   const std::shared_ptr<BMemory>& memory,
                   bool  &returnSignal,
                   const BuiltinArgs&  builtinArgs);

#define MEMGET(memory, arg) (command->knownLocal[arg]?memory->getOrNullShallow(command->args[arg]):memory->get(command->args[arg]))
//#define SCOPY(data) if(data && data->isDestroyable) data = data->shallowCopy()
//#define INLINE_SCOPY(data) ((data && data->isDestroyable)?data->shallowCopy():nullptr)
#define SCOPY(data) if(false) data = data->shallowCopy()
#define INLINE_SCOPY(data) data

// Code reused when returning various data from overridden Data::implement to avoid reallocating memory
#define STRING_RESULT(expr) if (args->preallocResult && args->preallocResult->getType() == STRING && args->preallocResult->isDestroyable) { \
                                auto bstr = std::static_pointer_cast<BString>(args->preallocResult); \
                                bstr->value = (expr); \
                                return args->preallocResult; \
                            } \
                            return std::make_shared<BString>(expr)

#define BB_BOOLEAN_RESULT(expr) if (args->preallocResult && args->preallocResult->getType() == BB_BOOL && args->preallocResult->isDestroyable) { \
                                auto bbool = std::static_pointer_cast<Boolean>(args->preallocResult); \
                                bbool->value = (expr); \
                                return args->preallocResult; \
                            } \
                            return std::make_shared<Boolean>(expr)

#define BB_INT_RESULT(expr) if (args->preallocResult && args->preallocResult->getType() == BB_INT && args->preallocResult->isDestroyable) { \
                            auto bint = std::static_pointer_cast<Integer>(args->preallocResult); \
                            bint->value = (expr); \
                            return args->preallocResult; \
                         } \
                         return std::make_shared<Integer>(expr)

#define BB_FLOAT_RESULT(expr) if (args->preallocResult && args->preallocResult->getType() == BB_FLOAT && args->preallocResult->isDestroyable) { \
                              auto bfloat = std::static_pointer_cast<BFloat>(args->preallocResult); \
                              bfloat->value = (expr); \
                              return args->preallocResult; \
                          } \
                          return std::make_shared<BFloat>(expr)

#endif // COMMON_H
