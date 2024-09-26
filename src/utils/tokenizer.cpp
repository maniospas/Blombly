#ifndef TOKENIZER_CPP
#define TOKENIZER_CPP

#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include "utils.h"
#include "common.h"

// Check if the string is a quoted string
bool isString(const std::string& value) {
    return value[0] == '"' && value[value.size() - 1] == '"';
}

// Check if the string is a boolean
bool isBool(const std::string& value) {
    return value == "true" || value == "false";
}

// Check if the string is an integer
bool isInt(const std::string& value) {
    if (value.length() > 3 && value.substr(0, 3) == "inf")
        return false;
    try {
        std::stoi(value);
        return true;
    }
    catch (...) {
        return false;
    }
}

// Check if the string is a floating-point number
bool isFloat(const std::string& value) {
    if (value.length() > 3 && value.substr(0, 3) == "inf")
        return false;
    try {
        std::stof(value);
        return true;
    }
    catch (...) {
        return false;
    }
}

// Utility function to determine built-in type and handle newline replacement
int determineBuiltintype(std::string& name) {
    if (isString(name)) {
        size_t pos = 0;
        // Replace \n with space
        while ((pos = name.find("\n", pos)) != std::string::npos) {
            name.replace(pos, 1, " ");
            pos += 1;
        }
        // Replace \\n with newline
        pos = 0;
        while ((pos = name.find("\\n", pos)) != std::string::npos) {
            name.replace(pos, 2, "\n");
            pos += 1; // Move past the replaced character
        }
        return 1; // String type
    }
    else if (isBool(name))
        return 2; // Boolean type
    else if (isInt(name))
        return 3; // Integer type
    else if (isFloat(name))
        return 4; // Float type
    else
        return 0; // Unknown type
}




Token::Token(const std::string& name, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable)
    : name(name), file(__file), line(__line), printable(printable) {
    builtintype = determineBuiltintype(this->name);
}

Token::Token(const std::string& name, const std::string& _file, int _line, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable)
    : name(name), file(__file), line(__line), printable(printable) {
    file.push_back(_file);
    line.push_back(_line);
    builtintype = determineBuiltintype(this->name);
}

Token::Token(const std::string& name, const std::vector<std::string>& _file, const std::vector<int>& _line, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable)
    : name(name), file(__file), line(__line), printable(printable) {
    file.insert(file.end(), _file.begin(), _file.end());
    line.insert(line.end(), _line.begin(), _line.end());
    //file.push_back(_file.back());
    //line.push_back(_line.back());
    builtintype = determineBuiltintype(this->name);
}

Token::Token(const std::string& name, const std::string& _file, int _line, bool printable)
    : name(name), printable(printable) {
    file.push_back(_file);
    line.push_back(_line);
    builtintype = determineBuiltintype(this->name);
}

bool Token::has(int _line, const std::string& _file) const {
    for(int i=0;i<file.size();++i)
        if(file[i]==_file && line[i] == _line)
            return true;
    return false;
}

std::string Token::toString() const {
    return file.back() + " line " + std::to_string(line.back());
}

std::vector<Token> tokenize(const std::string& text, const std::string& file) {
    std::string word;
    int line = 1;
    std::vector<Token> ret;
    bool specialCharacter = false;
    bool inComment = false;
    bool inString = false;
    char prevChar = '\n';

    for (int i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (inComment) {
            if (c != '\n')
                continue;
            else
                inComment = false;
        }
        if (c == '/' && i < text.size() - 1 && text[i + 1] == '/') {
            word = "";
            inComment = true;
            continue;
        }
        if (c == '"') {
            if (inString)
                word += c;
            if (word.size())
                ret.emplace_back(word, file, line);
            word = "";
            if (!inString)
                word += c;
            inString = !inString;
            continue;
        }
        if (inString) {
            if (c == '\n' || c == '\t')
                c = ' ';
            word += c;
            continue;
        }
        prevChar = c;
        if (c == '\\') {
            if (word.size())
                ret.emplace_back(word, file, line);
            word = "\\";
            specialCharacter = false;
            continue;
        }
        bool prevSpecialCharacter = specialCharacter;
        if (c == ' ' || c == '\t' || c == '\n'
            || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '=' ||
            (c == ':' && (i == text.size() - 1 || text[i + 1] != ':') && (i == 0 || text[i - 1] != ':'))
            || c == ';' || c == ',' || c == '.' ||
            c == '*' || c == '+' || c == '^' || c == '-' || c == '/' || c == '%' || c == '&' || c == '|' || c == '!' || c == '<' || c == '>' ||
            c == '/' || c == '#') {
            if (word.size())
                ret.emplace_back(word, file, line);
            word = "";
            if (c == '\n')
                line++;
            if (c == ' ' || c == '\t' || c == '\n')
                continue;
            specialCharacter = true;
        }
        else
            specialCharacter = false;

        if (prevSpecialCharacter && word.size()) {
            ret.emplace_back(word, file, line);
            word = "";
        }
        word += c;
    }
    if (inString)
        bberror("Missing `\"`. The file ended without closing the string: " + word);
    if (word.size())
        ret.emplace_back(word, file, line);

    return ret;
}


#endif