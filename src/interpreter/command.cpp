#include "interpreter/Command.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "data/Boolean.h"
#include <iostream>
#include <cstdlib>
#include "BMemory.h" // for variable manager


Command::Command(const std::string& command, SourceFile* source, int line, CommandContext* descriptor): 
    source(source), line(line), descriptor(descriptor), lastCalled(nullptr) {
    value = nullptr;
    std::vector<std::string> argNames;
    argNames.reserve(4);
    std::string accumulate;
    int pos = 0;
    bool inString = false;
    while (pos < command.size()) {
        if (command[pos] == '"')
            inString = !inString;
        if (!inString && (command[pos] == ' ' || pos == command.size() - 1)) {
            if (command[pos] != ' ')
                accumulate += command[pos];
            argNames.push_back(accumulate);
            accumulate = "";
        } else {
            accumulate += command[pos];
        }
        pos += 1;
    }
    operation = getOperationType(argNames[0]);
    nargs = argNames.size() - 1;

    if (operation == BUILTIN) {
        nargs -= 1;
        std::string raw = argNames[2];
        if (raw[0] == '"')
            value = new BString(raw.substr(1, raw.size() - 2));
        else if (raw[0] == 'I')
            value = new Integer(std::atoi(raw.substr(1).c_str()));
        else if (raw[0] == 'F')
            value = new BFloat(std::atof(raw.substr(1).c_str()));
        else if (raw[0] == 'B')
            value = new Boolean(raw == "Btrue");
        else {
            bberror("Unable to understand builtin value prefix (should be one of I,F,B,\"): " + raw);
        }
        value->isDestroyable = false;
    }

    args = new int[nargs];
    knownLocal = new bool[nargs];
    for (int i = 0; i < nargs; ++i) {
        knownLocal[i] = argNames[i+1].substr(0, 3) == "_bb";
        args[i] = variableManager.getId(argNames[i+1]);
    }
}

Command::~Command() {
    delete[] args;
    delete value;
    delete[] knownLocal;
}

std::string Command::toString() const {
    if(descriptor)
        return descriptor->source;
    std::string ret = getOperationTypeName(operation);
    for(int i=0;i<nargs;++i) 
        ret += " "+variableManager.getSymbol(args[i]);
    return ret;
}

SourceFile::SourceFile(const std::string& path): path(path) {}
CommandContext::CommandContext(const std::string& source): source(source) {}