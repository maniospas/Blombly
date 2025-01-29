/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
std::string ltrim_copy(std::string s);
std::string rtrim_copy(std::string s);
std::string trim_copy(std::string s);

namespace Terminal {
    void enableVirtualTerminalProcessing();
}

#endif