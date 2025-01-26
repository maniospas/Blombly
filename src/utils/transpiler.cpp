#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "utils.h"
#include "common.h"
#include <unordered_map> 


class TranspilerCommand {
public:
    std::vector<std::string> args;
    TranspilerCommand(std::string command) {
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
        if(args.size()==0)
            return "";
        std::string ret = args[0];
        for(int i=1;i<args.size();i++)
            ret = ret + " "+args[i];
        return ret+"\n";
    }
};



std::string transpile(const std::vector<std::shared_ptr<TranspilerCommand>>& program, 
				  int from, 
				  int to, 
				  std::string& method_declarations, 
				  const std::unordered_map<std::string, std::string>& inherited_types,
				  const std::string& stack,  // a unique stack name to prepend to method declarations
				  const std::string& tab
				  ) {
    std::string transpilation;
    std::unordered_map<std::string, std::string> inferred_type(inherited_types);
    
    for(int i=from;i<=to;++i) {
    		auto command = program[i];
          if(command->args.size()==0)
          	continue;
    		std::string type = command->args[0];
    		std::string inferred_result_type;
    		if(type=="BUILTIN" && command->args.size()==3) {
    			if(command->args[2][0]=='I') {
    				inferred_result_type = "int";
    				transpilation += tab+"int "+command->args[1]+" = "+command->args[2].substr(1)+";\n";
    			}
    			else if(command->args[2][0]=='F') {
    				inferred_result_type = "float";
    				inferred_result_type = "int";
    				transpilation += tab+"float "+command->args[1]+" = "+command->args[2].substr(1)+";\n";
    			}
    			else if(command->args[2][0]=='"') {
    				inferred_result_type = "char*";
    				transpilation += tab+"char* "+command->args[1]+" = "+command->args[2]+";\n";
    			}
    			else
    				bberror("Unknown builtin type"+command->args[2]);
    		}
    		else if(type=="add" && command->args.size()==4) {
    			if(inferred_type[command->args[2]]=="char*" && inferred_type[command->args[3]]=="char*") {
    				inferred_result_type = "char*";
	    			transpilation += tab+inferred_result_type+" "+command->args[1]+" = strcat("+command->args[2]+", "+command->args[3]+");\n";
    			}
    			else {
	    			if(inferred_type[command->args[2]]=="int" && inferred_type[command->args[3]]=="int") 
	    				inferred_result_type = "int";
	    			else if(inferred_type[command->args[2]]=="int" && inferred_type[command->args[3]]=="float") 
	    				inferred_result_type = "float";
	    			else if(inferred_type[command->args[2]]=="float" && inferred_type[command->args[3]]=="int") 
	    				inferred_result_type = "float";
	    			else
    					bberror("Two incompatible types are being added");
	    			transpilation += tab+inferred_result_type+" "+command->args[1]+" = "+command->args[2]+" + "+command->args[3]+";\n";
    			}
    		}
    		else if(type=="lt" && command->args.size()==4) {
    			if(inferred_type[command->args[2]]=="int" && inferred_type[command->args[3]]=="int") 
    				inferred_result_type = "bool";
    			else if(inferred_type[command->args[2]]=="int" && inferred_type[command->args[3]]=="float") 
    				inferred_result_type = "bool";
    			else if(inferred_type[command->args[2]]=="float" && inferred_type[command->args[3]]=="int") 
    				inferred_result_type = "bool";
    			else
				bberror("Two incompatible types are being compared with <");
    			transpilation += tab+inferred_result_type+" "+command->args[1]+" = "+command->args[2]+" < "+command->args[3]+";\n";
    		}
    		else if(type=="print" && command->args.size()==3) {
    			if(inferred_type[command->args[2]]=="char*")
    				transpilation += tab+"printf(\"%s\\n\", "+command->args[2]+");\n";
    			else if(inferred_type[command->args[2]]=="int")
    				transpilation += tab+"printf(\"%d\\n\", "+command->args[2]+");\n";
    			else if(inferred_type[command->args[2]]=="float")
    				transpilation += tab+"printf(\"%f\\n\", "+command->args[2]+");\n";
    			else
    				bberror("An unknown type is being printed");
    		}
		else 
   			bberror("Transpilation not implemented for command: " + command->args[0]+" with "+std::to_string(command->args.size()-1)+" arguments");
    		if(command->args[1]!="#")
        		inferred_type[command->args[1]] = inferred_result_type;
    }
    
    return (transpilation);
}


void transpile(const std::string& source, const std::string& destination) {
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  
        bberror("Unable to open file: " + source);

    std::vector<std::shared_ptr<TranspilerCommand>> program;
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(std::make_shared<TranspilerCommand>(line));
    inputFile.close();

    std::string method_declarations;
    std::unordered_map<std::string, std::string> inferred_type;
    std::string transpilation = transpile(program, 0, program.size(), method_declarations, inferred_type, "main", "  ");
    transpilation = "#include <stdio.h>\n\n"+method_declarations+"int main(){\n"+transpilation+"return 0;\n}";
    

    std::ofstream outputFile(destination);
    if (!outputFile.is_open())  
        bberror("Unable to write to file: " + source);
    outputFile << transpilation;
    outputFile.close(); 

}
