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
    std::unordered_set<int> symbolDefinitions;
    for (const auto& command : *program) {
        if(command.args.size()) symbolDefinitions.insert(command.args[0]);
        if(command.operation==CALL && command.args.size()>2)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SET && command.args.size()>3)  symbolDefinitions.insert(command.args[2]);
        if(command.operation==SET && command.args.size()>2)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SET && command.args.size()>3)  symbolDefinitions.insert(command.args[2]);
        if(command.operation==SETFINAL && command.args.size()>2)  symbolDefinitions.insert(command.args[1]);
        if(command.operation==SETFINAL && command.args.size()>3)  symbolDefinitions.insert(command.args[2]);
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
            if(symbolDefinitions.find(arg)==symbolDefinitions.end()) bberrorexplain(enrichErrorDescription(command, "Missing symbol: "+variableManager.getSymbol(arg)), "There is no setting this variable at any place that would be visible here. This is a precautionary check to avoid common runtime errors.", "");
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
        optimize(fileName + "vm", fileName + "vm", true); // always minify
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
    preliminarySimpleChecks(program);

    return Result(new Code(program, 0, program->size() - 1, program->size() - 1));
}


int vm(const std::string& fileName, int numThreads) {
    Future::setMaxThreads(numThreads);
    bool hadError = false;
    try {
        {
            std::ifstream inputFile(fileName);
            bbassert(inputFile.is_open(), "Unable to open file: " + fileName);

            auto program = new std::vector<Command>();
            auto source = new SourceFile(fileName);
            std::string line;
            int i = 1;
            
            //program->emplace_back("BEGIN _bbmain", source, 0, new CommandContext("main context start"));
            CommandContext* descriptor = nullptr;
            while (std::getline(inputFile, line)) {
                if (line[0] != '%') program->emplace_back(line, source, i, descriptor);
                else descriptor = new CommandContext(line.substr(1));
                ++i;
            }
            inputFile.close();
            //program->emplace_back("END", source, program->size()-1, new CommandContext("main context end"));
            //program->emplace_back("call _bbmainresult # _bbmain", source, program->size()-1, new CommandContext("main context run"));

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
            newCode = optimizeFromCode(newCode, true); 
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

                auto code = new Code(program, 0, program->size() - 1, program->size() - 1);
                if(numThreads) {
                    ExecutionInstance executor(0, code, &memory, false);
                    auto returnedValue = executor.run(code);
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