#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include "common.h"
#include "data/Data.h"

class SourceFile {
public:
    std::string path;
    SourceFile(const std::string& path);
};

class CommandContext {
public:
    std::string source;
    CommandContext(const std::string& source);
};

class Command {
public:
    OperationType operation;
    int* args;
    bool* knownLocal;
    int nargs;
    Data* value;
    SourceFile* source;
    int line;
    CommandContext* descriptor;
    BMemory* lastCalled;

    Command(const std::string& command, SourceFile* source, int line, CommandContext* descriptor);
    ~Command();
    std::string toString() const;
};

#endif // COMMAND_H
