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


#ifndef OPTIMIZER_CPP
#define OPTIMIZER_CPP

#include <string>
#include "utils.h"
#include "common.h"
#include <unordered_map> 
#include <iostream>
#include <fstream>
#include <sstream>
//#define XXH_NO_XXH3
//#define XXH_NO_LONG_LONG
#define XXH_NO_STREAM
#include "xxhash.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdint>
 
 #define DISABLE {command->enabled = false;++changes;continue;}
 
 
 class OptimizerCommand {
 public:
     std::vector<std::string> args;
     bool enabled;
     std::string info;
     OptimizerCommand(std::string command) {
         if(command.size() && command[0]=='%') {
             info = command;
             enabled = false;
             args.push_back("");
             return;
         }
         enabled = true;
         std::string accumulate;
         size_t pos = 0;
         bool inString = false;
         while(pos<command.size()){
             // strings can only be the last arguments of builtin types
             if(command[pos]=='"' && !inString)
                 inString = !inString;
             if(inString && pos==command.size()-1) {
                 accumulate += command[pos];
                 args.push_back(accumulate);
                 break;
             }
             if(!inString && (command[pos]==' ' || pos==command.size()-1)){
                 if(command[pos]!=' ')
                     accumulate += command[pos];
                 args.push_back(accumulate);
                 accumulate = "";
             }
             else
                 accumulate += command[pos];
             pos += 1;
         }
     }
     std::string toString() {
         if(args.size()==0 || !enabled) return "";
         std::string ret = args[0];
         for(size_t i=1;i<args.size();i++) ret = ret + " "+args[i];
         return ret+"\n";
     }
 };
 
 
 void moveRangeToFront(std::vector<std::shared_ptr<OptimizerCommand>>& program, size_t i, size_t j, size_t front) {
     if (i > j || j >= program.size() || front >= program.size()) throw std::out_of_range("Invalid indices");
     std::vector<std::shared_ptr<OptimizerCommand>> temp(program.begin() + i, program.begin() + j + 1);
     program.erase(program.begin() + i, program.begin() + j + 1);
     program.insert(program.begin()+front, temp.begin(), temp.end());
 }
 
 std::string optimizeFromCode(const std::string& code, bool minimify) {
     // there's little meaning to also creatint a cache, so we won't do it
     std::istringstream inputStream(code);
     std::vector<std::shared_ptr<OptimizerCommand>> program;
     std::string line;
     while (std::getline(inputStream, line)) program.push_back(std::make_shared<OptimizerCommand>(line));
 
     // remove all IS (not AS) that follow an assignment that is not AT or GET
     for(size_t i=0;i<program.size();i++) {
         std::shared_ptr<OptimizerCommand> command = program[i];
         if(command->args.size()<3 || command->args[0]!="IS") continue;
         std::string to_replace = command->args[1];
         std::string symbol = command->args[2];
         if(symbol.size() <3 || symbol.substr(0,3)!="_bb") continue;
         int declaration = i-1; // for the time being this issue can arise only by adding an IS after an immediate command
         if(declaration>=0 && program[declaration]->args.size()>1 
             && program[declaration]->args[0]!="IS" 
             && program[declaration]->args[1] == symbol 
             && program[declaration]->args[0] != "at" 
             && program[declaration]->args[0] != "get"
             && program[declaration]->args[0] != "call" // calling on a method on the objet's self creates an error for the pattern `point=point.copy()`
             ) {
             program[declaration]->args[1] = to_replace;
             program[i]->enabled = false;
         }
         else if(declaration>=0 && program[declaration]->args[0]=="END") {
             // this optimizes away code block creation that is immediately assigned to a variable
             size_t starting = declaration;
             int depth = 0;
             while(starting>0) {
                 if(program[starting]->args[0]=="END") depth++;
                 if(program[starting]->args[0]=="BEGIN" || program[starting]->args[0]=="BEGINFINAL") {
                     depth--;
                     if(depth==0) {
                         if(program[starting]->args.size()>1 && symbol==program[starting]->args[1]) {
                             program[starting]->args[1] = to_replace;
                             program[i]->enabled = false;
                         }
                         break;
                     }
                 }
                 starting--;
             }
         }
     }
 
     // remove outputs from some expressions
     for(size_t i=0;i<program.size();i++) {
         std::shared_ptr<OptimizerCommand> command = program[i];
         if(command->args.size()<2) continue;
         if(command->args[0]=="put" || command->args[0]=="clear" || command->args[0]=="setfinal" || command->args[0]=="set" || command->args[0]=="final") command->args[1] = "#";
         //if(command->args[0]=="push" && command->args[1]=="#") command->args[1]  = command->args[2];
     }
     
     // remove unused methods
     int changes = minimify?-1:0; // skip the loop if not minifying
     while(changes!=0) {
         std::unordered_map<std::string, int> symbolUsageCount;
         for (const auto& command : program) {
             if(!command->enabled || command->args.size()==0) continue;
             if(command->args[0]=="END" || command->args[0]=="BEGIN" || command->args[0]=="BEGINFINAL" || command->args[0]=="final") continue;
             size_t j = 2;
             if(command->args[0]=="set" || command->args[0]=="push") {
                 const std::string& symbol = command->args[j];
                 if (symbol == "LAST") bberror("Internal error: the LAST keyword has been deprecated");
                 if (symbol != "#")  symbolUsageCount[symbol]++;
                 ++j;
             }
             for (; j < command->args.size(); ++j) {
                 const std::string& symbol = command->args[j];
                 if (symbol == "LAST") bberror("Internal error: the LAST keyword has been deprecated");
                 if (symbol != "#")  symbolUsageCount[symbol]++;
             }
         }
         changes = 0;
         for (size_t i=0;i<program.size();++i) {
             auto& command = program[i];
             if(!command->enabled) continue;
             if(i<program.size()-1 && program[i+1]->args[0]=="END") continue;
 
             //if(command->args.size()>=2 && command->args[1].size() && command->args[1][0]=='\\')  // operators are still valid
             //    continue;
             
             if(command->args.size()>=2 && (command->args[1]=="put"
                 || command->args[1]=="at" 
                 || command->args[1]=="call" 
                 || command->args[1]=="str" 
                 || command->args[1]=="float" 
                 || command->args[1]=="int" 
                 || command->args[1]=="bool"
                 || command->args[1]=="list"
                 || command->args[1]=="vector"
                 || command->args[1]=="add" 
                 || command->args[1]=="sub" 
                 || command->args[1]=="mul" 
                 || command->args[1]=="div" 
                 || command->args[1]=="mod" 
                 || command->args[1]=="pow" 
                 || command->args[1]=="and" 
                 || command->args[1]=="or"
                 || command->args[1]=="type"
                 || command->args[1]=="le"
                 || command->args[1]=="lt"
                 || command->args[1]=="ge"
                 || command->args[1]=="gt"
                 || command->args[1]=="eq"
                 || command->args[1]=="neq"
                 || command->args[1]=="clear"
                 || command->args[1]=="next"
                 || command->args[1]=="push"
                 || command->args[1]=="move"
                 )) {
                 continue;
             }
             if(command->args.size() && command->args[0]=="exists" && symbolUsageCount[command->args[1]]==0) DISABLE;
             if(command->args.size()<=1) continue;
             if(command->args[0]=="call" && command->args.size()>=3 && symbolUsageCount[command->args[1]]==0) {
                //it's pretty important to remove unused call outputs because this lets us identify return errors
                command->args[1]="#"; 
                continue;
             } 
             if(command->args[0]=="final" && command->args.size()>=3 && symbolUsageCount[command->args[2]]==0) DISABLE;
             if(command->args[0]=="set" && command->args.size()>=4 && (symbolUsageCount[command->args[2]]==0 || symbolUsageCount[command->args[3]]==0)) DISABLE;
             if(command->args[0]=="push" && command->args.size()>=4 && (symbolUsageCount[command->args[2]]==0 || symbolUsageCount[command->args[3]]==0)) DISABLE;
             if(command->args[0]=="BUILTIN" && command->args.size() && symbolUsageCount[command->args[1]]==0) DISABLE;
             if((command->args[0]=="IS" || command->args[0]=="AS" || command->args[0]=="new") && command->args.size() && symbolUsageCount[command->args[1]]==0) DISABLE;
             if(command->args[0]!="BEGIN" && command->args[0]!="BEGINFINAL") continue;
             if(symbolUsageCount[command->args[1]]!=0) continue;
             // std::cout << "removing "<<command->args[0]<<" "<<command->args[1]<<" "<<command->enabled<<"\n";
             i = i+1;
             int depth = 1;
             ++changes;
             command->enabled = false;
             while(i<program.size()) {
                 program[i]->enabled = false;
                 if(program[i]->args[0]=="BEGIN" || program[i]->args[0]=="BEGINFINAL") depth += 1;
                 if(program[i]->args[0]=="END") depth -= 1;
                 if(depth==0) break;
                 ++i;
             }
         }
     }
 
 
     //export the program
     std::string result("");
     int n = program.size();
     std::string towrite("");
     for (int i = 0; i < n; i++) {
         if (program[i]->enabled && !towrite.empty()) {
             result += towrite + "\n";
             towrite.clear();
         }
         if (!program[i]->info.empty()) towrite = program[i]->info;
         else result += program[i]->toString();
     }
 
 
     return RESMOVE(result);
 }



 
 std::string cleanSymbols(const std::string& code) {
    std::istringstream inputStream(code);
    std::vector<std::shared_ptr<OptimizerCommand>> program;
    std::string line;
    while (std::getline(inputStream, line)) program.push_back(std::make_shared<OptimizerCommand>(line));
    std::unordered_map<std::string, std::string> symbols;
    
    int n = program.size();
    for (int i = 0; i < n; i++) {
        for(int j=1;j<program[i]->args.size();++j) {
            std::string& arg = program[i]->args[j];
            if(arg.size()>=3 && arg.substr(0,3)=="_bb") {
                if(arg.size()>=8 && arg.substr(0, 8)=="_bbmacro") {
                    if(symbols.find(arg)==symbols.end()) symbols[arg] = "_bbmacro"+std::to_string(symbols.size());
                    arg = symbols[arg];
                }
                else {
                    if(symbols.find(arg)==symbols.end()) symbols[arg] = "_bb"+std::to_string(symbols.size());
                    arg = symbols[arg];
                }
            }
        }
    }


     //export the program
     std::string result("");
     std::string towrite("");
     for (int i = 0; i < n; i++) {
         if (program[i]->enabled && !towrite.empty()) {
             result += towrite + "\n";
             towrite.clear();
         }
         if (!program[i]->info.empty()) towrite = program[i]->info;
         else result += program[i]->toString();
     }
 
 
     return RESMOVE(result);

 }


 std::string int64_to_string(int64_t value) {
    std::string result(8, '\0'); // Create a string with 8 null characters
    std::memcpy(&result[0], &value, 8); // Copy raw bytes of int64_t into string
    return result;
}

 std::string blockToCodeWithoutFirst(const std::vector<std::shared_ptr<OptimizerCommand>>& program, int& pos, bool includeComments, const std::string prefix) {
    // we will be skippint the first line
    int depth = 1;
    std::string towrite("");
    std::string result("");
    int n = program.size();
    std::unordered_map<std::string, std::string> anonymize; // will anonymize only _bb prefixes
    for (int i = pos+1; i < n; i++) {
        if(program[i]->args.size()>=2) {
            auto& arg = program[i]->args[1];
            if(arg.size()>=3 && arg.substr(0, 3)=="_bb" 
                && (arg.size()<8 || arg.substr(0, 8)!="_bbcache")
                && (arg.size()<7 || arg.substr(0, 7)!="_bbpass")) {
                //std::string matched_to = prefix + std::to_string(anonymize.size());
                std::string matched_to = program[i]->args[0]; 
                for(int j=2;j<program[i]->args.size();++j) {
                    auto& arg = program[i]->args[j];
                    const auto& it = anonymize.find(arg);
                    if(it!=anonymize.end()) matched_to += " "+it->second;
                    else matched_to += " "+arg;
                }
                XXH128_hash_t hashval = XXH3_128bits(matched_to.c_str(), matched_to.size());
                std::stringstream ss;
                ss << std::hex
                << std::setw(16) << std::setfill('0') << hashval.high64
                << std::setw(16) << std::setfill('0') << hashval.low64;
                std::string hex_str = ss.str();
                
                matched_to = "_bbpass" + hex_str + std::to_string(anonymize.size());

                auto [it, inserted] = anonymize.try_emplace(arg, matched_to);
                arg = it->second;
            }
        }

        for(int j=2;j<program[i]->args.size();++j) {
            auto& arg = program[i]->args[j];
            const auto& it = anonymize.find(arg);
            if(it!=anonymize.end()) arg = it->second;
            /*int j = 0;
            if(program[i]->args[j].size()>=3 
                && program[i]->args[j].substr(0, 3)=="_bb" 
                && (program[i]->args[j].size()<=8 || program[i]->args[j].substr(0, 8)!="_bbcache")) {
                auto& arg = program[i]->args[j];
                auto [it, inserted] = anonymize.try_emplace(arg, prefix + std::to_string(anonymize.size()));
                arg = it->second;
            }*/
        }
        program[i]->enabled = true;
        if(!towrite.empty()) {
            result += towrite + "\n";
            towrite.clear();
        }
        if(!program[i]->info.empty()) {
            if(includeComments) towrite = program[i]->info;
        }
        else result += program[i]->toString();
        program[i]->enabled = false;
        if(program[i]->args.size()==0) continue;
        if(program[i]->args[0]=="BEGIN") depth++;
        if(program[i]->args[0]=="END") {
            depth--;
            pos = i;
            if(depth==0) return RESMOVE(result);
        }
    }
    bberror("Imbalanced code blocks in bbvm file");
 }


std::string removeCacheDuplicates(std::unordered_map<std::string, int> &programToCacheIndex,  int& cacheIndex, const std::string& code, int& diff, int repetition) {
    std::istringstream inputStream(code);
    std::vector<std::shared_ptr<OptimizerCommand>> program;
    std::string line;
    while (std::getline(inputStream, line)) program.push_back(std::make_shared<OptimizerCommand>(line));

    std::string cachePreample("");
    int pos = 0;
    while(pos<program.size()) {
        // the following prevents both _bbcache and intermediate inlines starting with _bb (e.g., used in while loops or expanded from macros)
        if(program[pos]->args.size()!=0 && program[pos]->args[0]=="BEGIN" && program[pos]->enabled 
                && (program[pos]->args[1].size()<8 || program[pos]->args[1].substr(0, 8)!="_bbcache") ) { 
            diff++;
            int i = pos;
            // The following anonymization prefix loses only a little bit of inference power 
            // that enables speedups (when very different methods have the same name and use different arguments)
            // but enables going through the code in linear time.
            // Therefore, an expected base case of repeatedly going through removeCacheDuplicates until
            // convergence is O(n log n). The worst case O(n^2) is completely unrealistic as it requires
            // both a negligible fraction of business logic in each function and each one to only declare
            // one other function inside. 
            //
            // Example of worst case (note: no arguments because we'd lose the worst case): 
            // `foo = {return {return {return {return {return 0}}}}}`
            std::string prefix;
            if(program[pos]->args[1].size()>=3 && program[pos]->args[1].substr(0, 3)=="_bb") prefix = "_bbpass"+std::to_string(repetition)+"_anon";
            else prefix = "_bb_"+program[pos]->args[1];
            std::string repr = blockToCodeWithoutFirst(program, pos, false, prefix); 
            if(programToCacheIndex.find(repr)==programToCacheIndex.end()) {
                pos = i;
                programToCacheIndex[repr] = cacheIndex;
                cachePreample += "BEGIN _bbcache"+std::to_string(cacheIndex)+"\n";
                cachePreample += blockToCodeWithoutFirst(program, pos, true, prefix);
                cacheIndex++;
            }
            program[i]->args[0] = "ISCACHED";
            program[i]->args.push_back("_bbcache"+std::to_string(programToCacheIndex[repr]));
        }
        ++pos;
    }

    if(cachePreample.size()) cachePreample = "CACHE\n"+cachePreample+"END\n";
     //export the program
     std::string result(std::move(cachePreample));
     int n = program.size();
     std::string towrite("");
     for (int i = 0; i < n; i++) {
         if (program[i]->enabled && !towrite.empty()) {
             result += towrite + "\n";
             towrite.clear();
         }
         if (!program[i]->info.empty()) towrite = program[i]->info;
         else result += program[i]->toString();
     }
 
     return RESMOVE(result);
 }
#include <zlib.h>
 void write_compressed(const std::string& destination, const std::string& data) {
    // Allocate a buffer for compressed data
    uLong sourceLen = data.size();
    uLong destLen = compressBound(sourceLen);
    std::vector<Bytef> compressed(destLen);

    // Compress the data
    int res = compress(compressed.data(), &destLen, reinterpret_cast<const Bytef*>(data.data()), sourceLen);
    if (res != Z_OK) {
        throw std::runtime_error("Compression failed");
    }

    // Write compressed data to file
    std::ofstream outputFile(destination, std::ios::binary);
    if (!outputFile.is_open()) {
        throw std::runtime_error("Unable to write to file: " + destination);
    }
    outputFile.write(reinterpret_cast<char*>(&sourceLen), sizeof(sourceLen));
    outputFile.write(reinterpret_cast<char*>(compressed.data()), destLen);
    outputFile.close();
}

std::string read_decompressed(const std::string& source) {
    std::ifstream inputFile(source, std::ios::binary);
    if (!inputFile.is_open()) throw std::runtime_error("Unable to read from file: " + source);

    uLong originalSize = 0;
    inputFile.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
    if (inputFile.gcount() != sizeof(originalSize)) throw std::runtime_error("Failed to read original size header.");

    std::vector<Bytef> compressedData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    std::vector<char> decompressed(originalSize);
    int res = uncompress(reinterpret_cast<Bytef*>(decompressed.data()), &originalSize, compressedData.data(), compressedData.size());
    if (res != Z_OK) throw std::runtime_error("Decompression failed");

    return std::string(decompressed.data(), originalSize);
}

 
void optimize(const std::string& source, const std::string& destination, bool minimify, bool compress) {
    std::ifstream inputFile(source);
    bbassert(inputFile.is_open(), "Unable to open file: " + source);
    std::string code = "";
    std::string line;
    while (std::getline(inputFile, line)) code += line + "\n";
    inputFile.close();

    std::string optimized = optimizeFromCode(code, minimify);
    int diff = minimify?-1:0;
    std::unordered_map<std::string, int> codeBlockToCacheIndex;
    int cacheIndex = 0;
    int repetition = 0;
    while(diff) {
        diff = 0;
        optimized = removeCacheDuplicates(codeBlockToCacheIndex, cacheIndex, optimized, diff, repetition);
        repetition++;
    }
    optimized = cleanSymbols(optimized);

    if(compress) {
        write_compressed(destination, optimized);
        return;
    }
    std::ofstream outputFile(destination);
    bbassert(outputFile.is_open(), "Unable to write to file: " + destination);
    outputFile << optimized;
    outputFile.close();
}
 
#endif