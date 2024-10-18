#ifndef TOKENIZER_CPP
#define TOKENIZER_CPP

#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include <sstream>
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
    if (value.find('.') != std::string::npos)
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
    builtintype = determineBuiltintype(this->name);
}

Token::Token(const std::string& name, const std::string& _file, int _line, bool printable)
    : name(name), printable(printable) {
    file.push_back(_file);
    line.push_back(_line);
    builtintype = determineBuiltintype(this->name);
}

bool Token::has(int _line, const std::string& _file) const {
    for (int i = 0; i < file.size(); ++i)
        if (file[i] == _file && line[i] == _line)
            return true;
    return false;
}

std::string Token::toString() const {
    return file.back() + " line " + std::to_string(line.back());
}

std::string escapeSpecialCharacters(char c) {
    switch(c) {
        case '\n': return " ";
        case '\t': return " ";
        case '\r': return " ";
        default:   return std::string(1, c); 
    }
}

std::vector<Token> tokenize(const std::string& text, const std::string& file) {
    std::stringstream wordStream;
    int line = 1;
    std::vector<Token> ret;
    bool inComment = false;
    bool inString = false;
    bool braceMode = false;  // Indicates we are in the custom brace mode
    char prevChar = '\n';
    bool specialCharacter = false;

    for (int i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (inComment) {
            if (c != '\n')
                continue;
            else
                inComment = false;
        }
        if (c == '/' && i < text.size() - 1 && text[i + 1] == '/') {
            wordStream.str("");  // Clear the stream
            inComment = true;
            continue;
        }

        if (braceMode && c == '{')
            bberror("Cannot open a bracket `{` within a string f-expression: " + wordStream.str());

        if (braceMode && c == '}') {
            if (wordStream.str().size())
                ret.emplace_back(wordStream.str(), file, line);
            wordStream.str("");  // Clear the stream and start a new string
            wordStream.clear();
            ret.emplace_back(")", file, line);
            ret.emplace_back("+", file, line);
            wordStream << "\"";
            inString = true;
            braceMode = false;  // Exit brace mode
            specialCharacter = false;
            continue;
        }

        if (c == '"') {
            if (inString) {
                wordStream << c;
            }
            if (wordStream.str().size())
                ret.emplace_back(wordStream.str(), file, line);
            specialCharacter = false;
            wordStream.str("");  // Clear the stream
            wordStream.clear();  // Clear any errors
            if (!inString)
                wordStream << c;
            inString = !inString;
            continue;
        }

        if (inString) {
            if (c == '{') {
                // Add current string
                wordStream << "\"";
                ret.emplace_back(wordStream.str(), file, line);
                wordStream.str("");  // Clear the stream
                wordStream.clear();

                // Prepare expression
                ret.emplace_back("+", file, line);
                ret.emplace_back("str", file, line);
                ret.emplace_back("(", file, line);

                braceMode = true;  // Enter the special brace mode
                inString = false;
                specialCharacter = false;
                continue;
            } 
            wordStream << escapeSpecialCharacters(c);
            continue;
        }

        prevChar = c;
        if (c == '\\') {
            if (wordStream.str().size())
                ret.emplace_back(wordStream.str(), file, line);
            wordStream.clear();
            wordStream.str("");  // Reset the stream to store only the backslash
            wordStream<<c;
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
            if (wordStream.str().size())
                ret.emplace_back(wordStream.str(), file, line);
            wordStream.str("");  // Clear the stream
            wordStream.clear();
            if (c == '\n')
                line++;
            if (c == ' ' || c == '\t' || c == '\n')
                continue;
            specialCharacter = true;
        } else {
            specialCharacter = false;
        }

        if (prevSpecialCharacter && wordStream.str().size()) {
            ret.emplace_back(wordStream.str(), file, line);
            wordStream.str("");  // Clear the stream
            wordStream.clear();
        }
        wordStream << c;
    }
    if (inString)
        bberror("Missing `\"`. The file ended without closing the string: " + wordStream.str());
    if (wordStream.str().size())
        ret.emplace_back(wordStream.str(), file, line);
    
    return ret;
}

#endif
