#include "interpreter/Command.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "data/Boolean.h"
#include <iostream>
#include <cstdlib>
#include "BMemory.h" // for variable manager


Command::Command(std::string command) {
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
            std::cerr << "Unable to understand builtin value: " << raw << std::endl;
            exit(1);
        }
        value->isDestroyable = false;
    }

    args = new int[nargs];
    knownLocal = new bool[nargs];
    for (int i = 0; i < nargs; i++) {
        knownLocal[i] = argNames[i+1].substr(0, 3) == "_bb";
        args[i] = variableManager.getId(argNames[i+1]);
    }
}

Command::~Command() {
    delete[] args;
    delete value;
    delete[] knownLocal;
}
