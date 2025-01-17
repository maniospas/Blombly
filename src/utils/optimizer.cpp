#ifndef OPTIMIZER_CPP
#define OPTIMIZER_CPP

#include <string>
#include <iostream>
#include "stringtrim.cpp"
#include "common.h"
#include <unordered_map> 

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
        int pos = 0;
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
        for(int i=1;i<args.size();i++) ret = ret + " "+args[i];
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
    std::istringstream inputStream(code);
    std::vector<std::shared_ptr<OptimizerCommand>> program;
    std::string line;
    while (std::getline(inputStream, line)) program.push_back(std::make_shared<OptimizerCommand>(line));

    // remove all IS (not AS) that follow an assignment that is not AT or GET
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<3 || command->args[0]!="IS")
            continue;
        std::string to_replace = command->args[1];
        std::string symbol = command->args[2];
        int declaration = i-1; // for the time being this issue can arise only by adding an IS after an immediate command
        if(declaration>=0 && program[declaration]->args.size()>1 && program[declaration]->args[0]!="IS" 
            && program[declaration]->args[1] == symbol && program[declaration]->args[0] != "at" && program[declaration]->args[0] != "get"
            && program[declaration]->args[0] != "call" // calling on a method on the objet's self creates an error for the pattern `point=point.copy()`
            ) {
            program[declaration]->args[1] = to_replace;
            program[i]->enabled = false;
        }
    }

    // remove outputs from some expressions
    for(int i=0;i<program.size();i++) {
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
            if(command->args[0]=="set") ++j;
            for (; j < command->args.size(); ++j) {
                const std::string& symbol = command->args[j];
                if (symbol == "LAST") bberror("Internal error: the LAST keyword has been deprecated");
                if (symbol != "#")  symbolUsageCount[symbol]++;
            }
        }
        changes = 0;
        for (int i=0;i<program.size();++i) {
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
                ))
                continue;
            
            if(command->args.size() && command->args[0]=="exists" && symbolUsageCount[command->args[1]]==0) DISABLE;
            if(command->args.size()<=1) continue;
            if(command->args[0]=="final" && command->args.size()>=3 && symbolUsageCount[command->args[2]]==0) DISABLE;
            if(command->args[0]=="set" && command->args.size()>=4 && (symbolUsageCount[command->args[2]]==0 || symbolUsageCount[command->args[3]]==0)) DISABLE;
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
    std::string result;
    int n = program.size();
    std::string towrite;
    for (int i = 0; i < n; i++) {
        if (program[i]->enabled && !towrite.empty()) {
            result += towrite + "\n";
            towrite.clear();
        }
        if (!program[i]->info.empty()) towrite = program[i]->info;
        else result += program[i]->toString();
    }


    return std::move(result);
}


void optimize(const std::string& source, const std::string& destination, bool minimify) {
    std::ifstream inputFile(source);
    bbassert(inputFile.is_open(), "Unable to open file: " + source);
    std::string code = "";
    std::string line;
    while (std::getline(inputFile, line)) code += line + "\n";
    inputFile.close();

    std::string optimized = optimizeFromCode(code, minimify);
    //optimized = cacheDuplicates(optimized);

    std::ofstream outputFile(destination);
    bbassert(outputFile.is_open(), "Unable to write to file: " + destination);
    outputFile << optimized;
    outputFile.close();


    /*
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  
        bberror("Unable to open file: " + source);
    std::vector<std::shared_ptr<OptimizerCommand>> program;
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(std::make_shared<OptimizerCommand>(line));
    inputFile.close();

    // save the compiled code to the de tination file
    std::ofstream outputFile(destination);
    bbassert (outputFile.is_open(), "Unable to write to file: " + source);
    int n = program.size();
    std::string towrite;
    for(int i=0;i<n;i++) {
        if(program[i]->enabled && towrite.size()) {
            outputFile << towrite << "\n";
            towrite.clear();
        }
        if(program[i]->info.size())
            towrite = program[i]->info;
        else
            outputFile << program[i]->toString();
    }
    outputFile.close(); */
}







#endif