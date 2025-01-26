#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <memory>
#include "common.h"
#include "data/Data.h"

class SourceFile {
public:
    std::string path;
    explicit SourceFile(const std::string& path);
};

class CommandContext {
public:
    std::string source;
    explicit CommandContext(const std::string& source);
};

class Command {
public:
    OperationType operation;
    std::vector<int> args;
    int nargs;
    mutable DataPtr value;
    SourceFile* source;
    int line;
    CommandContext* descriptor;

    Command(const std::string& command, SourceFile* source, int line, CommandContext* descriptor);
    std::string toString() const;
    std::string tocpp(bool first_assignment) const;
};

#endif // COMMAND_H
