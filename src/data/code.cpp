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


#include "data/Code.h"
#include "common.h"
#include "BMemory.h"
#include "data/Jitable.h"
#include <vector>
#include <mutex>

extern std::vector<SymbolWorries> symbolUsage;
extern std::mutex ownershipMutex;

Code::Code(const std::vector<Command>* programAt, size_t startAt, size_t endAt, size_t premature_end)
    : program(programAt), start(startAt), end(endAt), scheduleForParallelExecution(true), Data(CODE), jitable(nullptr), premature_end(premature_end) {}

std::string Code::toString(BMemory* memory){
    if(jitable) return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end) + " with "+jitable->toString();
    return "code block in .bbvm file lines " + std::to_string(start) + " to " + std::to_string(end);
}
size_t Code::getStart() const {return start;}
size_t Code::getEnd() const {return end;}
size_t Code::getOptimizedEnd() const {return premature_end;}
const std::vector<Command>* Code::getProgram() const {return program;}


CodeExiter::CodeExiter(Code* code) : code(code) {}
CodeExiter::~CodeExiter() {
    if(code->requestAccess.size() || code->requestModification.size()) {
        std::lock_guard<std::mutex> lock(ownershipMutex);
        for(int access : code->requestAccess) {
            auto& symbol = symbolUsage[access];
            symbol.access--;
        }
        for(int access : code->requestModification) {
            auto& symbol = symbolUsage[access];
            symbol.modification--;
        }
    }
}


SymbolEntrantExiter::SymbolEntrantExiter(int symbol): symbol(symbol) {}
SymbolEntrantExiter::~SymbolEntrantExiter() {
    std::lock_guard<std::mutex> lock(ownershipMutex);
    auto& symbol_ = symbolUsage[symbol];
    symbol_.access--;
}