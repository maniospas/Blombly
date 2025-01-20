#include <iostream>
#include <cstdlib>
#include <memory>

#include "interpreter/Command.h"
#include "data/Integer.h"
#include "data/BFloat.h"
#include "data/BString.h"
#include "BMemory.h"
#include "common.h"
#include <string>

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Advance past the last replaced portion
    }
}

// SourceFile constructor
SourceFile::SourceFile(const std::string& path) : path(path) {}

// CommandContext constructor
CommandContext::CommandContext(const std::string& source) : source(source) {}

// Command constructor
Command::Command(const std::string& command, SourceFile* source_, int line_, CommandContext* descriptor_) 
    : source((source_)), line(line_), descriptor((descriptor_)), value(nullptr) {

    std::vector<std::string> argNames;
    argNames.reserve(4);
    std::string accumulate;
    int pos = 0;
    bool inString = false;
    while (pos < command.size()) {
        // strings can only be the last arguments of builtin types
        if(command[pos]=='"' && !inString)
            inString = !inString;
        if(inString && pos==command.size()-1) {
            accumulate += command[pos];
            argNames.push_back(accumulate);
            break;
        }
        if (!inString && (command[pos] == ' ' || pos == command.size() - 1)) {
            if (command[pos] != ' ') {
                accumulate += command[pos];
            }
            argNames.push_back(accumulate);
            accumulate = "";
        } else {
            accumulate += command[pos];
        }
        pos += 1;
    }

    operation = getOperationType(argNames[0]);
    nargs = argNames.size() - 1;

    if (operation == BUILTIN) {
        nargs -= 1;
        std::string raw = argNames[2];
        if (raw[0] == '"') {
            raw = raw.substr(1, raw.size() - 2);
            replaceAll(raw, "\\n", "\n");
            //replaceAll(raw, "\\t", "\t");
            //replaceAll(raw, "\\'", "'");
            //replaceAll(raw, "\\\"", "\"");
            value = new BString(raw);
        }
        else if (raw[0] == 'I') value = DataPtr((int64_t)std::atoi(raw.substr(1).c_str()));//new Integer(std::atoi(raw.substr(1).c_str()));}
        else if (raw[0] == 'F') value = DataPtr((double)std::atof(raw.substr(1).c_str()));//new BFloat(std::atof(raw.substr(1).c_str()));
        else if (raw[0] == 'B') value = DataPtr((raw == "Btrue")?true:false);//(raw == "Btrue")?Boolean::valueTrue:Boolean::valueFalse;
        else bberror("Unable to understand builtin value prefix (should be one of I,F,B,\"): " + raw);
        if(value.exists()) value->addOwner();
    }

    // Initialize args and knownLocal vectors
    args.reserve(nargs);
    //knownLocal.reserve(nargs);
    for (int i = 0; i < nargs; ++i) {
        //knownLocal.push_back(argNames[i + 1].size()>=3 && argNames[i + 1].substr(0, 3) == "_bb" && (argNames[i + 1].size()<=8 || argNames[i + 1].substr(0, 8) != "_bbmacro"));
        args.push_back(variableManager.getId(argNames[i + 1]));
    }

    bbassert(args.size() == 0 || args[0] != variableManager.thisId || argNames[0] == "set" || argNames[0] == "setfinal" || argNames[0] == "get" || argNames[0] == "return",
        "Cannot assign to `this`."
        "\n    Encountered for operation: " + argNames[0] +
        "\n    This error appears only when you use `this` as a method argument or for invalid hand-written .bbvm files.");
}

std::string Command::toString() const {
    std::string ret = getOperationTypeName(operation);
    for (auto arg : args) ret += " " + variableManager.getSymbol(arg);
    return ret;
}

std::string Command::tocpp(bool first_assignment) const {
    /*if(operation==FINAL) return "";
    if(operation==RETURN) return "this is a return value";
    if(operation==PUSH) return (first_assignment?"auto ":"")+variableManager.getSymbol(args[1])+".push_back("+variableManager.getSymbol(args[2])+");";
    if(operation==NEXT) {
        std::string ret = (first_assignment?"auto ":"")+variableManager.getSymbol(args[0])+"="+variableManager.getSymbol(args[1])+".front("+variableManager.getSymbol(args[2])+");";
        ret += variableManager.getSymbol(args[1])+".pop_front();";
        return ret;
    }
    if(operation==POP) {
        std::string ret = (first_assignment?"auto ":"")+variableManager.getSymbol(args[0])+"="+variableManager.getSymbol(args[1])+".back("+variableManager.getSymbol(args[2])+");";
        ret += variableManager.getSymbol(args[1])+".pop_back();";
        return ret;
    }
    if(operation==BB_PRINT) {
        std::string ret = "std::cout";
        for (int i = 1; i < args.size(); ++i) 
            ret += "<<"+variableManager.getSymbol(args[i]);
        ret += "<<std::endl;";
        return ret;
    }
    if(operation==TOLIST) {
        std::string ret = (first_assignment?"std::list ":"")+variableManager.getSymbol(args[0])+" = {";
        for (int i = 1; i < args.size(); ++i) {
            if(i!=1)
                ret+=",";
            ret += variableManager.getSymbol(args[i]);
        }
        ret += "};";
        return ret;
    }
    if(operation==BUILTIN) {
        if(value->getType()==BB_INT) return (first_assignment?"int ":"")+variableManager.getSymbol(args[0])+" = "+value->toString(nullptr)+";";
        if(value->getType()==BB_FLOAT) return (first_assignment?"double ":"")+variableManager.getSymbol(args[0])+" = "+value->toString(nullptr)+";";
        if(value.isbool()) return (first_assignment?"bool ":"")+variableManager.getSymbol(args[0])+" = "+value->toString(nullptr)+";";
        if(value->getType()==STRING) return (first_assignment?"string ":"")+variableManager.getSymbol(args[0])+" = \""+value->toString(nullptr)+"\";";
        return "unknown type";
    }
    if(operation==ADD) return (first_assignment?"auto ":"")+variableManager.getSymbol(args[0]) + "="+variableManager.getSymbol(args[1])+"+"+variableManager.getSymbol(args[2])+";";
    if(operation==SUB) return (first_assignment?"auto ":"")+variableManager.getSymbol(args[0]) + "="+variableManager.getSymbol(args[1])+"-"+variableManager.getSymbol(args[2])+";";
    if(operation==MUL) return (first_assignment?"auto ":"")+variableManager.getSymbol(args[0]) + "="+variableManager.getSymbol(args[1])+"*"+variableManager.getSymbol(args[2])+";";
    if(operation==DIV) return (first_assignment?"auto ":"")+variableManager.getSymbol(args[0]) + "="+variableManager.getSymbol(args[1])+"/"+variableManager.getSymbol(args[2])+";";
    if(args.size()==0)return "unknown command";
    std::string ret = (first_assignment?"auto ":"")+variableManager.getSymbol(args[0]) + "="+getOperationTypeName(operation)+"(";
    for (int i = 1; i < args.size(); ++i) {
        if(i!=1)
            ret+=",";
        ret += variableManager.getSymbol(args[i]);
    }
    ret += ");";
    return ret;*/
    return "";
}