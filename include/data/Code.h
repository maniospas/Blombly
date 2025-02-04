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

#ifndef CODE_H
#define CODE_H

#include <memory>
#include <string>
#include <vector>
#include "data/Data.h"
#include "tsl/hopscotch_map.h"

class BMemory;
class Command;
class Jitable;

struct SymbolWorries {
    int access;
    int modification;
    SymbolWorries() {
        access = 0;
        modification = 0;
    }
    SymbolWorries(const SymbolWorries& other) {
        access = (int)other.access;
        modification =(int)other.modification;
    }
    SymbolWorries(SymbolWorries&& other) {
        access = (int)other.access;
        modification =(int)other.modification;
    }
};

class Code : public Data {
private:
    size_t start, end, premature_end;
    const std::vector<Command>* program;

public:
    bool scheduleForParallelExecution;
    Jitable* jitable;
    
    explicit Code(const std::vector<Command>* programAt, size_t startAt, size_t endAt, size_t premature_end);
    Code* copy() const {Code* ret = new Code(program, start, end, premature_end);ret->jitable=jitable;ret->scheduleForParallelExecution=scheduleForParallelExecution; return ret;}
    std::string toString(BMemory* memory)override;
    size_t getStart() const;
    size_t getEnd() const;
    size_t getOptimizedEnd() const;
    const std::vector<Command>* getProgram() const;

    std::vector<int> requestAccess;
    std::vector<int> requestModification;
};

class CodeExiter {
    Code* code;
public:
    CodeExiter(Code* code);
    CodeExiter(const CodeExiter&) = delete;
    CodeExiter(CodeExiter&&) = delete;
    ~CodeExiter();
};


class SymbolEntrantExiter {
    int symbol;
public:
    SymbolEntrantExiter(int symbol);
    SymbolEntrantExiter(const SymbolEntrantExiter&) = delete;
    SymbolEntrantExiter(SymbolEntrantExiter&&) = delete;
    ~SymbolEntrantExiter();
};



#endif // CODE_H
