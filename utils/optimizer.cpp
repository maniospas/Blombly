#include <string>
#include <iostream>
#include "stringtrim.cpp"
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
            if(command[pos]=='"')
                inString = !inString;
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



int optimize(const std::string& source, const std::string& destination) {
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  {
        std::cerr << "Unable to open file: " << source << std::endl;
        return 1;
    }
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

    // remove all IS statements that follow an assignment
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args.size()<3 || command->args[0]!="IS")
            continue;
        std::string to_replace = command->args[1];
        std::string symbol = command->args[2];
        int declaration = i-1; // for the time being this issue can arise only by adding an IS after an immediate command
        if(declaration>0 && program[declaration]->args.size()>1 && program[declaration]->args[0]!="IS"
            && program[declaration]->args[1] == symbol) {
            program[declaration]->args[1] = to_replace;
            program[i]->enabled = false;
        }
        /*else if(i<program.size()-1)
            for(int k=2;k<program[i+1]->args.size();k++)
                if(to_replace==program[i+1]->args[k]){
                    to_replace=program[i+1]->args[k] = symbol;
                    program[i]->enabled = false;
                }*/
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
    int cacheNum = 0;
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args[1]=="#" 
        && command->args[0]!="if" 
        && command->args[0]!="while" 
        && command->args[0]!="print"
        && command->args[0]!="set"
        && command->args[0]!="final"
        && command->args[0]!="push"
        && command->args[0]!="put") {
            command->args[1] = "_bbresult"+std::to_string(cacheNum);
            cacheNum += 1;
        }
    }

    // remove put and push assignments
    for(int i=0;i<program.size();i++) {
        std::shared_ptr<OptimizerCommand> command = program[i];
        if(command->args[0]=="put" || command->args[0]=="push" || command->args[0]=="set"|| command->args[0]=="final") 
            command->args[1] = "#";
    }

    // add local symbol cache (just check that every symbol is not final)
    // TODO

    // save the compiled code to the destination file
    std::ofstream outputFile(destination);
    if (!outputFile.is_open())  {
        std::cerr << "Unable to write to file: " << source << std::endl;
        return 1;
    }
    for(int i=0;i<program.size();i++) 
        outputFile << program[i]->toString();
    outputFile.close();

    // return success code if no errors have occured
    return 0;    
}