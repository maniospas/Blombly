/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef ERROR_HANDLING_CPP
#define ERROR_HANDLING_CPP

#include "interpreter/functional.h"
unsigned int ExecutionInstance::maxDepth = 42;
void ExecutionInstance::handleExecutionError(const Command& command, const BBError& e) {throw BBError(std::move(enrichErrorDescription(command, e.what())));}

std::string enrichErrorDescription(const Command& command, std::string message) {
    std::string comm = command.toString();
    comm.resize(40, ' ');
    if(command.descriptor) {
        size_t idx = command.descriptor->source.find("//");
        if(idx==std::string::npos || idx>=command.descriptor->source.size()-2) message += std::string("\n   \x1B[34m\u2192\033[0m ") + "\x1B[90m" + command.descriptor->source;
        else {
            std::string sourceCode = command.descriptor->source.substr(0, idx);
            sourceCode.resize(42, ' ');
            message += std::string("\n   \x1B[34m\u2192\033[0m ") + sourceCode + " \t\x1B[90m "+command.descriptor->source.substr(idx+2);
        }
    }
    else message += std::string("\n   \x1B[34m\u2192\033[0m ") + comm + " \t\x1B[90m " + command.source->path + " line " + std::to_string(command.line);
    return std::move(message);
}

#endif  // ERROR_HANDLING_CPP