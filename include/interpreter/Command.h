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
    std::vector<int> args;                  // Store arguments as a vector
    std::vector<bool> knownLocal;           // Store knownLocal as a vector
    std::shared_ptr<Data> value;            // Use shared_ptr for value
    std::shared_ptr<SourceFile> source;     // Use shared_ptr for source
    int line;
    int nargs;                              // Keeep this separately for fast checks
    std::shared_ptr<CommandContext> descriptor;  // Use shared_ptr for descriptor
    BMemory* lastCalled;

    Command(const std::string& command, std::shared_ptr<SourceFile> source, int line, std::shared_ptr<CommandContext> descriptor);
    std::string toString() const;
};

#endif // COMMAND_H
