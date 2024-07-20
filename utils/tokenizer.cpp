#include <string>
#include <unordered_set>
#include <map>
#include <vector>


bool isString(const std::string& value) {
    return value[0]=='"' && value[value.size()-1]=='"';
}
bool isBool(const std::string& value) {
    return value=="true" || value=="false";
}
bool isInt(const std::string& value) {
    try{
        std:stoi(value);
        return true;
    }
    catch(...) {
        return false;
    }
}
bool isFloat(const std::string& value) {
    try{
        std:stof(value);
        return true;
    }
    catch(...) {
        return false;
    }
}


class Token {
public:
    std::string name;
    int line;
    int builtintype;
    bool printable;
    Token(const std::string& name, int line, bool printable=true): name(name), line(line), printable(printable) {
        if(isString(name))
            builtintype = 1;
        else if(isBool(name))
            builtintype = 2;
        else if(isInt(name))
            builtintype = 3;
        else if(isFloat(name))
            builtintype = 4;
        else
            builtintype = 0;
    }
};


std::vector<Token> tokenize(const std::string& text) {
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
                ret.push_back(Token(word, line));
            word = "";
            if(!inString)
                word += c;
            inString = !inString;
            continue;
        }
        if(inString) {
            word += c;
            continue;
        }
        prevChar = c;
        bool prevSpecialCharacter = specialCharacter;
        if(c==' ' || c=='\t' || c=='\n' 
            || c=='(' || c==')' || c=='[' || c==']' || c=='{' || c=='}'|| c=='=' || c==':' || c==';'|| c==',' || c=='.'
            || c=='*' || c=='+' || c=='^' || c=='-' || c=='/' || c=='%' || c=='&' || c=='|' || c=='!' || c=='<' || c=='>'
            || c=='/') {
            if(word.size())
                ret.push_back(Token(word, line));
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
            ret.push_back(Token(word, line));
            word = "";
        }
        word += c;
    }
    if(word.size())
        ret.push_back(Token(word, line));

    return std::move(ret);
}