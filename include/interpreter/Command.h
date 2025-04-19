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
    int line;
    std::shared_ptr<SourceFile> source;
    std::shared_ptr<CommandContext> descriptor;

    Command(const std::string& command, const std::shared_ptr<SourceFile>& source, int line, const std::shared_ptr<CommandContext>& descriptor);
    ~Command();
    std::string toString() const;
    std::string tocpp(bool first_assignment) const;
};

#endif // COMMAND_H
