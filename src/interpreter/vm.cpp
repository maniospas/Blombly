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


#ifndef VM_CPP
#define VM_CPP

#include <memory>
#include <mutex>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include "BMemory.h"
#include "data/Future.h"
#include "data/Code.h"
#include "data/Jitable.h"
#include "utils.h"
#include "interpreter/functional.h"

extern std::string enrichErrorDescription(const Command& command, std::string message);
extern std::unordered_map<std::string, OperationType> toOperationTypeMap;
extern BMemory cachedData;
extern std::vector<SymbolWorries> symbolUsage;

class UnionFind {
private:
    std::unordered_map<int, int> parent;
public:
    int find(int x) {
        if (parent.find(x) == parent.end()) parent[x] = x; 
        if (parent[x] != x) parent[x] = find(parent[x]); 
        return parent[x];
    }
    void merge(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY) {
            if (rootX < rootY) parent[rootY] = rootX;
            else parent[rootX] = rootY;
        }
    }
    int getParent(int x) {return find(x);}
};


void preliminaryDependencies(std::vector<Command>* program) {
    std::unordered_map<int, std::unordered_set<int>> redirects;
    std::unordered_set<int> symbols;
    std::unordered_map<int, int> mergedSymbols;
    UnionFind uf;
    for (const auto& command : *program) {
        if(command.operation==AS || command.operation==IS) {
            redirects[command.args[0]].insert(command.args[1]);
            redirects[command.args[1]].insert(command.args[0]);
            symbols.insert(command.args[0]);
            symbols.insert(command.args[1]);
        }
        if(command.operation==SET) {
            redirects[command.args[3]].insert(command.args[2]);
            redirects[command.args[2]].insert(command.args[3]);
            symbols.insert(command.args[3]);
            symbols.insert(command.args[2]);
        }
        if(command.operation==GET) {
            redirects[command.args[0]].insert(command.args[2]);
            redirects[command.args[2]].insert(command.args[0]);
            symbols.insert(command.args[0]);
            symbols.insert(command.args[2]);
        }
    }
    for(int symbol : symbols) redirects[symbol].insert(symbol);
    for(const auto& [key, values] : redirects) for (int value : values) uf.merge(key, value);
    for(int symbol : symbols) mergedSymbols[symbol] = uf.getParent(symbol);

    //std::cout << "---------------------------------\n";
    //for (const auto& [key, value] : mergedSymbols) std::cout << variableManager.getSymbol(key) << " ~ " << variableManager.getSymbol(value) << "\n";

    // uses and affects refer to used and modified struct fields
    std::unordered_map<int, std::unordered_set<int>> uses;
    std::unordered_map<int, std::unordered_set<int>> affects;
    std::unordered_map<int, std::unordered_set<int>> calls; // includes inlining
    int programSize = program->size();

    // compile code blocks while we are at it
    for(int i=-1;i<programSize;++i) {
        int commandSymbolGroup;
        int original_i = i;
        if(i==-1) {
            //i = 0; (DO NOT ADD THIS DUE TO pos=i+1)
            commandSymbolGroup = variableManager.mainScopeNameId;
        }
        else {
            const Command& command = (*program)[i];
            if(command.operation!=BEGIN && command.operation!=BEGINFINAL && command.operation!=BEGINCACHE) continue;
            commandSymbolGroup = mergedSymbols.find(command.args[0])==mergedSymbols.end()?command.args[0]:mergedSymbols[command.args[0]];
        }
        // find end of code block while checking symbols inside
        size_t pos = i + 1;
        int depth = 0;
        OperationType command_type(END);
        std::unordered_set<int> fullyControlledVariables;
        while(pos < programSize) {
            const Command& codeCommand = (*program)[pos];
            command_type = codeCommand.operation;
            if(command_type == BEGIN || command_type == BEGINFINAL) depth++;
            if(command_type == END) {
                if(depth == 0) break;
                --depth;
            }
            ++pos;
            if(depth==0) {
                if( command_type == LEN ) {
                    fullyControlledVariables.insert(codeCommand.args[0]);
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else uses[commandSymbolGroup].insert(codeCommand.args[1]);
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(variableManager.structLen);
                    else calls[commandSymbolGroup].insert(variableManager.structLen);
                    continue;
                }

                //std::cout << codeCommand.toString() << "\n";
                if(command_type == NEW || command_type == TOLIST || command_type == TOVECTOR || command_type == RANDOM || command_type == BUILTIN 
                    || command_type == TOFILE || command_type == TOGRAPHICS  || command_type == TOSQLITE  || command_type == BEGIN 
                    || command_type == BEGINFINAL
                    || command_type == TORANGE) {
                    //std::cout << "Fully controlling: "+variableManager.getSymbol(codeCommand.args[0])<<"\n";
                    if(codeCommand.args.size()) fullyControlledVariables.insert(codeCommand.args[0]);
                    continue;
                }
                if(command_type==TOITER) {
                    if(codeCommand.args.size()) fullyControlledVariables.insert(codeCommand.args[0]);
                    continue;
                }
                if(codeCommand.args.size()){
                    //std::cout << "Lost control of "+variableManager.getSymbol(codeCommand.args[0])<<"\n";
                    // blanket remove all assignments
                    if(fullyControlledVariables.find(codeCommand.args[0])!=fullyControlledVariables.end()) fullyControlledVariables.erase(codeCommand.args[0]);
                } 

                if(command_type == IS) {
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else uses[commandSymbolGroup].insert(codeCommand.args[1]);
                    if(fullyControlledVariables.find(codeCommand.args[1])!=fullyControlledVariables.end()) fullyControlledVariables.erase(codeCommand.args[1]);
                    continue;
                }
                if(command_type == INLINE) {
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[1]);
                    continue;
                }
                if(command_type == TRY) {
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[1]);
                    continue;
                }
                if(command_type == CALL) {
                    fullyControlledVariables.clear();
                    if(mergedSymbols.find(variableManager.callId)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.callId]);
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[2]);
                    if(mergedSymbols.find(codeCommand.args[3])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[3]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[3]);
                    continue;
                }
                if(command_type == WHILE) {
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[2]);
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[1]);
                    continue;
                }
                if(command_type == IF) {
                    if(mergedSymbols.find(codeCommand.args[3])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[3]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[3]);
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[2]);
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[1]);
                    continue;
                }
                if(command_type == CATCH) {
                    if(mergedSymbols.find(codeCommand.args[3])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[3]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[3]);
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[2]);
                    if(mergedSymbols.find(codeCommand.args[1])!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[1]]);
                    else calls[commandSymbolGroup].insert(codeCommand.args[1]);
                    continue;
                }
                //if(command_type == CALL) calls[commandSymbolGroup].insert(variableManager.structCall);
                //if(command_type == ADD) calls[commandSymbolGroup].insert(variableManager.structAdd);
                if(command_type == SET) {
                    if(fullyControlledVariables.find(codeCommand.args[3])!=fullyControlledVariables.end()) fullyControlledVariables.erase(codeCommand.args[3]);
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else affects[commandSymbolGroup].insert(codeCommand.args[2]);
                    continue;
                }
                if(command_type == GET) {
                    if(mergedSymbols.find(codeCommand.args[2])!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[2]]);
                    else uses[commandSymbolGroup].insert(codeCommand.args[2]);
                    continue;
                }
                if(command_type == BB_PRINT) {
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else affects[commandSymbolGroup].insert(variableManager.structStr);
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else calls[commandSymbolGroup].insert(variableManager.structStr);
                    // print modifies the console
                    if(mergedSymbols.find(variableManager.consoleId)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.consoleId]);
                    else affects[commandSymbolGroup].insert(variableManager.consoleId);
                    if(mergedSymbols.find(variableManager.consoleId)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.consoleId]);
                    else calls[commandSymbolGroup].insert(variableManager.consoleId);
                    continue;
                }
                if(command_type == READ) {
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else affects[commandSymbolGroup].insert(variableManager.structStr);
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else calls[commandSymbolGroup].insert(variableManager.structStr);
                    // read modifies the console
                    if(mergedSymbols.find(variableManager.consoleId)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.consoleId]);
                    else affects[commandSymbolGroup].insert(variableManager.consoleId);
                    if(mergedSymbols.find(variableManager.consoleId)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.consoleId]);
                    else calls[commandSymbolGroup].insert(variableManager.consoleId);
                    continue;
                }
                if(command_type == ADD) {
                    if(mergedSymbols.find(variableManager.structAdd)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structAdd]);
                    else uses[commandSymbolGroup].insert(variableManager.structAdd);
                    if(mergedSymbols.find(variableManager.structAdd)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structAdd]);
                    else calls[commandSymbolGroup].insert(variableManager.structAdd);
                    continue;
                }
                if(command_type == SUB) {
                    if(mergedSymbols.find(variableManager.structSub)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structSub]);
                    else uses[commandSymbolGroup].insert(variableManager.structSub);
                    if(mergedSymbols.find(variableManager.structSub)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structSub]);
                    else calls[commandSymbolGroup].insert(variableManager.structSub);
                    if(mergedSymbols.find(variableManager.structRSub)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structRSub]);
                    else uses[commandSymbolGroup].insert(variableManager.structRSub);
                    if(mergedSymbols.find(variableManager.structRSub)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structRSub]);
                    else calls[commandSymbolGroup].insert(variableManager.structRSub);
                    continue;
                }
                if(command_type == MUL) {
                    if(mergedSymbols.find(variableManager.structMul)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structMul]);
                    else uses[commandSymbolGroup].insert(variableManager.structMul);
                    if(mergedSymbols.find(variableManager.structMul)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structMul]);
                    else calls[commandSymbolGroup].insert(variableManager.structMul);
                    continue;
                }
                if(command_type == DIV) {
                    if(mergedSymbols.find(variableManager.structDiv)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structDiv]);
                    else uses[commandSymbolGroup].insert(variableManager.structDiv);
                    if(mergedSymbols.find(variableManager.structDiv)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structDiv]);
                    else calls[commandSymbolGroup].insert(variableManager.structDiv);
                    if(mergedSymbols.find(variableManager.structRDiv)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structRDiv]);
                    else uses[commandSymbolGroup].insert(variableManager.structRDiv);
                    if(mergedSymbols.find(variableManager.structRDiv)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structRDiv]);
                    else calls[commandSymbolGroup].insert(variableManager.structRDiv);
                    continue;
                }
                if(command_type == POW) {
                    if(mergedSymbols.find(variableManager.structPow)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structPow]);
                    else uses[commandSymbolGroup].insert(variableManager.structPow);
                    if(mergedSymbols.find(variableManager.structPow)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structPow]);
                    else calls[commandSymbolGroup].insert(variableManager.structPow);
                    if(mergedSymbols.find(variableManager.structRPow)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structRPow]);
                    else uses[commandSymbolGroup].insert(variableManager.structRPow);
                    if(mergedSymbols.find(variableManager.structRPow)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structRPow]);
                    else calls[commandSymbolGroup].insert(variableManager.structRPow);
                    continue;
                }
                if(command_type == LOG) {
                    if(mergedSymbols.find(variableManager.structLog)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structLog]);
                    else uses[commandSymbolGroup].insert(variableManager.structLog);
                    if(mergedSymbols.find(variableManager.structLog)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structLog]);
                    else calls[commandSymbolGroup].insert(variableManager.structLog);
                    continue;
                }
                if(command_type == MOD) {
                    if(mergedSymbols.find(variableManager.structMod)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structMod]);
                    else uses[commandSymbolGroup].insert(variableManager.structMod);
                    if(mergedSymbols.find(variableManager.structMod)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structMod]);
                    else calls[commandSymbolGroup].insert(variableManager.structMod);
                    if(mergedSymbols.find(variableManager.structRMod)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structRMod]);
                    else uses[commandSymbolGroup].insert(variableManager.structRMod);
                    if(mergedSymbols.find(variableManager.structRMod)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structRMod]);
                    else calls[commandSymbolGroup].insert(variableManager.structRMod);
                    continue;
                }
                if(command_type == LT) {
                    if(mergedSymbols.find(variableManager.structLT)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structLT]);
                    else uses[commandSymbolGroup].insert(variableManager.structLT);
                    if(mergedSymbols.find(variableManager.structLT)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structLT]);
                    else calls[commandSymbolGroup].insert(variableManager.structLT);
                    continue;
                }
                if(command_type == GT) {
                    if(mergedSymbols.find(variableManager.structGT)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structGT]);
                    else uses[commandSymbolGroup].insert(variableManager.structGT);
                    if(mergedSymbols.find(variableManager.structGT)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structGT]);
                    else calls[commandSymbolGroup].insert(variableManager.structGT);
                    continue;
                }
                if(command_type == LE) {
                    if(mergedSymbols.find(variableManager.structLE)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structLE]);
                    else uses[commandSymbolGroup].insert(variableManager.structLE);
                    if(mergedSymbols.find(variableManager.structLE)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structLE]);
                    else calls[commandSymbolGroup].insert(variableManager.structLE);
                    continue;
                }
                if(command_type == GE) {
                    if(mergedSymbols.find(variableManager.structGE)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structGE]);
                    else uses[commandSymbolGroup].insert(variableManager.structGE);
                    if(mergedSymbols.find(variableManager.structGE)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structGE]);
                    else calls[commandSymbolGroup].insert(variableManager.structGE);
                    continue;
                }
                if(command_type == EQ) {
                    if(mergedSymbols.find(variableManager.structEq)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structEq]);
                    else uses[commandSymbolGroup].insert(variableManager.structEq);
                    if(mergedSymbols.find(variableManager.structEq)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structEq]);
                    else calls[commandSymbolGroup].insert(variableManager.structEq);
                    continue;
                }
                if(command_type == NEQ) {
                    if(mergedSymbols.find(variableManager.structNEq)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structNEq]);
                    else uses[commandSymbolGroup].insert(variableManager.structNEq);
                    if(mergedSymbols.find(variableManager.structNEq)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structNEq]);
                    else calls[commandSymbolGroup].insert(variableManager.structNEq);
                    continue;
                }
                if(command_type == NOT) {
                    if(mergedSymbols.find(variableManager.structNot)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structNot]);
                    else uses[commandSymbolGroup].insert(variableManager.structNot);
                    if(mergedSymbols.find(variableManager.structNot)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structNot]);
                    else calls[commandSymbolGroup].insert(variableManager.structNot);
                    continue;
                }
                if(command_type == TOBB_FLOAT) {
                    if(mergedSymbols.find(variableManager.structFloat)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structFloat]);
                    else uses[commandSymbolGroup].insert(variableManager.structFloat);
                    if(mergedSymbols.find(variableManager.structFloat)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structFloat]);
                    else calls[commandSymbolGroup].insert(variableManager.structFloat);
                    continue;
                }
                if(command_type == TOBB_INT) {
                    if(mergedSymbols.find(variableManager.structInt)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structInt]);
                    else uses[commandSymbolGroup].insert(variableManager.structInt);
                    if(mergedSymbols.find(variableManager.structInt)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structInt]);
                    else calls[commandSymbolGroup].insert(variableManager.structInt);
                    continue;
                }
                if(command_type == TOBB_BOOL) {
                    if(mergedSymbols.find(variableManager.structBool)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structBool]);
                    else uses[commandSymbolGroup].insert(variableManager.structBool);
                    if(mergedSymbols.find(variableManager.structBool)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structBool]);
                    else calls[commandSymbolGroup].insert(variableManager.structBool);
                    continue;
                }
                if(command_type == TOSTR) {
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else uses[commandSymbolGroup].insert(variableManager.structStr);
                    if(mergedSymbols.find(variableManager.structStr)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structStr]);
                    else calls[commandSymbolGroup].insert(variableManager.structStr);
                    continue;
                }
                if(command_type == MAX) {
                    if(mergedSymbols.find(variableManager.structMax)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structMax]);
                    else uses[commandSymbolGroup].insert(variableManager.structMax);
                    if(mergedSymbols.find(variableManager.structMax)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structMax]);
                    else calls[commandSymbolGroup].insert(variableManager.structMax);
                    continue;
                }
                if(command_type == MIN) {
                    if(mergedSymbols.find(variableManager.structMin)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structMin]);
                    else uses[commandSymbolGroup].insert(variableManager.structMin);
                    if(mergedSymbols.find(variableManager.structMin)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structMin]);
                    else calls[commandSymbolGroup].insert(variableManager.structMin);
                    continue;
                }
                if(command_type == AND) {
                    if(mergedSymbols.find(variableManager.structAnd)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structAnd]);
                    else uses[commandSymbolGroup].insert(variableManager.structAnd);
                    if(mergedSymbols.find(variableManager.structAnd)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structAnd]);
                    else calls[commandSymbolGroup].insert(variableManager.structAnd);
                    continue;
                }
                if(command_type == OR) {
                    if(mergedSymbols.find(variableManager.structOr)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structOr]);
                    else uses[commandSymbolGroup].insert(variableManager.structOr);
                    if(mergedSymbols.find(variableManager.structOr)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structOr]);
                    else calls[commandSymbolGroup].insert(variableManager.structOr);
                    continue;
                }
                if(command_type == MMUL) {
                    if(mergedSymbols.find(variableManager.structMMul)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structMMul]);
                    else uses[commandSymbolGroup].insert(variableManager.structMMul);
                    if(mergedSymbols.find(variableManager.structMMul)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structMMul]);
                    else calls[commandSymbolGroup].insert(variableManager.structMMul);
                    continue;
                }
                if(command_type == AT) {
                    if(codeCommand.args[1]!=variableManager.argsId && fullyControlledVariables.find(codeCommand.args[1])!=fullyControlledVariables.end() ) {
                        if(mergedSymbols.find(variableManager.structPush)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.structPush]);
                        else uses[commandSymbolGroup].insert(variableManager.structPush);
                        if(mergedSymbols.find(variableManager.structPush)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structPush]);
                        else calls[commandSymbolGroup].insert(variableManager.structPush);
                        // forcefully sync everything
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else uses[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else calls[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                    }
                    continue;
                }
                if(command_type == PUT) {
                    if( codeCommand.args[1]!=variableManager.argsId && fullyControlledVariables.find(codeCommand.args[1])!=fullyControlledVariables.end() ) {
                        if(mergedSymbols.find(variableManager.structAt)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structAt]);
                        else affects[commandSymbolGroup].insert(variableManager.structAt);
                        if(mergedSymbols.find(variableManager.structAt)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structAt]);
                        else calls[commandSymbolGroup].insert(variableManager.structAt);
                        // forcefully sync everything
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else affects[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else calls[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                    }
                    continue;
                }
                if(command_type == PUSH) {
                    if(codeCommand.args[1]!=variableManager.argsId && fullyControlledVariables.find(codeCommand.args[1])==fullyControlledVariables.end()) {
                        if(mergedSymbols.find(variableManager.structAt)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structAt]);
                        else affects[commandSymbolGroup].insert(variableManager.structAt);
                        if(mergedSymbols.find(variableManager.structAt)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structAt]);
                        else calls[commandSymbolGroup].insert(variableManager.structAt);
                        // forcefully sync everything
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else affects[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else calls[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                    }
                    continue;
                }
                if(command_type == POP) {
                    if(codeCommand.args[1]!=variableManager.argsId && fullyControlledVariables.find(codeCommand.args[1])==fullyControlledVariables.end()) {
                        if(mergedSymbols.find(variableManager.structPop)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structPop]);
                        else affects[commandSymbolGroup].insert(variableManager.structPop);
                        if(mergedSymbols.find(variableManager.structPop)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structPop]);
                        else calls[commandSymbolGroup].insert(variableManager.structPop);
                        // forcefully sync everything
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else affects[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else calls[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                    }
                    continue;
                }
                if(command_type == NEXT) {
                    if(codeCommand.args[1]!=variableManager.argsId && fullyControlledVariables.find(codeCommand.args[1])==fullyControlledVariables.end()) {
                        if(mergedSymbols.find(variableManager.structNext)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.structNext]);
                        else affects[commandSymbolGroup].insert(variableManager.structNext);
                        if(mergedSymbols.find(variableManager.structNext)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.structNext]);
                        else calls[commandSymbolGroup].insert(variableManager.structNext);
                        // forcefully sync everything
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else affects[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                        if(mergedSymbols.find(variableManager.synchronizedListModification)!=mergedSymbols.end()) calls[commandSymbolGroup].insert(mergedSymbols[variableManager.synchronizedListModification]);
                        else calls[commandSymbolGroup].insert(variableManager.synchronizedListModification);
                    }
                    continue;
                }
                int assignmentPos = 0;
                if(codeCommand.args.size()>assignmentPos && codeCommand.args[0]!=variableManager.noneId) {
                    if(mergedSymbols.find(codeCommand.args[0])!=mergedSymbols.end()) affects[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[0]]);
                    else affects[commandSymbolGroup].insert(codeCommand.args[0]);
                }
                for(int c=assignmentPos+1;c<codeCommand.args.size();++c) if(codeCommand.args[c]!=variableManager.thisId) {
                    if(mergedSymbols.find(codeCommand.args[c])!=mergedSymbols.end()) uses[commandSymbolGroup].insert(mergedSymbols[codeCommand.args[c]]);
                    else uses[commandSymbolGroup].insert(codeCommand.args[c]);
                }
            }
        }
        bbassert(depth >= 0, "Code block never ended.");
        // create the code block
        if(original_i!=-1) {
            auto cache = new Code(program, i + 1, pos, command_type == END?(pos-1):pos);
            cache->addOwner();
            cache->jitable = jit(cache);
            (*program)[i].value = cache;
        }
        else i = -1;
    }

    // merge everything with dependent blocks
    int changes = 1;
    while(changes) {
        changes = 0;
        for(const auto& [key, children] : calls) {
            int prevUses = uses[key].size();
            int prevAffects = affects[key].size();
            for(int child : children) {
                for(int symbol : uses[child]) uses[key].insert(symbol);
                for(int symbol : affects[child]) affects[key].insert(symbol);
            }
            if(prevUses!=uses[key].size() || prevAffects!=affects[key].size()) changes++;
        }
    }

    std::unordered_set<int> canBeModified;

    // allocate to compiled blocks their group's affect and input symbols
    for(int i=0;i<programSize;++i) {
        const Command& command = (*program)[i];
        if(command.operation!=BEGIN && command.operation!=BEGINFINAL && command.operation!=BEGINCACHE) continue;
        int commandSymbolGroup = mergedSymbols.find(command.args[0])==mergedSymbols.end()?command.args[0]:mergedSymbols[command.args[0]];
        Code* cache = static_cast<Code*>(command.value.get());
        cache->requestAccess.reserve(uses[commandSymbolGroup].size());
        cache->requestModification.reserve(affects[commandSymbolGroup].size());
        for(int symbol : uses[commandSymbolGroup]) cache->requestAccess.push_back(symbol);
        for(int symbol : affects[commandSymbolGroup]) {
            cache->requestModification.push_back(symbol);
            canBeModified.insert(symbol);
        }
    }
    for(int symbol : affects[variableManager.mainScopeNameId]) canBeModified.insert(symbol);
    
    //std::cout << "The following symbolos can change in structs:\n";
    //for(int symbol :canBeModified) std::cout << variableManager.getSymbol(symbol) << "\n";

    // schedule for paralell execution everything that does not use modifiable struct fields and itself does not modify said fields
    for(int i=0;i<programSize;++i) {
        Command& command = (*program)[i];
        if(command.operation!=BEGIN && command.operation!=BEGINFINAL && command.operation!=BEGINCACHE) continue;
        int commandSymbolGroup = mergedSymbols.find(command.args[0])==mergedSymbols.end()?command.args[0]:mergedSymbols[command.args[0]];
        Code* cache = static_cast<Code*>(command.value.get());
        bool usesAffected(false);
        for(int symbol : uses[commandSymbolGroup]) if(canBeModified.find(symbol)!=canBeModified.end()) usesAffected = true;
        cache->scheduleForParallelExecution = !(usesAffected || cache->requestModification.size());
        
        /*std::cout << variableManager.getSymbol(command.args[0]) << "\n";
        std::cout << cache->toString(nullptr) << "\n";
        std::cout << "  uses struct fields: ";
        for(int symbol : uses[commandSymbolGroup]) std::cout<<variableManager.getSymbol(symbol)<<" ";
        std::cout << "\n  sets struct fields: ";
        for(int symbol : affects[commandSymbolGroup]) std::cout<<variableManager.getSymbol(symbol)<<" ";
        if(cache->scheduleForParallelExecution ) std::cout << "\n  Can run in parallel";
        std::cout << "\n";*/
    }

    // reserve symbol usage in variable manager
    symbolUsage.resize(variableManager.size());
}


void preliminarySimpleChecks(std::vector<Command>* program) {
    // the following is a sanity check to prevent external bbvm code from being invalid
    int depth = 0;
    for (const auto& command : *program) {
        auto op =command.operation;
        auto size = command.args.size();
        if(size==0 && op!=END && op!=BEGINCACHE) bberrorexplain("Invalid bbvm instruction: "+command.toString(), "Expecting a return value (even if that is `#`)", getStackFrame(command));
        if(op==NOT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`not` accepts exactly one argument after the return value", getStackFrame(command));
        if(op==AND) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`and` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==OR) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`or` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==EQ) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`eq` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==NEQ) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`neq` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==LE) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`le` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==GE) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`ge` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==LT) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`lt` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==GT) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`gt` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==ADD) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`add` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==SUB) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`sub` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==MUL) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`mul` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==MMUL) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`mul` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==MUL) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`mul` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==MMUL) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`mmul` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==DIV) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`div` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==LEN) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`len` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==POW) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`pow` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==LOG) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`log` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==PUSH) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`push` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==POP) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`pop` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==NEXT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`next` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==PUT) bbassertexplain(size==4, "Invalid bbvm instruction: "+command.toString(), "`put` accepts exactly 3 arguments after the return value", getStackFrame(command));
        if(op==AT) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`at` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==SHAPE) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`shape` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==TOVECTOR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`vector` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOLIST) bbassertexplain(size==2 || size==1, "Invalid bbvm instruction: "+command.toString(), "`list` accepts 1 or no arguments after the return value", getStackFrame(command));
        if(op==TOMAP) bbassertexplain(size==2 || size==1, "Invalid bbvm instruction: "+command.toString(), "`map` accepts 1 or no arguments after the return value", getStackFrame(command));
        if(op==TOBB_INT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`int` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOBB_FLOAT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`float` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOSTR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`str` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOBB_BOOL) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`bool` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOFILE) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`file` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==SUM) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`sum` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==MAX) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`max` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==MIN) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`min` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==BUILTIN) bbassertexplain(size==1, "Invalid bbvm instruction: "+command.toString(), "`BUILTIN` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==BEGIN) bbassertexplain(size==1, "Invalid bbvm instruction: "+command.toString(), "`BEGIN` accepts no argument after the return value", getStackFrame(command));
        if(op==BEGINFINAL) bbassertexplain(size==1, "Invalid bbvm instruction: "+command.toString(), "`BEGINFINAL` accepts no argument after the return value", getStackFrame(command));
        if(op==BEGINCACHE) bbassertexplain(size==0, "Invalid bbvm instruction: "+command.toString(), "`CACHE` accepts no argument and, by exception, assigns to no value", getStackFrame(command));
        if(op==END) bbassertexplain(size==0, "Invalid bbvm instruction: "+command.toString(), "`END` accepts no argument and, by exception, assigns to no return value", getStackFrame(command));
        if(op==RETURN) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`return` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==FINAL) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`final` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==IS) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`IS` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==CALL) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`call` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==WHILE) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`while` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==IF) bbassertexplain(size==4 || size==3, "Invalid bbvm instruction: "+command.toString(), "`if` accepts 2 or 3 arguments after the return value", getStackFrame(command));
        if(op==NEW) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`new` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==BB_PRINT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`print` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==INLINE) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`inline` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==GET) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`get` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==SET) bbassertexplain(size==4, "Invalid bbvm instruction: "+command.toString(), "`set` accepts exactly 3 arguments after the return value", getStackFrame(command));
        if(op==SETFINAL) bbassertexplain(size==4, "Invalid bbvm instruction: "+command.toString(), "`setfinal` accepts exactly 3 arguments after the return value", getStackFrame(command));
        if(op==DEFAULT) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`default` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TIME) bbassertexplain(size==1, "Invalid bbvm instruction: "+command.toString(), "`default` accepts no arguments after the return value", getStackFrame(command));
        if(op==TOITER) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`iter` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TRY) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`try` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==CATCH) bbassertexplain(size==4 || size==3, "Invalid bbvm instruction: "+command.toString(), "`catch` accepts 2 or 3 arguments after the return value", getStackFrame(command));
        if(op==FAIL) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`fail` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==EXISTS) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`exists` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==READ) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`read` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==CREATESERVER) bbassertexplain(size==3, "Invalid bbvm instruction: "+command.toString(), "`server` accepts exactly 2 arguments after the return value", getStackFrame(command));
        if(op==AS) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`as` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TORANGE) bbassertexplain(size>=2 && size<=4, "Invalid bbvm instruction: "+command.toString(), "`range` accepts 1 to 3 arguments after the return value", getStackFrame(command));
        if(op==DEFER) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`defer` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==CLEAR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`clear` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==MOVE) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`move` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==ISCACHED) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`ISCACHED` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOSQLITE) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`sqllite` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==TOGRAPHICS) bbassertexplain(size==4, "Invalid bbvm instruction: "+command.toString(), "`graphics` accepts exactly 3 arguments after the return value", getStackFrame(command));
        if(op==RANDOM) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`random` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==RANDVECTOR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`vector::consume` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==ZEROVECTOR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`vector::zero` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==ALLOCVECTOR) bbassertexplain(size==2, "Invalid bbvm instruction: "+command.toString(), "`vector::alloc` accepts exactly 1 argument after the return value", getStackFrame(command));
        if(op==LISTELEMENT) bbassertexplain(size>=2, "Invalid bbvm instruction: "+command.toString(), "`list::element` accepts at least 1 argument after the return value", getStackFrame(command));
        if(op==LISTGATHER) bbassertexplain(size>=2, "Invalid bbvm instruction: "+command.toString(), "`list::gather` accepts at least 1 argument after the return value", getStackFrame(command));

        if(op==BEGIN || op==BEGINCACHE || op==BEGINFINAL) depth++;
        if(op==END) {
            depth--;
            bbassertexplain(depth>=0, "Unexpected bbvm instruction: "+command.toString(), "`END` can only be encountered once a code block declaration has began by `CACHE`, `BEGIN`, or `BEGINFINAL` instructions. If this file was a compilation outcome, it may now be corrupted or tempered.", getStackFrame(command));
        }
    }
    if(depth) bberrorexplain("Unexpected end of bbvm file", "There are "+std::to_string(depth)+" missing `END` instructions. These should have been placed throughout the file. If this file was a compilation outcome, it may now be corrupted or tempered.", "");


    std::unordered_set<int> symbolDefinitions;
    for (const auto& command : *program) {
        if(command.args.size()) symbolDefinitions.insert(command.args[0]);
        if(command.operation==CALL)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SET)  symbolDefinitions.insert(command.args[2]);
        if(command.operation==SET)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SET)  symbolDefinitions.insert(command.args[2]);
        if(command.operation==SETFINAL)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SETFINAL)  symbolDefinitions.insert(command.args[2]);
    }
    symbolDefinitions.insert(variableManager.getId("io::key"));
    symbolDefinitions.insert(variableManager.getId("io::type"));
    symbolDefinitions.insert(variableManager.getId("io::x"));
    symbolDefinitions.insert(variableManager.getId("io::y"));
    symbolDefinitions.insert(variableManager.getId("io::username"));
    symbolDefinitions.insert(variableManager.getId("io::password"));
    symbolDefinitions.insert(variableManager.getId("io::timeout"));
    symbolDefinitions.insert(variableManager.getId("server::uri"));
    symbolDefinitions.insert(variableManager.getId("server::query"));
    symbolDefinitions.insert(variableManager.getId("server::method"));
    symbolDefinitions.insert(variableManager.getId("server::http"));
    symbolDefinitions.insert(variableManager.getId("server::ip"));
    symbolDefinitions.insert(variableManager.getId("server::ssl"));
    symbolDefinitions.insert(variableManager.getId("server::content"));
    for (const auto& command : *program) {
        for(int arg : command.args) {
            if(arg==variableManager.thisId || arg==variableManager.noneId || arg==variableManager.argsId) continue;
            if(symbolDefinitions.find(arg)==symbolDefinitions.end()) bberrorexplain("Missing symbol: "+variableManager.getSymbol(arg), 
                "This symbol will never be visible from here with a value already set elsewhere (e.g., in a final parent block or through a struct or inlined code block). This is a precautionary check to avoid common runtime errors.", 
                getStackFrame(command));
        }
    }
    preliminaryDependencies(program);
}


Result compileAndLoad(const std::string& fileName, BMemory* currentMemory) {
    std::lock_guard<std::recursive_mutex> lock(compileMutex);

    // Compile and optimize
    std::string file = fileName;
    if (fileName.substr(fileName.size() - 3, 3) == ".bb") {
        compile(fileName, fileName + "vm");
        optimize(fileName + "vm", fileName + "vm", true, true); // always minify and compress
        file = fileName + "vm";
    }

    // Open the compiled .bbvm file
    std::ifstream inputFile(file);
    if (!inputFile.is_open()) {
        bberror("Unable to open file: " + file);
        return Result(DataPtr::NULLP);
    }

    // Organize each line into a new assembly command
    auto program = new std::vector<Command>();
    auto source = new SourceFile(file);
    std::string line;
    int i = 1;
    while (std::getline(inputFile, line)) {
        if (line[0] != '%') program->emplace_back(line, source, i, nullptr);
        ++i;
    }
    inputFile.close();
    // the following ensure smooth close-up even if the program is terminated through logically a non-assigned call
    program->emplace_back("BUILTIN _bbdonothing I0", source, i, nullptr);
    preliminarySimpleChecks(program);

    return Result(new Code(program, 0, program->size() - 1, program->size() - 1));
}


int vm(const std::string& fileName, int numThreads) {
    Future::setMaxThreads(numThreads);
    bool hadError = false;
    try {
        {
            std::unique_ptr<std::istream> inputFile;

            try {
                std::string contents = read_decompressed(fileName);
                inputFile = std::make_unique<std::stringstream>(contents);
            } 
            catch (...) {
                std::unique_ptr<std::ifstream> input = std::make_unique<std::ifstream>(fileName);
                bbassert(input->is_open(), "Unable to open file: " + fileName);
                inputFile = std::move(input);
            }

            auto program = new std::vector<Command>();
            auto source = new SourceFile(fileName);
            std::string line;
            int i = 1;
            
            //program->emplace_back("BEGIN _bbmain", source, 0, new CommandContext("main context start"));
            CommandContext* descriptor = nullptr;
            while (std::getline(*inputFile, line)) {
                if (line[0] != '%') program->emplace_back(line, source, i, descriptor);
                else descriptor = new CommandContext(line.substr(1));
                ++i;
            }

            //program->emplace_back("END", source, program->size()-1, new CommandContext("main context end"));
            //program->emplace_back("call _bbmainresult # _bbmain", source, program->size()-1, new CommandContext("main context run"));

            // the following ensure smooth close-up even if the program is terminated through logically a non-assigned call
            program->emplace_back("BUILTIN _bbdonothing I0", source, i, descriptor);
            preliminarySimpleChecks(program);
            
            BMemory memory(0, nullptr, DEFAULT_LOCAL_EXPECTATION);
            try {
                auto code = new Code(program, 0, program->size() - 1, program->size() - 1);
                ExecutionInstance executor(0, code, &memory, false);
                auto returnedValue = executor.run(code);
                if(returnedValue.get().existsAndTypeEquals(ERRORTYPE)) throw BBError(returnedValue.get()->toString(nullptr));
                bbassert(!returnedValue.returnSignal, "The virtual machine cannot return a value.");
                //memory.detach(nullptr);
            }
            catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        cachedData.release();
        BMemory::verify_noleaks();
        //std::cout<<"Program completed successfully\n";
    } catch (const BBError& e) {
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }
    if(hadError) {
        std::cerr << "Docs and bug reports for the Blombly language: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

extern std::string compileFromCode(const std::string& code, const std::string& source);
extern std::string optimizeFromCode(const std::string& code, bool minimify);

int vmFromSourceCode(const std::string& sourceCode, int numThreads) {
    Future::setMaxThreads(numThreads);
    bool hadError = false;
    try {
        {
            std::string newCode = compileFromCode(sourceCode, "terminal argument");
            if(!newCode.size()) return 0;
            newCode = optimizeFromCode(newCode, true);
            if(!newCode.size()) return 0;
            BMemory memory(0, nullptr, DEFAULT_LOCAL_EXPECTATION);
            try {
                std::istringstream inputFile(newCode);

                auto program = new std::vector<Command>();
                auto source = new SourceFile("terminal argument");
                std::string line;
                int i = 1;
                
                CommandContext* descriptor = nullptr;
                while (std::getline(inputFile, line)) {
                    if (line[0] != '%') program->emplace_back(line, source, i, descriptor);
                    else descriptor = new CommandContext(line.substr(1));
                    ++i;
                }
                // the following ensure smooth close-up even if the program is terminated through logically a non-assigned call
                program->emplace_back("BUILTIN _bbdonothing I0", source, i, descriptor);
                preliminarySimpleChecks(program);

                auto code = new Code(program, 0, program->size() - 1, program->size() - 1);
                if(numThreads) {
                    ExecutionInstance executor(0, code, &memory, false);
                    auto returnedValue = executor.run(code);
                    if(returnedValue.get().existsAndTypeEquals(ERRORTYPE)) throw BBError(returnedValue.get()->toString(nullptr));
                    bbassert(!returnedValue.returnSignal, "The virtual machine cannot return a value.");
                }
                //memory.detach(nullptr);
            }
            catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        cachedData.release();
        BMemory::verify_noleaks();
        //std::cout<<"Program completed successfully\n";
    } catch (const BBError& e) {
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }
    if(hadError) {
        std::cerr << "Docs and bug reports for the Blombly language: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

#endif