#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include "common.h"


bool isString(const std::string& value) {
    return value[0]=='"' && value[value.size()-1]=='"';
}
bool isBool(const std::string& value) {
    return value=="true" || value=="false";
}
bool isInt(const std::string& value) {
    if(value.length()>3 && value.substr(0, 3)=="inf")
        return false;
    try{
        std:stoi(value);
        return true;
    }
    catch(...) {
        return false;
    }
}
bool isFloat(const std::string& value) {
    if(value.length()>3 && value.substr(0, 3)=="inf")
        return false;
    try{
        std::stof(value);
        return true;
    }
    catch(...) {
        return false;
    }
}


class Token {
public:
    std::string name;
    std::string file;
    int line;
    int builtintype;
    bool printable;
    Token(const std::string& name, const std::string& file, int line, bool printable=true): name(name), file(file), line(line), printable(printable) {
        if(isString(name)) {
            builtintype = 1;
            // replace \n with space
            size_t pos = 0;
            while ((pos = this->name.find("\n", pos)) != std::string::npos) {
                this->name.replace(pos, 1, " ");
                pos += 1;
            }
            // replace \\n with new line
            pos = 0;
            while ((pos = this->name.find("\\n", pos)) != std::string::npos) {
                this->name.replace(pos, 2, "\n");
                pos += 1; // Move past the replaced character
            }

        }
        else if(isBool(name))
            builtintype = 2;
        else if(isInt(name))
            builtintype = 3;
        else if(isFloat(name))
            builtintype = 4;
        else
            builtintype = 0;
    }

    std::string toString() const {
        return "at line "+std::to_string(line)+" in "+file;
    }
};


std::vector<Token> tokenize(const std::string& text, const std::string& file) {
    std::string word;
    int line = 1;
    std::vector<Token> ret;
    bool specialCharacter = false;
    bool inComment = false;
    bool inString = false;
    char prevChar = '\n';
    for(int i=0;i<text.size();++i) {
        char c = text[i];
        if(inComment) {
            if(c!='\n')
                continue;
            else
                inComment = false;
        }
        if(c=='/' && i<text.size()-1 && text[i+1]=='/') {
            word = "";
            inComment = true;
            continue;
        } 
        if(c=='"') {
            if(inString)
                word += c;
            if(word.size()) 
                ret.emplace_back(word, file, line);
            word = "";
            if(!inString)
                word += c;
            inString = !inString;
            continue;
        }
        if(inString) {
            if(c=='\n' || c=='\t')
                c = ' ';
            word += c;
            continue;
        }
        prevChar = c;
        if(c=='\\') {
            if(word.size())
                ret.emplace_back(word, file, line);
            word = "\\";
            specialCharacter = false;
            continue;
        }
        bool prevSpecialCharacter = specialCharacter;
        if(c==' ' || c=='\t' || c=='\n' 
            || c=='(' || c==')' || c=='[' || c==']' || c=='{' || c=='}'|| c=='=' || c==':' || c==';'|| c==',' || c=='.'
            || c=='*' || c=='+' || c=='^' || c=='-' || c=='/' || c=='%' || c=='&' || c=='|' || c=='!' || c=='<' || c=='>'
            || c=='/' || c=='#') {
            if(word.size())
                ret.emplace_back(word, file, line);
            word = "";
            if(c=='\n')
                line++;
            if(c==' ' || c=='\t' || c=='\n')
                continue;
            specialCharacter = true;
        }
        else
            specialCharacter = false;
        if(prevSpecialCharacter && word.size()) {
            ret.emplace_back(word, file, line);
            word = "";
        }
        word += c;
    }
    if(inString)
        bberror("Missing `\"`. The file ended without closing the string: "+word);
    if(word.size())
        ret.emplace_back(word, file, line);

    return std::move(ret);
}