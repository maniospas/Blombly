/**
 * This file implements the blombly language compiler and virtual machine
 * to run the compiled program.
 * Produce an executable and run one of the following two file formats:
 *  * blombly file.bb
 *  * blombly file.bbvm
 * The .bb format the progamming language code, which is converted to the
 * .bbvm format, which is then run. The .bbvm format is the assembly language
 * of the blombly virtual machine.
 * 
 * Author: Emmanouil (Manios) Krasanakis
 * Email: maniospas@hotmail.com
 * License: Apache 2.0
*/


#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <sstream>
#include <memory> 
#include <cstdlib>
#include <algorithm>
#include <unordered_set>  



// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

class Unimplemented {
    /**
     * An object of this type is thrown as an exception when trying to
     * implement overloaded Data methods.
    */
public:
    Unimplemented() {
    }
};


class Data {
    /**
     * This abstract class represents data stored in the memory.
    */
public:
    virtual std::string toString() const = 0;
    virtual std::string getType() const = 0;
    virtual ~Data() = default;
    static std::shared_ptr<Data> run(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        std::shared_ptr<Data> ret;
        for(const std::shared_ptr<Data>& implementer : all)
            try {
                ret = implementer->implement(operation, all);
                break;
            } 
            catch(Unimplemented) { // TODO: catch only Unimplemented exceptions
            }
        if(ret==nullptr)
            std::cerr << "No valid implementation of "+operation+" for any of its arguments" << std::endl;
        return ret;
    }
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        throw Unimplemented();
    }
};


class Integer : public Data {
private:
    int value;
public:
    Integer(int val) : value(val) {}
    std::string getType() const override {return "int";}
    std::string toString() const override {return std::to_string(value);}
    int getValue() const {return value;}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(operation=="add" && all.size()==2 && all[0]->getType()=="int" && all[1]->getType()=="int") 
            return std::make_shared<Integer>(
                std::static_pointer_cast<Integer>(all[0])->getValue()+
                std::static_pointer_cast<Integer>(all[1])->getValue()
            );
        throw Unimplemented();
    }
};

class Float : public Data {
private:
    float value;
public:
    Float(float val) : value(val) {}
    std::string getType() const override {return "float";}
    std::string toString() const override {return std::to_string(value);}
    float getValue() const {return value;}
    virtual std::shared_ptr<Data> implement(const std::string& operation, std::vector<std::shared_ptr<Data>>& all) {
        if(operation=="add" && all.size()==2 && all[0]->getType()=="float" && all[1]->getType()=="float") 
            return std::make_shared<Integer>(
                std::static_pointer_cast<Float>(all[0])->getValue()+
                std::static_pointer_cast<Float>(all[1])->getValue()
            );
        if(operation=="add" && all.size()==2 && all[0]->getType()=="int" && all[1]->getType()=="float") 
            return std::make_shared<Integer>(
                std::static_pointer_cast<Integer>(all[0])->getValue()+
                std::static_pointer_cast<Float>(all[1])->getValue()
            );
        if(operation=="add" && all.size()==2 && all[0]->getType()=="float" && all[1]->getType()=="int") 
            return std::make_shared<Integer>(
                std::static_pointer_cast<Float>(all[0])->getValue()+
                std::static_pointer_cast<Integer>(all[1])->getValue()
            );
        throw Unimplemented();
    }
};


class BString : public Data {
private:
    std::string value;
public:
    BString(const std::string& val) : value(val) {}
    std::string getType() const override {return "string";}
    std::string toString() const override {return value;}
};


class Code : public Data {
private:
    int start, end;
public:
    Code(int startAt, int endAt) : start(startAt), end(endAt) {}
    std::string getType() const override {return "code";}
    std::string toString() const override {return "code from "+std::to_string(start)+" to "+std::to_string(end);}
    int getStart() const {return start;}
    int getEnd() const {return end;}
};


std::vector<std::string> split(std::string s, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;
    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        if(token.size()>0)
            res.push_back (token);
    }
    res.push_back(s.substr (pos_start));
    return res;
}


class Command {
public:
    std::vector<std::string> args;
    Command(std::string command) {
        args = split(command, " ");
    }
};


class Memory {
private:
    std::shared_ptr<Memory> parent;
    std::unordered_map<std::string, std::shared_ptr<Data>> data;
public:
    Memory() {parent=nullptr;}
    Memory(std::shared_ptr<Memory> par): parent(par) {}
    std::shared_ptr<Data> get(std::string item) {
        if(item=="#")
            return nullptr;
        std::shared_ptr<Data> ret = data[item];
        if(!ret && parent)
            ret = parent->get(item);
        if(!ret)
            std::cerr << "Missing value: " + item << std::endl;
        return ret;
    }
    void set(std::string item, std::shared_ptr<Data> value) {
        if(item=="#") 
            return;
        data[item] = value;
    }
};


std::shared_ptr<Data> executeBlock(const std::vector<std::shared_ptr<Command>>& program,
                  int start, 
                  int end,
                  const std::shared_ptr<Memory>& memory) {
    /**
     * Executes a block of code from within a list of commands.
     * @param program The list of command pointers.
     * @param start The position from which to start the interpreting.
     * @param end The position at which to stop the interpreting (inclusive).
     * @param memory A pointer to the read and write memory.
     * @return A Data shared pointer holding the code block's outcome. This is nullptr if nothing is returned.
    */
    for(int i=start;i<=end;i++) {
        auto command = program[i]->args;
        std::shared_ptr<Data> value;
        if(command[0]=="CONST") {
            if(command[2][0]=='"')
                value = std::make_shared<BString>(command[2].substr(2, command[2].size()-1));
            else if(command[2][0]=='I')
                value = std::make_shared<Integer>(std::atoi(command[2].substr(1).c_str()));
            else if(command[2][0]=='F')
                value = std::make_shared<Float>(std::atof(command[2].substr(1).c_str()));
            else
                std::cerr << "Unable to understand constant: " << command[2] << std::endl;
        }
        else if(command[0]=="print") 
            std::cout << memory->get(command[2])->toString() << std::endl;
        else if(command[0]=="BEGIN") {
            int pos = i+1;
            int depth = 0;
            std::string command_type;
            while(pos<=end) {
                command_type = program[pos]->args[0];
                if(command_type=="BEGIN")
                    depth += 1;
                if(command_type=="END") {
                    if(depth==0)
                        break;
                    depth -= 1;
                }
                pos += 1;
            }
            if(depth>0)
                std::cerr << "Unclosed code block" << std::endl;
            value = std::make_shared<Code>(i+1, pos);
            i = pos;
        }
        else if(command[0]=="END") {
            break;
        }
        else if(command[0]=="CALL") {
            // create new writable memory under the current context
            std::shared_ptr<Memory> newMemory = std::make_shared<Memory>(memory);
            std::shared_ptr<Data> context = memory->get(command[2]);
            std::shared_ptr<Data> execute = memory->get(command[3]);
            // check if the call has some context, and if so, execute it in the new memory
            if(context && context->getType()=="code") {
                std::shared_ptr<Code> code = std::static_pointer_cast<Code>(context);
                value = executeBlock(program, code->getStart(), code->getEnd(), newMemory);
                if(value) // show an erroe message if the context returned with anything other than END
                    std::cerr << "Code execution context should not return a value." << std::endl;
            }
            // execute the called code in the new memory
            std::shared_ptr<Code> code = std::static_pointer_cast<Code>(execute);
            value = executeBlock(program, code->getStart(), code->getEnd(), newMemory);
        }
        else if(command[0]=="return") {
            return memory->get(command[2]);
        }
        else  {
            std::vector<std::shared_ptr<Data>> args;
            for(int i=2;i<command.size();i++)
                args.push_back(memory->get(command[i]));
            value = Data::run(command[0], args);
        }
        memory->set(command[1], value);
    }
    return nullptr;
}


int vm(const std::string& fileName) {
    /**
     * Reads commands from a compiled blombly assembly file (.bbvm) 
     * and runs them in the virtual machine.
     * @param fileName The path to the file.
     * @return 0 if execution completed successfully
    */

    // open the blombly assembly file
    std::ifstream inputFile(fileName);
    std::vector<std::shared_ptr<Command>> program;
    if (!inputFile.is_open())  {
        std::cerr << "Unable to open file: " << fileName << std::endl;
        return 1;
    }

    // organizes each line to a new assembly command
    std::string line;
    while (std::getline(inputFile, line)) 
        program.push_back(std::make_shared<Command>(line));
    inputFile.close();

    // initialize memory and execute the assembly commands
    std::shared_ptr<Memory> memory = std::make_shared<Memory>();
    executeBlock(program, 0, program.size()-1, memory);
    return 0;
}


class Parser {
private:
    std::string compiled;
    std::unordered_set<std::string> symbols;
    int topTemp;
    std::string getAssignee(std::string lhs) {
        /**
         * Get the symbol on which the expression assigns a value.
        */
        std::string accumulate;
        int pos = 0;
        int depth = 0;
        while(pos<lhs.size()) {
            if(lhs[pos]=='('||lhs[pos]=='{')
                break;
            if(lhs[pos]=='=') {
                trim(accumulate);
                return accumulate;
            }
            accumulate += lhs[pos];
            pos += 1;
        }
        return "#";
    }
    std::string getValue(std::string rhs) {
        /**
         * Get the string expression (this is guaranteed not to contain an assignee).
        */
        int pos = 0;
        while(pos<rhs.size()) {
            if(rhs[pos]=='('||rhs[pos]=='{')
                break;
            if(rhs[pos]=='='){
                rhs = rhs.substr(pos+1);
                break;
            }
            pos += 1;
        }
        trim(rhs);
        return rhs;
    }
    bool isString(const std::string& value) {
        return value[0]=='"' && value[value.size()-1]=='"';
    }
    bool isInt(const std::string& value) {
        try{
            std::atoi(value.c_str());
            return true;
        }
        catch(...) {
            return false;
        }
    }
    bool isFloat(const std::string& value) {
        try{
            std::atof(value.c_str());
            return true;
        }
        catch(...) {
            return false;
        }
    }
    void addCommand(std::string& command) {
        /**
         * Parses an indivual command found in a block of code. Called by parse.
        */
        if(command.size()==0)
            return;
        std::string variable = getAssignee(command);
        std::string value = getValue(command);
        size_t pos = value.find('(');
        if(pos == std::string::npos) {
            if(variable=="#")
                return;
            if(isString(value))
                compiled += "CONST "+variable+" "+value+"\n";
            else if(isInt(value))
                compiled += "CONST "+variable+" I"+value+"\n";
            else if(isFloat(value))
                compiled += "CONST "+variable+" F"+value+"\n";
            else
                compiled += value+" "+variable+"\n";
        } else {
            std::string args = value.substr(pos+1); // leaving the right parenthesis to be removed during further computations
            value = value.substr(0, pos);
            trim(value);
            if(symbols.find(value) != symbols.end()) {
                std::string argexpr = args.substr(0, args.size()-1);
                if(argexpr=="")
                    argexpr = "#";
                compiled += "CALL "+variable+" "+argexpr+" "+value+"\n";
            }
            else {
                std::string argexpr;
                int depth = 0;
                int i = 0;
                std::string accumulate;
                while(i<args.size()) {
                    if(depth==0 && (args[i]==',' || i==args.size()-1)) {
                        trim(accumulate);
                        if(symbols.find(accumulate) == symbols.end()) {
                            std::string tmp = "_anon"+std::to_string(topTemp);
                            topTemp += 1;
                            Parser tmpParser = Parser(symbols, topTemp);
                            tmpParser.parse(tmp+" = "+accumulate+";");
                            compiled += tmpParser.toString();
                            accumulate = tmp;
                            //topTemp = tmpParser.topTemp;
                        }
                        argexpr += " "+accumulate;
                        accumulate = "";
                        i += 1;
                        continue;
                    }
                    if(args[i]=='(' || args[i]=='{')
                        depth += 1;
                    if(args[i]==')' || args[i]=='}')
                        depth -= 1;
                    accumulate += args[i];
                    i += 1;
                }
                compiled += value+" "+variable+argexpr+"\n";
            }
        }
        if(variable!="#")
            symbols.insert(variable);
    }
public:
    Parser() {
        topTemp = 0;
    }
    Parser(std::unordered_set<std::string>& symbs, int topTemps) {
        symbols = symbs;
        topTemp = topTemps;
    }
    void parse(const std::string& code) {
        /**
         * Parses a block of code.
        */
        std::string command;
        int pos = 0;
        int depth = 0;
        while(pos<code.size()) {
            char c = code[pos];
            if(c=='\n')
                c = ' ';
            if(c=='\t')
                c = ' ';
            if(c=='(') 
                depth += 1;
            if(c==')')
                depth -= 1;
            if(depth==0 && c=='{') {
                std::string variable = getAssignee(command);
                compiled += "BEGIN "+variable+"\n";
                if(variable!="#")
                    symbols.insert(variable);
                command = "";
            }
            else if(depth==0 && c=='}') {
                addCommand(command);
                compiled += "END\n";
                command = "";
            }
            else if(depth==0 && (c==';' || pos==code.size()-1)) {
                addCommand(command);
                command = "";
            }
            else 
                command += c;
            pos += 1;
        }
    }
    std::string toString() const {
        /**
         * Returns the end-result of parsing.
        */
        return compiled;
    }
};


int compile(const std::string& source, const std::string& destination) {
    /**
     * Compiles a blombly file (.bb) written in the namesake programming
     * language to a corresponding virtual machine file (.bbvm).
     * @param source The blombly file path that contains the source code.
     * @param destination The file path on which to write the compiled virtual machine assembly.
     * @return 0 if compilation was completed successfully
    */
    
    // load the source code from the source file
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  {
        std::cerr << "Unable to open file: " << source << std::endl;
        return 1;
    }
    std::string code = "";
    std::string line;
    while (std::getline(inputFile, line)) 
        code += line;
    inputFile.close();
        
    // create a compiled version of the code
    Parser parser;
    parser.parse(code);
    std::string compiled = parser.toString();

    // save the compiled code to the destination file
    std::ofstream outputFile(destination);
    if (!outputFile.is_open())  {
        std::cerr << "Unable to write to file: " << source << std::endl;
        return 1;
    }
    outputFile << compiled;
    outputFile.close();

    // return success code if no errors have occured
    return 0;    
}


int main(int argc, char* argv[]) {
    // parse file to run
    std::string fileName = "main.bb";
    if (argc > 1) 
        fileName = argv[1];

    // if the file has a blombly source code format (.bb) compile 
    // it into an assembly file (.bbvm)
    if(fileName.substr(fileName.size()-3, fileName.size())==".bb") {
        if(compile(fileName, fileName+"vm"))
            return false;
        fileName = fileName+"vm";
    }

    // run the assembly file in the virtual machine
    return vm(fileName);
}