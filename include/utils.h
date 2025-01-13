#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

class Token {
public:
    std::string name;
    std::vector<std::string> file;
    std::vector<int> line;
    int builtintype;
    bool printable;

    Token(const std::string& name, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable = true);
    Token(const std::string& name, const std::string& _file, int _line, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable = true);
    Token(const std::string& name, const std::vector<std::string>& _file, const std::vector<int>& _line, const std::vector<std::string>& __file, const std::vector<int>& __line, bool printable = true);
    Token(const std::string& name, const std::string& _file, int _line, bool printable = true);
    bool has(int _line, const std::string& _file) const;
    std::string toString() const;
};


void compile(const std::string& source, const std::string& destination);
void optimize(const std::string& source, const std::string& destination, bool minimify);
std::vector<Token> tokenize(const std::string& text, const std::string& file);

static void ltrim(std::string &s);
static void rtrim(std::string &s);
static void trim(std::string &s);
static std::string ltrim_copy(std::string s);
static std::string rtrim_copy(std::string s);
static std::string trim_copy(std::string s);

namespace Terminal {
    void enableVirtualTerminalProcessing();
}

#endif