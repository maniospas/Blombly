#ifndef OPTIMIZER_CPP
#define OPTIMIZER_CPP

#include <string>
#include <iostream>
#include "stringtrim.cpp"
#include "common.h"
#include <unordered_map> 


class OptimizerCommand {
public:
    std::vector<std::string> args;
    bool enabled;
    OptimizerCommand(std::string command) {
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
        if(args.size()==0 || !enabled)
            return "";
        std::string ret = args[0];
        for(int i=1;i<args.size();i++)
            ret = ret + " "+args[i];
        return ret+"\n";
    }
};


void moveRangeToFront(std::vector<std::shared_ptr<OptimizerCommand>>& program, size_t i, size_t j, size_t front) {
    if (i > j || j >= program.size() || front >= program.size()) {
        throw std::out_of_range("Invalid indices");
    }
    std::vector<std::shared_ptr<OptimizerCommand>> temp(program.begin() + i, program.begin() + j + 1);
    program.erase(program.begin() + i, program.begin() + j + 1);
    program.insert(program.begin()+front, temp.begin(), temp.end());
}



void optimize(const std::string& source, const std::string& destination) {
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  
        bberror("Unable to open file: " + source);
    std::vector<std::shared_ptr<OptimizerCommand>> program;
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(std::make_shared<OptimizerCommand>(line));
    inputFile.close();
    
    /*
    // hash all build statements
    std::unordered_map<std::string, std::string> buildins;
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<3 || command->args[0]!="BUILTIN")
            continue;
        if(buildins.find(command->args[2])==buildins.end())
            buildins[command->args[2]] = "_bbb"+std::to_string(buildins.size());
    }

    // find all buildins and replace them in their block
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<3 || command->args[0]!="BUILTIN")
            continue;
        int depth = 0;
        command->enabled = false;
        for(int j=i+1;j<program.size();j++) {
            if(program[j]->args.size()==0 || !program[j]->enabled)
                continue;
            // stop if code block ends or something is assigned
            if(program[j]->args[0]=="BEGIN" || command->args[1]=="BEGINFINAL") 
                depth += 1;
            if(program[j]->args[0]=="END") {
                depth -= 1;
                if(depth<0)
                    break;
            }
            // replace arg with the final buildin
            for(int k=2;k<program[j]->args.size();k++)
                if(program[j]->args[k]==command->args[1])
                    program[j]->args[k] = buildins[command->args[2]];
            if(program[j]->args[1]==command->args[1]) 
                break;
        }
    }

    // add the final buildin statements at the beginning
    for(auto const& entry : buildins) {
        program.insert(program.begin(), std::make_shared<OptimizerCommand>("final # "+entry.second));
        program.insert(program.begin(), std::make_shared<OptimizerCommand>("BUILTIN "+entry.second+" "+entry.first));
    }*/

    // remove all IS (not AS) that follow an assignment that is not AT or GET
    // TODO: add assembly commands for atAS and getAS (or some other indicators for AS assignments)
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

    
    // find symbols
    std::unordered_map<std::string, int> symbols; 
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        for(int j=1;j<command->args.size();j++)
            if(command->args[j]!="#" && command->args[j]!="LAST" && (command->args[0]!="BUILTIN" || j==1)
                 && symbols.find(command->args[j])==symbols.end())
                symbols[command->args[j]] = symbols.size();
    }

    // anonymize
    /*for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        for(int j=1;j<command->args.size();j++)
            if(command->args[j]!="#" && command->args[j]!="LAST" && (command->args[0]!="BUILTIN" || j==1))
                command->args[j] = std::to_string(symbols[command->args[j]]);
    }*/

    
    // add outcome caches (these prevent re-initialization of shared pointers that have been previously computed)
    /*int cacheNum = 0;
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<2)
            continue;
        if(command->args[1]=="#" 
        && command->args[0]!="if" 
        && command->args[0]!="while" 
        && command->args[0]!="print"
        //&& (command->args[0]!="return" || command->args.size()>2)
        && command->args[0]!="set"
        && command->args[0]!="setfinal"
        && command->args[0]!="final"
        && command->args[0]!="push"
        && command->args[0]!="put") {
            command->args[1] = "_bbresult"+std::to_string(cacheNum);
            cacheNum += 1;
        }
    }*/

    // remove put and push assignments
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<2)
            continue;
        if(command->args[0]=="put" || command->args[0]=="push" || command->args[0]=="setfinal" || command->args[0]=="set" || command->args[0]=="final") 
            command->args[1] = "#";
    }

    // add local symbol cache (just check that every symbol is not final)
    // TODO

    // optimize local code blocks
    /*for(int i=0;i<program.size();++i) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()==0 || command->args[0]!="BEGIN" || !command->enabled)
            continue;
        if(command->args.size()<2 || command->args[1].substr(0, 3)!="_bb")
            continue;
        command->args[0] = "BEGINFINAL";
    }*/

    // flatten nested code blocks (move begin-end declarations to the beginning of the program)
    /*int front = 0;
    for(int i=0;i<program.size();++i) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if((command->args[0]!="BEGIN" && command->args[0]!="BEGINFINAL") || !command->enabled)
            continue;
        if(command->args.size()<2 || command->args[1].substr(0, 3)!="_bb")
            continue;
        int depth = 0;
        int block_end = i;
        bool has_finals = false;
        int j;
        for(j=i+1;j<program.size();++j) {
            if(program[j]->args.size()==0 || !program[j]->enabled)
                continue;
            // stop if code block ends or something is assigned
            if(program[j]->args[0]=="FINAL" || command->args[1]=="BEGINFINAL") 
                has_finals = true;
            if(program[j]->args[0]=="BEGIN" || command->args[1]=="BEGINFINAL") {
                depth += 1;
                if(!has_finals)
                    break;
            }
            if(program[j]->args[0]=="END") {
                depth -= 1;
                if(depth<0) {
                    block_end = j;
                    break;
                }
            }
        }
        if(has_finals) {
            i = j;
        }
        else if(block_end!=i) {
            moveRangeToFront(program, i, block_end, front);
        }
    }

    // move all builtins at the beginning
    /*for(int i=1;i<program.size();++i) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args[0]!="BUILTIN" || !command->enabled || command->args.size()<2)
            continue;
        if(command->args[1].substr(0, 3)=="_bb") {
            moveRangeToFront(program, i, i);
            --i;
        }
    }*/
    
    // remove unused methods
    int changes = -1;
    while(changes!=0) {
        std::unordered_map<std::string, int> symbolUsageCount;
        for (const auto& command : program) {
            if(!command->enabled || command->args.size()==0)
                continue;
            if(command->args[0]=="END" || command->args[0]=="BEGIN" || command->args[0]=="BEGINFINAL" || command->args[0]=="final")  // TODO: why was there an "exists" at some point here?
                continue;
            for (size_t j = 2; j < command->args.size(); ++j) {
                const std::string& symbol = command->args[j];
                if (symbol == "LAST")
                    bberror("Internal error: the LAST keyword has been deprecated");
                if (symbol != "#") 
                    symbolUsageCount[symbol]++;

            }
        }
        changes = 0;
        for (int i=0;i<program.size();++i) {
            auto& command = program[i];
            if(!command->enabled)
                continue;

            if(i<program.size()-1 && program[i+1]->args[0]=="END")
                continue;

            //if(command->args.size()>=2 && command->args[1].size() && command->args[1][0]=='\\')  // operators are still valid
            //    continue;
            if(command->args.size()>=2 && (command->args[1]=="put"
                || command->args[1]=="at" || command->args[1]=="call" 
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
                ))
                continue;
            
            if(command->args.size() && command->args[0]=="exists" && symbolUsageCount[command->args[1]]==0) {
                command->enabled = false;
                ++changes;
                continue;
            }
            if(command->args.size()<=1)
                continue;
            if(command->args[0]=="final" && command->args.size()>=3 && symbolUsageCount[command->args[2]]==0) {
                command->enabled = false;
                ++changes;
                continue;
            }
            if(command->args[0]=="BUILTIN" && command->args.size() && symbolUsageCount[command->args[1]]==0) {
                command->enabled = false;
                ++changes;
                continue;
            }
            if((command->args[0]=="IS" || command->args[0]=="AS") && command->args.size() && symbolUsageCount[command->args[1]]==0) {
                command->enabled = false;
                ++changes;
                continue;
            }
            if(command->args[0]!="BEGIN" && command->args[0]!="BEGINFINAL")
                continue;
            if(symbolUsageCount[command->args[1]]!=0)
                continue;
            // std::cout << "removing "<<command->args[0]<<" "<<command->args[1]<<" "<<command->enabled<<"\n";
            i = i+1;
            int depth = 1;
            ++changes;
            command->enabled = false;
            while(i<program.size()) {
                program[i]->enabled = false;
                if(program[i]->args[0]=="BEGIN" || program[i]->args[0]=="BEGINFINAL")
                    depth += 1;
                if(program[i]->args[0]=="END")
                    depth -= 1;
                if(depth==0)
                    break;
                ++i;
            }
        }

    }


    // save the compiled code to the de tination file
    std::ofstream outputFile(destination);
    if (!outputFile.is_open())  
        bberror("Unable to write to file: " + source);
    for(int i=0;i<program.size();i++) 
        outputFile << program[i]->toString();
    outputFile.close(); 
}

#endif