#ifndef ERROR_HANDLING_CPP
#define ERROR_HANDLING_CPP

#include "interpreter/functional.h"

void handleExecutionError(std::vector<Command*>* program, int i, const BBError& e) {
    Command* command = program->at(i);
    std::string comm = command->toString();
    comm.resize(40, ' ');
    std::string message = e.what();
    if(command->descriptor) {
        int idx = command->descriptor->source.find("//");
        if(idx==std::string::npos || idx>=command->descriptor->source.size()-2)
            message += std::string("\n   \x1B[34m\u2192\033[0m ") + "\x1B[90m" + command->descriptor->source;
        else {
            std::string sourceCode = command->descriptor->source.substr(0, idx);
            sourceCode.resize(42, ' ');
            message += std::string("\n   \x1B[34m\u2192\033[0m ") + sourceCode + " \t\x1B[90m "+command->descriptor->source.substr(idx+2);
        }
    }
    else
        message += std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command->source->path + " line " + std::to_string(command->line);
    throw BBError(std::move(message));
}

#endif