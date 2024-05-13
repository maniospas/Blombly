#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include "common.h"
#include "data/Data.h"

class Command {
public:
    OperationType operation;
    int* args;
    bool* knownLocal;
    int nargs;
    Data* value;

    Command(std::string command);
    ~Command();
};

#endif // COMMAND_H
