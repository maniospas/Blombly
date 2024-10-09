#ifndef ERROR_HANDLING_CPP
#define ERROR_HANDLING_CPP

#include "interpreter/functional.h"

void handleExecutionError(std::vector<Command*>* program, int i, const BBError& e) {
    Command* command = program->at(i);
    std::string comm = command->toString();
    comm.resize(40, ' ');
    throw BBError(e.what() + std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line));
}

#endif