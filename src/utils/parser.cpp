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

#ifndef PARSER_CPP
#define PARSER_CPP

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <stack> 
#include <filesystem>
#include <iostream>
#include <fstream>
#include "utils.h"
#include "common.h"
#include "BMemory.h"
#include "data/Future.h"
#include "utils.h"
#include "interpreter/functional.h"

constexpr size_t MISSING = -1;

const std::string PARSER_BUILTIN = "BUILTIN";
const std::string PARSER_BEGIN = "BEGIN";
const std::string PARSER_BEGINFINAL = "BEGINFINAL";
const std::string PARSER_END = "END";
const std::string PARSER_RETURN = "return";
const std::string PARSER_FINAL = "final";
const std::string PARSER_AT = "at";
const std::string PARSER_IS = "IS";
const std::string PARSER_CALL = "call";
const std::string PARSER_GET = "get";
const std::string PARSER_SET = "set";
const std::string PARSER_PUT = "put";
const std::string PARSER_WHILE = "while";
const std::string PARSER_IF = "if";
const std::string PARSER_INLINE = "inline";
const std::string PARSER_TRY = "do";
const std::string PARSER_DEFAULT = "default";
const std::string PARSER_NEW = "new";
const std::string PARSER_PRBB_INT = "print";
const std::string PARSER_COPY = "copy";
const std::string ANON = "_bb";
extern std::string blombly_executable_path;
extern bool debug_info;

std::string compileFromCode(const std::string& code, const std::string& source);
std::string optimizeFromCode(const std::string& code, bool minimify);
extern void addAllowedLocation(const std::string& location);
extern void addAllowedWriteLocation(const std::string& location);
extern void clearAllowedLocations();
extern bool isAllowedLocation(const std::string& path);
extern bool isAllowedWriteLocation(const std::string& path);
extern std::string normalizeFilePath(const std::string& path);
extern bool isAllowedLocationNoNorm(const std::string& path_);
extern void preliminarySimpleChecks(std::vector<Command>* program);
std::string top_level_file;

extern BMemory cachedData;

std::string singleThreadedVMForComptime(const std::string& code, const std::string& fileName) {
    Future::setMaxThreads(1);
    std::string result;
    bool hadError = false;
    try {
        {
            BMemory memory(0, nullptr, DEFAULT_LOCAL_EXPECTATION);
            try {
                auto program = new std::vector<Command>();
                auto source = new SourceFile(fileName);
                std::string line;
                int i = 1;
                
                CommandContext* descriptor = nullptr;
                std::istringstream inputStream(code);
                while (std::getline(inputStream, line)) {
                    if (line[0] != '%') program->emplace_back(line, source, i, descriptor);
                    else descriptor = new CommandContext(line.substr(1));
                    ++i;
                }
                preliminarySimpleChecks(program);

                auto code = new Code(program, 0, program->size() - 1, program->size() - 1);

                ExecutionInstance executor(0, code, &memory, true);
                auto returnedValue = executor.run(code);
                DataPtr ret = returnedValue.get();
                if (ret.existsAndTypeEquals(FUTURE)) {
                    auto res2 = static_cast<Future*>(ret.get())->getResult();
                    ret = res2.get();
                    returnedValue.result = res2;
                }

                bbassertexplain(!returnedValue.returnSignal, "Unexpected return statement.", "`!comptime` must evaluate to a value but not run a return statement.", "");
                //bbassert(ret, "`!comptime` must evaluate to a non-missing value.");
                if(ret.isbool()) result = ret.unsafe_tobool()?"true":"false";
                else if (ret.isint() || ret.isfloat()) result = ret.torepr();
                else if(!ret.exists()) result = "#";
                else if (ret->getType() == STRING) result = "\"" + ret->toString(nullptr) + "\"";
                else {
                    if (ret->getType() == ERRORTYPE) bberror(ret->toString(nullptr));
                    bberrorexplain("Unexpected compilation outcome.", "`!comptime` must evaluate to a float, int, str, or bool.", "");
                }
            } catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        cachedData.release();
        BMemory::verify_noleaks();
    } catch (const BBError& e) {
        cachedData.release();
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }

    if (hadError) std::cerr << "Docs and bug reports for the Blombly language: https://maniospas.github.io/Blombly\n";
    return RESMOVE(result);
}



extern void replaceAll(std::string &str, const std::string &from, const std::string &to);

class Parser {
private:
    std::vector<Token> tokens;
    static int tmp_var;
    std::string ret;
    std::string code_block_prepend;
    size_t find_end(size_t start, size_t end, const std::string& end_string, bool missing_error = false) {
        if(tokens[start].name==end_string) return start;
        if(start>=tokens.size() || start>=end) {
            if(missing_error) bberror("Unexpected end of code.\n"+show_position(tokens.size()-1));
            return MISSING;
        }
        int depth = 0;
        std::stack<size_t> last_open;
        std::stack<std::string> last_open_type;
        //size_t last_open_value = start;
        last_open.push(start);
        for (size_t i = start; i <= end; ++i) {
            if (depth == 0 && tokens[i].name == end_string) return i;
            if (depth < 0) {
                if(last_open.size()>1) last_open.pop();
                //size_t pos = last_open_value;
                std::string name = tokens[last_open.top()].name; 
                if(name=="(") bberrorexplain("Imbalanced parenthesis or brackets.", "Closing `)` is missing.", show_position(last_open.top()));
                if(name=="{") bberrorexplain("Imbalanced parenthesis or brackets.", "Closing `}` is missing.", show_position(last_open.top()));
                if(name=="[") bberrorexplain("Imbalanced parenthesis or brackets.", "Closing `]` is missing.", show_position(last_open.top()));
                bberror("Did not find ending `"+end_string+"`.\n"+show_position(start));
            }
            std::string name = tokens[i].name;
            if (name == "(" || name == "[" || name == "{") {
                    last_open.push(i);
                    last_open_type.push(tokens[i].name);
                    //last_open_value = i;
                    depth += 1;
                }
            if (name == ")" || name == "]" || name == "}") {
                    //last_open_value = last_open.top();
                    if(last_open_type.size()>=1) {
                        bbassertexplain(name!=")" || last_open_type.top()=="(", "Imbalanced parenthesis or brackets.", "Closing `)` is missing.", show_position(last_open.top()));
                        bbassertexplain(name!="}" || last_open_type.top()=="{", "Imbalanced parenthesis or brackets.", "Closing `}` is missing.", show_position(last_open.top()));
                        bbassertexplain(name!="]" || last_open_type.top()=="[", "Imbalanced parenthesis or brackets.", "Closing `]` is missing.", show_position(last_open.top()));
                        last_open.pop();
                        last_open_type.pop();
                    }
                    depth -= 1;
                }
        }
        if (missing_error) {
            size_t pos = depth<0?start:last_open.top();
            if(pos>=tokens.size()) bberror("Unexpected end of code.\n"+show_position(tokens.size()-1));
            if(end_string==";") {
                std::string name = tokens[pos].name;
                if(name=="(") bberrorexplain("Closing `)` is missing.", "", show_position(pos));
                if(name=="{") bberrorexplain("Closing `}` is missing.", "", show_position(pos));
                if(name=="[") bberrorexplain("Closing `]` is missing.", "", show_position(pos));
                bberrorexplain("Closing `" + end_string + "` is missing.", "", show_position(start));
            }
            else bberrorexplain("Closing `" + end_string + "` is missing.", "", show_position(pos));
        }
        return MISSING;
    }
    
    size_t find_last_end(size_t start, size_t end, const std::string& end_string, bool missing_error = false) {
        int depth = 0;
        size_t pos = MISSING;
        size_t last_open = end;
        for (size_t i = start; i <= end; ++i) {
            if (depth == 0 && tokens[i].name == end_string) pos = i;
            if (tokens[i].name == "(" || tokens[i].name == "[" || tokens[i].name == "{") depth += 1;
            if (depth < 0) bberrorexplain("Imbalanced parentheses, brackets, or scopes", "", show_position(last_open));
            if (tokens[i].name == ")" || tokens[i].name == "]" || 
                tokens[i].name == "}") {
                if(depth==0) last_open = i;
                depth -= 1;
            }
        }
        if (missing_error && pos == MISSING) bberrorexplain("Closing " + end_string + " is missing", "", show_position(last_open));
        return pos;
    }
public:
    const std::string& get() const {return ret;}
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {}
    std::string create_temp() {
        std::string ret = "_bb" + std::to_string(tmp_var);
        tmp_var += 1;
        return ret;
    }

    static std::string create_macro_temp() {
        std::string ret = "_bbmacro" + std::to_string(tmp_var);
        tmp_var += 1;
        return ret;
    }

    std::string to_string(int start, int end) const {
        std::string expr;
        std::string prev;
        for (int i = start; i <= end; i++) {
            if (!tokens[i].printable) 
                continue;
            prev = tokens[i].name;
            expr += prev;
            if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                        && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                            && tokens[i].name!="."&& tokens[i].name!="!")
                expr += " ";
        }
        return expr;
    }

    static std::string to_string(const std::vector<Token>& tokens, int start, int end) {
        std::string expr;
        std::string prev;
        for (int i = start; i <= end; i++) {
            if (!tokens[i].printable) 
                continue;
            prev = tokens[i].name;
            expr += prev;
            if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                        && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                            && tokens[i].name!="."&& tokens[i].name!="!")
                expr += " ";
        }
        return expr;
    }

    std::string show_position(size_t pos) const {return show_position(tokens, pos);}
    std::string show_position(size_t pos, size_t start, size_t end) const {
        return show_position(tokens, pos, start, end);
    }

    static std::string show_position(const std::vector<Token>& tokens, size_t pos) {
        std::string expr;
        for(size_t mac=tokens[pos].line.size()-1;mac>=0;--mac) {
            if(expr.size()) expr += "\n";
            size_t start = pos;
            size_t end = pos;
            while(start>0 && (tokens[start-1].has(tokens[pos].line[mac], tokens[pos].file[mac]))) start--;
            while(end<tokens.size()-1 && tokens[end+1].has(tokens[pos].line[mac], tokens[pos].file[mac])) end++;

            // print previous lines
            /*int prev_start = start;
            while(prev_start>0 && (tokens[prev_start-1].has(tokens[pos].line[mac]-1, tokens[pos].file[mac]))) 
                prev_start--;
            int prev_prev_start = prev_start;
            while(prev_prev_start>0 && (tokens[prev_prev_start-1].has(tokens[pos].line[mac]-2, tokens[pos].file[mac]))) 
                prev_prev_start--;
            expr += "   \x1B[34m\u2192\033[0m ...\n   ";
            for(int i=prev_start;i<start;i++)
                if (tokens[i].printable) {
                    expr += tokens[i].name;
                    if(i<end-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                                && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]")
                        expr += " ";
                }
            if(prev_start!=start)
                expr += "\n   ";
            for(int i=prev_prev_start;i<prev_start;i++)
                if (tokens[i].printable) {
                    expr += tokens[i].name;
                    if(i<end-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                                && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]")
                        expr += " ";
                }
             if(prev_start!=prev_prev_start)
                expr += "\n";*/

            // print error line with position indicator
            expr += "  \x1B[34m\u2192\033[0m   ";
            for (size_t i = start; i <= end; i++) if (tokens[i].printable) {
                expr += tokens[i].name;
                if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                            && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                && tokens[i].name!="."&& tokens[i].name!="!"
                                && tokens[i].name!="<"&& tokens[i].name!=">"&& tokens[i].name!="-"&& tokens[i].name!="+" && tokens[i].name!="*"&& tokens[i].name!="/"&& tokens[i].name!="^"&& tokens[i].name!=">="&& tokens[i].name!="<="&& tokens[i].name!="=="
                                && tokens[i+1].name!="<"&& tokens[i+1].name!=">"&& tokens[i+1].name!="-"&& tokens[i+1].name!="+" && tokens[i+1].name!="*"&& tokens[i+1].name!="/"&& tokens[i].name!="^"&& tokens[i+1].name!=">="&& tokens[i+1].name!="<="&& tokens[i+1].name!="=="
                            )
                    expr += " ";
            }
            expr += " \x1B[90m "+tokens[pos].toString();
            expr += "\n     \x1B[31m ";
            for (size_t i = start; i < pos; i++) if (tokens[i].printable) {
                for(size_t k=0;k<tokens[i].name.size();++k) expr += "~";
                if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                            && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                && tokens[i].name!="."&& tokens[i].name!="!"
                                && tokens[i].name!="<"&& tokens[i].name!=">"&& tokens[i].name!="-"&& tokens[i].name!="+" && tokens[i].name!="*"&& tokens[i].name!="/"&& tokens[i].name!="^"&& tokens[i].name!=">="&& tokens[i].name!="<="&& tokens[i].name!="=="
                                && tokens[i+1].name!="<"&& tokens[i+1].name!=">"&& tokens[i+1].name!="-"&& tokens[i+1].name!="+" && tokens[i+1].name!="*"&& tokens[i+1].name!="/"&& tokens[i].name!="^"&& tokens[i+1].name!=">="&& tokens[i+1].name!="<="&& tokens[i+1].name!="=="
                            )
                    expr += "~";
            }
            expr += "^";
            break; // TODO: decide whether to show full replacement stack (also uncomment to debug)
        }
        std::string circular("");
        for(int j=0;j<tokens[pos].file.size()-1;++j) circular += "  \x1B[34m\u2192\033[0m   !include \x1B[35m\u2193\u2193\u2193                            \x1B[90m" + tokens[pos].file[j] + " line "+std::to_string(tokens[pos].line[j])+"\n";
        return circular+expr;
    }

    static std::string show_position(const std::vector<Token>& tokens, size_t pos, size_t start, size_t end) {
        std::string expr;
        for(size_t mac=tokens[pos].line.size()-1;mac>=0;--mac) {
            if(expr.size())
                expr += "\n";
            while(start>0 && (tokens[start-1].has(tokens[start].line[mac], tokens[start].file[mac]))) start--;
            while(end<tokens.size()-1 && tokens[end+1].has(tokens[end].line[mac], tokens[end].file[mac])) end++;

            
            // print error line with position indicator
            expr += "  \x1B[34m\u2192\033[0m   ";
            for (size_t i = start; i <= end; i++) if (tokens[i].printable) {
                expr += tokens[i].name;
                if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                            && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                && tokens[i].name!="."&& tokens[i].name!="!"
                                && tokens[i].name!="<"&& tokens[i].name!=">"&& tokens[i].name!="-"&& tokens[i].name!="+" && tokens[i].name!="*"&& tokens[i].name!="/"&& tokens[i].name!="^"&& tokens[i].name!=">="&& tokens[i].name!="<="&& tokens[i].name!="=="
                                && tokens[i+1].name!="<"&& tokens[i+1].name!=">"&& tokens[i+1].name!="-"&& tokens[i+1].name!="+" && tokens[i+1].name!="*"&& tokens[i+1].name!="/"&& tokens[i].name!="^"&& tokens[i+1].name!=">="&& tokens[i+1].name!="<="&& tokens[i+1].name!="=="
                            )
                    expr += " ";
            }
            expr += " \x1B[90m "+tokens[pos].toString();
            expr += "\n     \x1B[31m ";
            for (size_t i = start; i < pos; i++) if (tokens[i].printable) {
                for(size_t k=0;k<tokens[i].name.size();++k) expr += "~";
                if(i<tokens.size()-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                            && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                && tokens[i].name!="."&& tokens[i].name!="!"
                                && tokens[i].name!="<"&& tokens[i].name!=">"&& tokens[i].name!="-"&& tokens[i].name!="+" && tokens[i].name!="*"&& tokens[i].name!="/"&& tokens[i].name!="^"&& tokens[i].name!=">="&& tokens[i].name!="<="&& tokens[i].name!="=="
                                && tokens[i+1].name!="<"&& tokens[i+1].name!=">"&& tokens[i+1].name!="-"&& tokens[i+1].name!="+" && tokens[i+1].name!="*"&& tokens[i+1].name!="/"&& tokens[i].name!="^"&& tokens[i+1].name!=">="&& tokens[i+1].name!="<="&& tokens[i+1].name!="=="
                            )
                    expr += "~";
            }
            expr += "^";
            break; // TODO: decide whether to show full replacement stack (also uncomment to debug)
        }
        std::string circular("");
        for(int j=0;j<tokens[pos].file.size()-1;++j) circular += "  \x1B[34m\u2192\033[0m   !include \x1B[35m\u2193\u2193\u2193                            \x1B[90m" + tokens[pos].file[j] + " line "+std::to_string(tokens[pos].line[j])+"\n";
        return circular+expr;
    }

    void breakpoint(int start, int end) {
        if(!debug_info) return;
        std::string comm = to_string(start, end);
        if(comm.size()>40) comm.resize(40);
        ret += "%"+comm+" //"+tokens[start].file.back()+" line "+std::to_string(tokens[start].line.back())+"\n";
    }

    std::string parse_expression(size_t start, size_t end, bool request_block = false, bool ignore_empty = false) {
        if(start<tokens.size()-2 && tokens[start].name=="!include") {
            // leftover includes from macros can only be .bbvm includes
            bbassert(start==end-1, "`!include` leaked to next statement\n"+Parser::to_string(start, end));
            bbassert(tokens[start+1].builtintype==1, "`!include` should always be followed by a string\n"+Parser::show_position(tokens, start+2));
            std::string fileName = tokens[start+1].name;
            fileName = fileName.substr(1, fileName.size() - 2);
            std::unique_ptr<std::istream> inputFile;
            std::string code;
            try {code = read_decompressed(fileName);} 
            catch (...) {
                std::ifstream input(fileName, std::ios::in | std::ios::binary);
                bbassertexplain(input.is_open(), "Unable to open file: " + fileName, "Imported files with the explicit .bbvm extension are directly inserted as in the compiled code as if they have been autonomously compiled. They should be the outcome of compiling with the --library option, often accompanied by --norun.", show_position(start+2));
                input.seekg(0, std::ios::end);
                code.resize(static_cast<size_t>(input.tellg()));
                input.seekg(0, std::ios::beg);
                input.read(&code[0], static_cast<std::streamsize>(code.size()));
            }
            ret += cleanSymbols(code, tmp_var);
            return "#";
        }
        if (ignore_empty && start >= end) return "#";
        if(end>start+1) breakpoint(start, end);
        //try {
            bool is_final = tokens[start].name == "final";
            bbassertexplain(start <= end || (request_block && code_block_prepend.size()), 
                      "Expecting value.", "There is nothing to compute here.",
                      show_position(start));
            bbassertexplain(tokens[start].name != "#", 
                      "Unexpected symbol.",
                      "Expression cannot start with `#`. This symbol is reserved for preprocessor directives. This issue often occurs when there an erroneous macro resolution.", 
                      show_position(start));
            if (is_final) start += 1;
            bbassertexplain(start <= end, "Empty final expression", "", show_position(start));
            bbassertexplain(tokens[start].name != "#", 
                      "Unexpected symbol.",
                      "Expression cannot start with `#`. This symbol is reserved for preprocessor directives. This issue often occurs when there an erroneous macro resolution.", 
                      show_position(start));

            std::string first_name = tokens[start].name;
            if (start == end) {
                bbassertexplain(code_block_prepend.size() == 0, 
                          "Unexpected positional arguments.",
                          "Positional arguments were declared on an assignment's left-hand-side, but the right-hand-side is not an explicit code block declaration.",
                          show_position(start));
                if(tokens[start].name=="return") {
                    ret += "return # #\n";
                    return "#";
                }
                int type = tokens[start].builtintype;
                if (type) {
                    std::string var = create_temp();
                    if (type == 1) ret += PARSER_BUILTIN + " " + var + " " + first_name + "\n";
                    if (type == 2) ret += PARSER_BUILTIN + " " + var + " B" + first_name + "\n";
                    if (type == 3) ret += PARSER_BUILTIN + " " + var + " I" + first_name + "\n";
                    if (type == 4) ret += PARSER_BUILTIN + " " + var + " F" + first_name + "\n";
                    return var;
                }
                return first_name;
            }

            if (first_name == "{" && find_end(start + 1, end, "}", true) == end) {
                bbassertexplain(!is_final, "Expecting variable.", "Code block declarations cannot be final. Only variables holding the blocks can be final.", show_position(start-1));
                std::string requested_var = create_temp();
                ret += "BEGIN " + requested_var + "\n";
                ret += code_block_prepend;
                code_block_prepend = "";
                parse(start + 1, end - 1);  // important to do a full parse
                ret += "END\n";
                return requested_var;
            }
            if (first_name == "(" && find_end(start + 1, end, ")", true) == end) {
                bbassertexplain(!is_final, "Expecting variable.", "`final` is only an acceptable qualifier for variables, but here the outcome of a parenthesis was set to final.", show_position(start));
                return parse_expression(start+1, end-1, request_block, ignore_empty);
            }

            if (request_block) {
                std::string requested_var = create_temp();
                ret += "BEGIN " + requested_var + "\n";
                ret += code_block_prepend;
                code_block_prepend = "";
                if (start <= end) parse(start, end);  // important to do a full parse
                ret += "END\n";
                return requested_var;
            }
            bbassertexplain(code_block_prepend.size() == 0, 
                      "Unexpected positional arguments.",
                      "Positional arguments were declared on an assignment's left-hand-side but the right-hand-side did not evaluate to a code block.",
                      show_position(start));

            if (first_name == "if" || first_name == "catch" || 
                first_name == "while") {
                int start_parenthesis = find_end(start, end, "(");
                int start_if_body = find_end(start_parenthesis + 1, end, ")", true) + 1;
                //int condition_start_in_ret = ret.size();
                std::string condition_text;
                std::string condition;
                if(first_name=="while") {
                        std::string requested_var = create_temp();
                        ret += "BEGIN " + requested_var + "\n";
                        ret += code_block_prepend;
                        code_block_prepend = "";
                        if (start <= end)
                            parse(start+1, start_if_body-1);  // important to do a full parse
                        ret += "END\n";
                        condition = requested_var;
                        /*
                        condition = parse_expression(start + 1, start_if_body - 1);
                        if (first_name == "while" && (ret.size()<5 || ret.substr(condition_start_in_ret, 5) != "BEGIN"))
                            condition_text = ret.substr(condition_start_in_ret);
                        bbassert(condition != "#", first_name + " condition does not evaluate to anything\n"+show_position(start_parenthesis+1));
                        */
                }
                size_t body_end = first_name == "while" ? MISSING : find_end(start_if_body, end, "else");
                if (body_end == MISSING) body_end = end;
                std::string bodyvar = create_temp();
                ret += "BEGIN " + bodyvar + "\n";
                if (body_end==MISSING && find_end(start_if_body, end, "else")!=MISSING) {
                    bbassertexplain(first_name != "while", "Invalid syntax.", "`while` expressions cannot have an else branch.", show_position(body_end));
                }
                if (tokens[start_if_body].name == "{") {
                    bbassertexplain(find_end(start_if_body + 1, body_end, "}", true) == body_end || tokens[body_end].name=="else", "Invalid syntax.", "There is leftover code after closing `}`.", show_position(body_end));
                    parse(start_if_body + 1, body_end - 1);
                } else if (tokens[body_end].name == "else") {
                    parse(start_if_body, body_end - 1);
                } else {
                    parse(start_if_body, body_end);
                }
                if (first_name == "while")
                    ret += condition_text;
                ret += "END\n";
                if (body_end <= end - 1 && tokens[body_end].name == "else") {
                    bbassertexplain(first_name != "while", "Invalid syntax.", "`while` expressions cannot have an else branch.", show_position(body_end));
                    size_t else_end = end;
                    if (tokens[body_end + 1].name == "{") {
                        else_end = find_end(body_end + 2, end, "}", true);
                        bbassertexplain(else_end == end, "Invalid syntax.", "There is leftover code after closing `}` for else", show_position(else_end));
                    }
                    std::string endvar = create_temp();
                    ret += "BEGIN " + endvar + "\n";
                    if (tokens[body_end + 1].name == "{") parse(body_end + 2, else_end - 1);
                    else parse(body_end + 1, else_end);
                    ret += "END\n";
                    bodyvar += " " + endvar;
                    body_end = else_end;
                }
                bbassertexplain(body_end == end, "Invalid syntax.", "`"+first_name + "` statement body terminated before end of expression.", show_position(body_end));
                if(first_name!="while") // parse condition last to take advantage of blomblyvm hotpaths
                    condition = parse_expression(start + 1, start_if_body - 1);
                breakpoint(start, end);
                ret += first_name + " # " + condition + " " + bodyvar + "\n";
                return "#";
            }

            if (first_name == "default" || first_name == "defer" || first_name == "do") {
                std::string var = first_name != "do" ? "#" : create_temp();
                std::string called = create_temp();
                std::string parsed = parse_expression(start + 1, end, tokens[start + 1].name != "{");
                if (first_name == "new" && ret.size()>=4 && ret.substr(ret.size() - 4) == "END\n")
                    ret = ret.substr(0, ret.size() - 4) + "return # this\nEND\n";
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(start+1));
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            if (first_name == "return") {
                std::string parsed = parse_expression(start + 1, end);
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            size_t assignment = find_end(start, end, "=");
            size_t asAssignment = find_end(start, end, "as");
            if ((asAssignment < assignment && asAssignment!=MISSING) || assignment==MISSING) assignment = asAssignment;

            if (assignment != MISSING) {
                bool negation = first_name=="not";
                if(negation) {
                    bbassertexplain(asAssignment==assignment, "Unexpected symbol.", "Keyword `not` can precede an `as` but not a `=` assignment, as the latter does not return.", show_position(start));
                    start += 1;
                    first_name = tokens[start].name;
                }
                bbassertexplain(assignment != start, "Expecting variable.", "Missing a variable to assign to.", show_position(assignment));
                int isSelfOperation = 0;
                if(assignment && 
                    (  tokens[assignment-1].name=="+" 
                    || tokens[assignment-1].name=="-"
                    || tokens[assignment-1].name=="*"
                    || tokens[assignment-1].name=="/"
                    || tokens[assignment-1].name=="^"
                    || tokens[assignment-1].name=="|"
                    || tokens[assignment-1].name=="%"
                    || tokens[assignment-1].name=="and"
                    || tokens[assignment-1].name=="or"
                    || tokens[assignment-1].name=="<<"
                    )) {
                    isSelfOperation = 1; 
                    // if for whatever reason operations like +as become available (which shouldn't) then don't forget to fix subsequent code
                    bbassertexplain(asAssignment!=assignment, "Invalid syntax.", "Pattern `"+tokens[assignment-1].name+"as` is not allowed. Use `"+tokens[assignment-1].name+"=` or write the full self-operation.", show_position(assignment));
                }
                if(isSelfOperation) bbassertexplain(assignment-1 != start, "Expecting variable.", "Missing a variable to operate and assign to.", show_position(assignment-1));

                std::string returned_value = "#"; // this is set only by start_assigment!=MISSING
                size_t start_assignment = find_last_end(start, assignment, ".");
                size_t start_entry = find_last_end((start_assignment == MISSING ? start : start_assignment-isSelfOperation) + 1, assignment-isSelfOperation, "[");
                
                if (start_entry != MISSING && start_entry+1 > start_assignment+1) {
                    size_t end_entry = find_end(start_entry + 1, assignment-isSelfOperation, "]", true);
                    bbassertexplain(end_entry == assignment - 1 - isSelfOperation, "Invalid syntax.", "Non-empty expression between last closing `]` and `"+tokens[assignment-isSelfOperation].name+"`.", show_position(end_entry+1));
                    std::string obj = parse_expression(start, start_entry - 1);
                    bbassertexplain(obj != "#", "Invalid syntax.", "There is no expression outcome to assign to.", show_position(start));
                    bbassertexplain(!is_final, "Unexpected symbol.", "Entries cannot be set to final.", show_position(start-1));

                    if (isSelfOperation) {
                        std::string entry = parse_expression(start_entry + 1, end_entry - 1);
                        std::string rhs = parse_expression(assignment + 1, end);
                        std::string entryvalue = create_temp();
                        ret += "at " + entryvalue + " "+ obj + " " + entry + "\n";
                        if (tokens[assignment - 1].name == "+") ret += "add " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "-") ret += "sub " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "*") ret += "mul " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "/") ret += "div " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "^") ret += "pow " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "%") ret += "mod " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "and") ret += "and " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "or") ret += "or " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "<<") ret += "push " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        ret += "put # " + obj + " " + entry + " " + entryvalue + "\n";
                    }
                    else ret += "put # " + obj + " " + parse_expression(start_entry + 1, end_entry - 1) + " " + parse_expression(assignment + 1, end) + "\n";
                    if (asAssignment==assignment) {
                        bberrorexplain("Unexpected symbol.", "`as` cannot be used for setting values outside of the current scope. Use `=` and catch on failure instead. This prevents leaking missing values.", show_position(assignment)); // proper integration is to have "asset" and "assetfinal" keywords, but I changed my mind: this is unsafe behavior
                        std::string temp = create_temp();
                        ret += "exists " + temp + " " + first_name + "\n";
                        return temp;
                    }
                    return "#";
                }
                if (start_assignment != MISSING) {
                    bbassertexplain(start_assignment >= start + 1-isSelfOperation, "Invalid symbol.", "Assignment expression can not start with `.`.", show_position(start));
                    size_t parenthesis_start = find_end(start_assignment + 1,  assignment - 1-isSelfOperation, "(");
                    std::string obj = parse_expression(start, start_assignment - 1);
                    bbassertexplain(obj != "#", "No result found..", "There is no expression outcome to assign to.", show_position(start));
                    returned_value = obj;
                    if (parenthesis_start != MISSING) {
                        code_block_prepend = "";
                        size_t parenthesis_end = find_end(parenthesis_start + 1, assignment - 1-isSelfOperation, ")", true);
                        bbassertexplain(parenthesis_end == assignment - 1-isSelfOperation, "Incomplete expression.", "There is leftover code after last parenthesis in assignment's left hand side.", show_position(parenthesis_end));
                        size_t j = parenthesis_start+1;
                        while(j<parenthesis_end) {
                            size_t next_j = find_end(j+1, parenthesis_end, ",");
                            if(next_j==MISSING) next_j = parenthesis_end;
                            bbassert(next_j!=j, "Empty argument.\n"+show_position(j));
                            std::string name = tokens[next_j-1].name;
                            code_block_prepend += "next " + name + " args\n";

                            if (name == "bbvm::int" || name == "bbvm::float" || 
                                name == "bbvm::str" || name == "bbvm::file" || 
                                name == "bbvm::bool" ||
                                name == "bbvm::list" || name == "bbvm::map" || 
                                name == "bbvm::move" || name == "bbvm::clear" || name == "bbvm::pop" || name == "bbvm::random" || name == "bbvm::push" || 
                                name == "bbvm::len" || name == "bbvm::next" || 
                                name == "bbvm::vector" || name == "bbvm::iter" || 
                                name == "bbvm::add" || name == "bbvm::sub" || 
                                name == "bbvm::min" || name == "bbvm::max" ||  
                                name == "bbvm::put" || 
                                name == "bbvm::sum" || 
                                name == "bbvm::call" || name == "bbvm::range" || 
                                name == "bbvm::print" || name == "bbvm::read") {
                                bberror("Cannot have bbvm implementation as argument `" + name + "`.\n"+show_position(next_j-1));
                            }
                            /*if (name == "int" || name == "float" || 
                                name == "str" || name == "file" || 
                                name == "bool" || 
                                name == "list" || name == "map" || 
                                name == "move" || name == "clear" || name == "pop" || name == "push" || 
                                name == "put" || 
                                name == "len" || name == "next" || 
                                name == "vector" || name == "iter" || 
                                name == "add" || name == "sub" || 
                                name == "min" || name == "max" || 
                                name == "sum" ||
                                name == "call" || name == "range" || 
                                name == "print" || name == "read" 
                                ) {
                                bberror("Cannot have builtin symbol `" + name + "` as argument."
                                        "\n   \033[33m!!!\033[0m This prevents ambiguity by making `"+name+"` always a shorthand for `bbvm::"+name+"` .\n"
                                        +show_position(next_j-1));
                            }*/

                            if (name == "default" || 
                                name == "do" || name == "new" || 
                                name == "defer" || 
                                name == "return" || name == "if" || 
                                name == "else" || name == "while" || 
                                name == "args" || name == "as" || 
                                name == "final" ||
                                name == "this" || 
                                name == "and" || 
                                name == "or" || 
                                name == "<<" || 
                                name == "not" || 
                                name == "=" || name == "+" || 
                                name == "-" || name == "*" || 
                                name == "^" || name == "/" || 
                                name == "%" || name == "==" || 
                                name == "<=" || name == ">=" || 
                                name == "!=" || name == "(" || 
                                name == ")" || name == "{" || 
                                name == "}" || name == "[" || 
                                name == "]" || name == "." || 
                                name == "," || name == ":" || 
                                name == ";" || name == "#") {
                                bberror("Cannot have blombly keyword `" + name + "` as argument."+
                                        "\n   \033[33m!!!\033[0m For safety, all keywords are considered final.\n"
                                        + show_position(next_j-1));
                            }

                            for(size_t jj=j;jj<next_j-1;++jj) {
                                std::string semitype = tokens[jj].name;
                                if(semitype=="float" || semitype=="int" || semitype=="str" || semitype=="bool" || semitype=="list" 
                                    || semitype=="vector" || semitype=="iter"
                                    || semitype=="vector::zero" || semitype=="vector::consume" 
                                    || semitype=="vector::alloc" || semitype=="list::element"
                                    || semitype=="list::gather" || semitype=="list::gather"
                                    || semitype=="file" || semitype=="clear" || semitype=="move" 
                                    || semitype=="bbvm::float" || semitype=="bbvm::int" || semitype=="bbvm::str" || semitype=="bbvm::bool" || semitype=="bbvm::iter"
                                    || semitype=="bbvm::list" || semitype=="bbvm::vector" || semitype=="bbvm::file" || semitype=="bbvm::clear" || semitype=="bbvm::move" )
                                    code_block_prepend += semitype+" "+name+" "+name+"\n";
                                else {
                                    std::string args = create_temp();
                                    std::string temp = create_temp();
                                    code_block_prepend += "BEGIN " + args + "\n";
                                    code_block_prepend += "list::element "+temp+" "+name+"\n";
                                    code_block_prepend += "IS args " + temp + "\n";
                                    code_block_prepend += "END\n";
                                    code_block_prepend += "call "+name+" "+args+" "+semitype+"\n";
                                }
                            }
                            j = next_j+1;
                        }
                        /*for (int j = parenthesis_start + 1; j < parenthesis_end; ++j) {
                            if (tokens[j].name != ",") {
                                code_block_prepend += "next " + tokens[j].name + " args\n";
                            }
                        }*/
                    } 
                    else parenthesis_start = assignment-isSelfOperation;

                    if(isSelfOperation) {
                        std::string entry = parse_expression(start_assignment + 1, parenthesis_start - 1);
                        std::string rhs = parse_expression(assignment + 1, end);
                        std::string entryvalue = create_temp();
                        ret += "get " + entryvalue + " "+ obj + " " + entry + "\n";
                        if (tokens[assignment - 1].name == "+") ret += "add " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "-") ret += "sub " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "*") ret += "mul " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "/") ret += "div " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "^") ret += "pow " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "%") ret += "mod " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "and") ret += "and " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "or") ret += "or " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        else if (tokens[assignment - 1].name == "<<") ret += "push " + entryvalue + " " + entryvalue + " " + rhs + "\n";
                        ret += (is_final ? "setfinal # " : "set # ") + obj + " " + entry + " " + entryvalue + "\n";
                    }
                    else 
                        ret += (is_final ? "setfinal # " : "set # ") + obj + " " 
                                    + parse_expression(start_assignment + 1, parenthesis_start - 1) + " " 
                                    + parse_expression(assignment + 1, end) + "\n";
                    if (asAssignment==assignment) {
                        bberrorexplain("Unexpected symbol.", "`as` cannot be used for setting values outside of the current scope. Use only `=` and catch on failure. This prevents leaking missing values.", show_position(assignment)); // proper integration is to have "asset" and "assetfinal" keywords, but I changed my mind: this is unsafe behavior
                        std::string temp = create_temp();
                        ret += "exists " + temp + " " + first_name + "\n";
                        //bbassert(returned_value=="#", "Cannot set struct fields with `A.x as y`. Use `A.x = y` instread. To ignore errors instead use `if(y as y) A.x=y;` \n"+show_position(assignment));
                        return temp;
                    }
                    return returned_value;
                }

                size_t parenthesis_start = find_end(start + 1, assignment - 1-isSelfOperation, "(");
                bbassertexplain(parenthesis_start == MISSING ? assignment == start + 1+isSelfOperation : parenthesis_start == start + 1+isSelfOperation, 
                          "Expecting variable.", "Cannot understrand what to assign to. You can only assign to variables, struct fields, or elements", show_position(assignment, start, end));
                if (first_name == "bbvm::int" || first_name == "bbvm::float" || 
                    first_name == "bbvm::str" || first_name == "bbvm::file" || 
                    first_name == "bbvm::bool" ||
                    first_name == "bbvm::list" || first_name == "bbvm::map" || 
                    first_name == "bbvm::move" || first_name == "bbvm::clear" || first_name == "bbvm::random" || first_name == "bbvm::pop" || first_name == "bbvm::push" || 
                    first_name == "bbvm::put" ||
                    first_name == "bbvm::len" || first_name == "bbvm::next" || 
                    first_name == "bbvm::vector" || first_name == "bbvm::iter" || 
                    first_name == "bbvm::add" || first_name == "bbvm::sub" || 
                    first_name == "bbvm::min" || first_name == "bbvm::max" ||  
                    first_name == "bbvm::sum" || 
                    first_name == "bbvm::call" || first_name == "bbvm::range" || 
                    first_name == "bbvm::print" || first_name == "bbvm::read") {
                    bberrorexplain("Unexpected symbol.", "Cannot assign to bbvm implementation `" + first_name + "`.", show_position(start));
                }
                /*if (first_name == "int" || first_name == "float" || 
                    first_name == "str" || first_name == "file" || 
                    first_name == "bool" || 
                    first_name == "list" || first_name == "map" || 
                    first_name == "move" || first_name == "clear" || first_name == "pop" || first_name == "push" || 
                    first_name == "put" ||
                    first_name == "len" || first_name == "next" || 
                    first_name == "vector" || first_name == "iter" || 
                    first_name == "add" || first_name == "sub" || 
                    first_name == "min" || first_name == "max" || 
                    first_name == "sum" ||
                    first_name == "call" || first_name == "range" || 
                    first_name == "print" || first_name == "read" 
                    ) {
                    bberror("Cannot assign to builtin symbol `" + first_name + "`."
                            "\n   \033[33m!!!\033[0m This prevents ambiguity by making `"+first_name+"` always a shorthand for `bbvm::"+first_name+"` ."
                            "\n       Consider assigning to \\"+first_name+" to override the builtin's implementation for a struct,"
                            "\n       or adding a prefix like `lib::"+first_name+"` to indicate specialization."
                            "\n       If you are sure you want to impact all subsequent code (as well as included code), you may reassign the"
                            "\n       symbol with !macro {"+first_name+"} as {lib::"+first_name+"} after implementing the latter.\n"
                            +show_position(start));
                }*/

                if (first_name == "default" || 
                    first_name == "do" || first_name == "new" || 
                    first_name == "defer" || 
                    first_name == "return" || first_name == "if" || 
                    first_name == "else" || first_name == "while" || 
                    first_name == "args" || first_name == "as" || 
                    first_name == "final" ||
                    first_name == "this" || 
                    first_name == "and" || 
                    first_name == "or" || 
                    first_name == "<<" || 
                    first_name == "not" || 
                    first_name == "=" || first_name == "+" || 
                    first_name == "-" || first_name == "*" || 
                    first_name == "^" || first_name == "/" || 
                    first_name == "%" || first_name == "==" || 
                    first_name == "<=" || first_name == ">=" || 
                    first_name == "!=" || first_name == "(" || 
                    first_name == ")" || first_name == "{" || 
                    first_name == "}" || first_name == "[" || 
                    first_name == "]" || first_name == "." || 
                    first_name == "," || first_name == ":" || 
                    first_name == ";" || first_name == "#") {
                    bberrorexplain("Unexpected symbol.", 
                            "Cannot assign to blombly keyword `" + first_name + "`. For safety, all keywords are considered final.",
                            show_position(start));
                }

                if (parenthesis_start != MISSING) {
                    /*code_block_prepend = "";
                    int parenthesis_end = find_end(parenthesis_start + 1, assignment - 1-isSelfOperation, ")", true);
                    bbassert(parenthesis_end == assignment - 1-isSelfOperation, 
                              "Leftover code after last parenthesis in assignment's left hand side.\n"
                              + show_position(parenthesis_end));
                    for (int j = parenthesis_start + 1; j < parenthesis_end; ++j) {
                        if (tokens[j].name != ",") 
                            code_block_prepend += "next " + tokens[j].name + " args\n";
                    }*/


                    code_block_prepend = "";
                        size_t parenthesis_end = find_end(parenthesis_start + 1, assignment - 1-isSelfOperation, ")", true);
                        bbassertexplain(parenthesis_end == assignment - 1-isSelfOperation, 
                                  "Invalid syntax.",
                                  "There is leftover code after last parenthesis in assignment's left hand side.",
                                  show_position(parenthesis_end));
                        size_t j = parenthesis_start+1;
                        while(j<parenthesis_end) {
                            size_t next_j = find_end(j+1, parenthesis_end, ",");
                            if(next_j==MISSING) next_j = parenthesis_end;
                            bbassertexplain(next_j!=j, "Empty argument.\n", "", show_position(j));
                            std::string name = tokens[next_j-1].name;
                            code_block_prepend += "next " + name + " args\n";

                            if (name == "bbvm::int" || name == "bbvm::float" || 
                                name == "bbvm::str" || name == "bbvm::file" || 
                                name == "bbvm::bool" ||
                                name == "bbvm::list" || name == "bbvm::map" || 
                                name == "bbvm::move" || name == "bbvm::clear" || name == "bbvm::pop" || name == "bbvm::random" || name == "bbvm::push" || 
                                name == "bbvm::len" || name == "bbvm::next" || 
                                name == "bbvm::vector" || name == "bbvm::iter" || 
                                name == "bbvm::add" || name == "bbvm::sub" || 
                                name == "bbvm::min" || name == "bbvm::max" ||  
                                name == "bbvm::sum" || 
                                name == "bbvm::call" || name == "bbvm::range" || 
                                name == "bbvm::print" || name == "bbvm::read") {
                                bberrorexplain("Unexpected symbol..", "Cannot have bbvm implementation as argument `" + name + "`.", show_position(next_j-1));
                            }
                            /*if (name == "int" || name == "float" || 
                                name == "str" || name == "file" || 
                                name == "bool" || 
                                name == "list" || name == "map" || 
                                name == "move" || name == "clear" || name == "pop" || name == "push" || 
                                name == "put" ||
                                name == "len" || name == "next" || 
                                name == "vector" || name == "iter" || 
                                name == "add" || name == "sub" || 
                                name == "min" || name == "max" || 
                                name == "sum" ||
                                name == "call" || name == "range" || 
                                name == "print" || name == "read" 
                                ) {
                                bberror("Cannot have builtin symbol `" + name + "` as argument."
                                        "\n   \033[33m!!!\033[0m This prevents ambiguity by making `"+name+"` always a shorthand for `bbvm::"+name+"` .\n"
                                        +show_position(next_j-1));
                            }*/

                            if (name == "default" || 
                                name == "do" || name == "new" || 
                                name == "defer" || 
                                name == "return" || name == "if" || 
                                name == "else" || name == "while" || 
                                name == "args" || name == "as" || 
                                name == "final" ||
                                name == "this" || 
                                name == "and" || 
                                name == "or" || 
                                name == "<<" || 
                                name == "not" || 
                                name == "=" || name == "+" || 
                                name == "-" || name == "*" || 
                                name == "^" || name == "/" || 
                                name == "%" || name == "==" || 
                                name == "<=" || name == ">=" || 
                                name == "!=" || name == "(" || 
                                name == ")" || name == "{" || 
                                name == "}" || name == "[" || 
                                name == "]" || name == "." || 
                                name == "," || name == ":" || 
                                name == ";" || name == "#") {
                                bberrorexplain("Unexpected symbol.",
                                        "Cannot have blombly keyword `" + name + "` as argument. For safety, all keywords are considered final.",
                                        show_position(next_j-1));
                            }

                            for(size_t jj=j;jj<next_j-1;++jj) {
                                std::string semitype = tokens[jj].name;
                                if(semitype=="float" || semitype=="int" || semitype=="str" || semitype=="bool" || semitype=="list" 
                                    || semitype=="vector" || semitype=="iter"
                                    || semitype=="vector::zero" || semitype=="vector::consume" 
                                    || semitype=="vector::alloc" || semitype=="list::element"
                                    || semitype=="list::gather" || semitype=="list::gather"
                                    || semitype=="clear" || semitype=="move" || semitype=="file" 
                                    || semitype=="bbvm::float" || semitype=="bbvm::int" || semitype=="bbvm::str" || semitype=="bbvm::bool" || semitype=="bbvm::iter"
                                    || semitype=="bbvm::list" || semitype=="bbvm::vector" || semitype=="bbvm::file"
                                    || semitype=="bbvm::clear" || semitype=="bbvm::move" )
                                    code_block_prepend += semitype+" "+name+" "+name+"\n";
                                else {
                                    std::string args = create_temp();
                                    std::string temp = create_temp();
                                    code_block_prepend += "BEGIN " + args + "\n";
                                    code_block_prepend += "list::element "+temp+" "+name+"\n";
                                    code_block_prepend += "IS args " + temp + "\n";
                                    code_block_prepend += "END\n";
                                    code_block_prepend += "call "+name+" "+args+" "+semitype+"\n";
                                }
                            }
                            j = next_j+1;
                        }

                }

                if(isSelfOperation) {
                    bbassertexplain(asAssignment!=assignment, "Invalid syntax.", "Cannot have a self-operation for an assinment. This error should never occur as there is a previous assert. This is just future-proofing.", show_position(start));
                    if(tokens[assignment-1].name=="+") ret += "add "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    else if(tokens[assignment-1].name=="-") ret += "sub "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    else if(tokens[assignment-1].name=="*") ret += "mul "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    else if(tokens[assignment-1].name=="/") ret += "div "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    else if(tokens[assignment-1].name=="%") ret += "mod "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    if(tokens[assignment-1].name=="^") ret += "pow "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    if(tokens[assignment-1].name=="and") ret += "and "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    if(tokens[assignment-1].name=="or") ret += "or "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    if(tokens[assignment-1].name=="<<") ret += "push "+first_name+" "+first_name+" "+parse_expression(assignment + 1, end) + "\n";
                    // the |= expression is handled by the sanitizer
                }
                else ret += (asAssignment == assignment?"AS ":"IS ") + first_name + " " + parse_expression(assignment + 1, end) + "\n";
                
                if (is_final) {
                    bbassertexplain(first_name.size() < 3 || first_name.substr(0, 3) != "_bb", 
                              "Invalid macro.",
                              "_bb variables cannot be made final. This indicates an improper macro implementation.", show_position(start));
                    ret += "final # " + first_name + "\n";
                }
                if (asAssignment != MISSING) {
                    std::string temp = create_temp();
                    ret += "exists " + temp + " " + first_name + "\n";
                    if(negation) {
                        std::string temp2 = create_temp();
                        ret += "not " + temp2 + " " + temp + "\n";
                        return temp2;
                    }
                    return temp;
                }
                return "#";
            }

            bbassertexplain(!is_final, "Expecting variable.", "Only assignments to variables can be final.", show_position(start));
            bbassertexplain(tokens[start].name != "#" && tokens[start].name != "!", 
                      "Syntax error.",
                      "Expression cannot start with `"+tokens[start].name +"` here. This is likely an erroneous macro.",
                      show_position(start));
            
            if (first_name == "bbvm::print" || first_name == "print") {
                std::string parsed = parse_expression(start + 1, end);
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += "print " + var + " " + parsed + "\n";
                return var;
            }
            
            if (first_name == "bbvm::read" || first_name == "read") {
                std::string parsed = parse_expression(start + 1, end);
                std::string var = create_temp();
                breakpoint(start, end);
                ret += "read " + var + " " + parsed + "\n";
                return var;
            }
            
            if (first_name == "fail" || first_name == "bbvm::fail") {
                std::string parsed = parse_expression(start + 1, end);
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(tokens, start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += "fail " + var + " " + parsed + "\n";
                return var;
            }

            if (first_name == "return") {
                std::string parsed = parse_expression(start + 1, end);
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            size_t epush = find_last_end(start, end, "<<");
            if (epush != MISSING) {
                std::string var = create_temp();
                ret += "push " + var + " " + parse_expression(start, epush - 1) + " " + parse_expression(epush + 1, end) + "\n";
                return var;
            }

            size_t listgen = find_end(start, end, ",");
            if (listgen != MISSING) {
                std::string list_vars = parse_expression(start, listgen - 1);
                while (listgen != MISSING) {
                    size_t next = find_end(listgen + 1, end, ",");
                    if (next == MISSING)
                        list_vars += " " + parse_expression(listgen + 1, end);
                    else
                        list_vars += " " + parse_expression(listgen + 1, next - 1);
                    listgen = next;
                }
                breakpoint(start, end);
                std::string var = create_temp();
                ret += "list::element " + var + " " + list_vars + "\n";
                return var;
            }

            size_t eand = find_last_end(start, end, "and");
            size_t eor = find_last_end(start, end, "or");
            if (eand != MISSING && eand+1 > eor+1) {
                std::string var = create_temp();
                ret += "and " + var + " " + parse_expression(start, eand - 1) + " " + parse_expression(eand + 1, end) + "\n";
                return var;
            }
            if (eor != MISSING) {
                std::string var = create_temp();
                ret += "or " + var + " " + parse_expression(start, eor - 1) + " " + parse_expression(eor + 1, end) + "\n";
                return var;
            }
            if(first_name=="not") {
                std::string parsed = parse_expression(start + 1, end, tokens[start + 1].name != "{");
                std::string temp = create_temp();
                ret += "not " + temp + " " + parsed + "\n";
                return temp;
            }

            size_t eq = find_last_end(start, end, "==");
            size_t ne = find_last_end(start, end, "!=");
            if (eq != MISSING && eq+1 > ne+1) {
                std::string var = create_temp();
                ret += "eq " + var + " " + parse_expression(start, eq - 1) + " " + parse_expression(eq + 1, end) + "\n";
                return var;
            }
            if (ne != MISSING) {
                std::string var = create_temp();
                ret += "neq " + var + " " + parse_expression(start, ne - 1) + " " + parse_expression(ne + 1, end) + "\n";
                return var;
            }

            size_t lt = find_last_end(start, end, "<");
            size_t gt = find_last_end(start, end, ">");
            size_t le = find_last_end(start, end, "<=");
            size_t ge = find_last_end(start, end, ">=");
            if (lt != MISSING && lt+1 > gt+1 && lt+1 > ge+1 && lt+1 > le+1) {
                std::string var = create_temp();
                ret += "lt " + var + " " + parse_expression(start, lt - 1) + " " + parse_expression(lt + 1, end) + "\n";
                return var;
            }
            if (le != MISSING && le+1 > gt+1 && le+1 > ge+1) {
                std::string var = create_temp();
                ret += "le " + var + " " + parse_expression(start, le - 1) + " " + parse_expression(le + 1, end) + "\n";
                return var;
            }
            if (gt != MISSING && gt+1 > ge+1) {
                std::string var = create_temp();
                ret += "gt " + var + " " + parse_expression(start, gt - 1) + " " + parse_expression(gt + 1, end) + "\n";
                return var;
            }
            if (ge != MISSING) {
                std::string var = create_temp();
                ret += "ge " + var + " " + parse_expression(start, ge - 1) + " " + parse_expression(ge + 1, end) + "\n";
                return var;
            }

            size_t add = find_last_end(start, end, "+");
            size_t sub = find_last_end(start, end, "-");
            if (sub != MISSING && sub+1 > add+1) {
                std::string var = create_temp();
                ret += "sub " + var + " " + parse_expression(start, sub - 1) + " " + parse_expression(sub + 1, end) + "\n";
                return var;
            }
            if (add != MISSING) {
                std::string var = create_temp();
                ret += "add " + var + " " + parse_expression(start, add - 1) + " " + parse_expression(add + 1, end) + "\n";
                return var;
            }
            size_t mul = find_last_end(start, end, "*");
            size_t div = find_last_end(start, end, "/");
            if (div != MISSING && div+1 > mul+1) {
                std::string var = create_temp();
                ret += "div " + var + " " + parse_expression(start, div - 1) + " " + parse_expression(div + 1, end) + "\n";
                return var;
            }
            if (mul != MISSING) {
                std::string var = create_temp();
                ret += "mul " + var + " " + parse_expression(start, mul - 1) + " " + parse_expression(mul + 1, end) + "\n";
                return var;
            }
            size_t mod = find_last_end(start, end, "%");
            if (mod != MISSING) {
                std::string var = create_temp();
                ret += "mod " + var + " " + parse_expression(start, mod - 1) + " " + parse_expression(mod + 1, end) + "\n";
                return var;
            }
            size_t pow = find_last_end(start, end, "^");
            if (pow != MISSING) {
                std::string var = create_temp();
                ret += "pow " + var + " " + parse_expression(start, pow - 1) + " " + parse_expression(pow + 1, end) + "\n";
                return var;
            }

            if (tokens[end].name == ":") {
                std::string var = create_temp();
                std::string toret = "inline " + var + " " + parse_expression(start, end - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            if (first_name == "bbvm::range" || first_name=="range") {
                bbassertexplain(tokens[start + 1].name == "(", "Invalid syntax.", "Missing ( just after `" + first_name+"`.", show_position(start+1));
                bbassertexplain(find_end(start + 2, end, ")") == end, "Invalid syntax.", "Leftover code after the last `)` for `" + first_name+"`.", show_position(start+2));
                size_t separator = find_end(start + 2, end, ",");
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::") first_name = first_name.substr(6);
                std::string temp = create_temp();
                if(separator==MISSING) {
                    auto toret = first_name + " " + temp + " " + parse_expression(start + 2, end - 1) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                    return temp;
                }
                size_t separator2 = find_end(separator + 1, end, ",");
                if(separator2==MISSING) {
                    auto toret = first_name + " " + temp + " " + parse_expression(start + 2, separator - 1) + " " + parse_expression(separator + 1, end - 1) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                    return temp;
                }
                auto toret = first_name + " " + temp + " " + parse_expression(start + 2, separator - 1) + " " + parse_expression(separator + 1, separator2 - 1) + " " + parse_expression(separator2 + 1, end - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return temp;
            }

            if (first_name == "bbvm::push" || first_name=="push") {
                bbassertexplain(tokens[start + 1].name == "(", "Invalid syntax.", "Missing ( just after `" + first_name+"`.", show_position(start+1));
                bbassertexplain(find_end(start + 2, end, ")") == end, "Invalid syntax.", "Leftover code after the last `)` for `" + first_name+"`.", show_position(start+2));
                size_t separator = find_end(start + 2, end, ",");
                bbassertexplain(separator != MISSING, "Invalid syntax.", "push requires at least two arguments.", show_position(end));
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::") first_name = first_name.substr(6);
                auto toret = first_name + " # " + parse_expression(start + 2, separator - 1) + " " + parse_expression(separator + 1, end - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return "#";
            }

            if (first_name == "bbvm::graphics" || first_name=="graphics") {
                bbassertexplain(tokens[start + 1].name == "(", "Invalid syntax.", "Missing ( just after `" + first_name+"`.", show_position(start+1));
                bbassertexplain(find_end(start + 2, end, ")") == end, "Invalid syntax.", "Leftover code after the last `)` for `" + first_name+"`.", show_position(start+2));
                size_t separator = find_end(start + 2, end, ",");
                bbassertexplain(separator != MISSING, "Invalid syntax.", "graphics require three arguments.", show_position(end));
                size_t separator2 = find_end(separator + 2, end, ",");
                bbassertexplain(separator2 != MISSING, "Invalid syntax.", "graphics require three arguments.", show_position(end));
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::") first_name = first_name.substr(6);
                std::string var = create_temp();
                auto toret = first_name + " " + var + " " + parse_expression(start + 2, separator - 1) + " " + parse_expression(separator + 1, separator2 - 1) + " " + parse_expression(separator2 + 1, end - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            if (first_name == "bbvm::len" || first_name == "bbvm::iter" || 
                first_name == "bbvm::int" || first_name == "bbvm::float" || 
                first_name == "bbvm::str" || first_name == "bbvm::bool" ||
                first_name == "bbvm::file" ||  
                first_name == "bbvm::max" || first_name == "bbvm::min" || 
                first_name == "bbvm::sum" || 
                first_name == "bbvm::move" || first_name == "bbvm::clear" || first_name == "bbvm::pop" || first_name == "bbvm::random" || 
                first_name == "bbvm::file" || first_name == "bbvm::next" || 
                first_name == "bbvm::list" || first_name == "bbvm::map" || 
                first_name == "bbvm::server" || first_name == "bbvm::sqlite" || first_name == "bbvm::graphics" || 
                first_name == "bbvm::vector" ||
                first_name == "len" || first_name == "iter" || 
                first_name == "int" || first_name == "float" || 
                first_name == "str" || first_name == "bool" || 
                first_name == "file" ||
                first_name == "max" || first_name == "min" || 
                first_name == "sum" || 
                first_name == "clear" || first_name == "move" || first_name == "pop" || first_name == "random" || 
                first_name == "file" || first_name == "next" || 
                first_name == "list" || first_name == "map" || 
                first_name == "server" || first_name == "sqlite" || first_name == "graphics" || 
                first_name == "vector"
                || first_name=="vector::zero" || first_name=="vector::consume" 
                || first_name=="vector::alloc" || first_name=="list::element"
                || first_name=="list::gather" || first_name=="list::gather") {
                bbassertexplain(tokens[start + 1].name == "(", "Invalid syntax.", "Missing '(' just after '" + first_name+"'.", show_position(start+1));
                if (start + 1 >= end - 1 && (first_name == "map" || 
                                             first_name == "list")) {
                    std::string var = create_temp();
                    if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::")
                        first_name = first_name.substr(6);
                    auto toret = first_name + " " + var + "\n";
                    breakpoint(start, end);
                    ret += toret;
                    return var;
                }
                std::string parsed = parse_expression(start + 1, end);
                bbassertexplain(parsed != "#", "Invalid syntax.", "An expression that computes no value was given to `" + first_name + "`.", show_position(start+1));
                std::string var = create_temp();
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::")
                    first_name = first_name.substr(6);
                auto toret = first_name + " " + var + " " + parsed + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            if (first_name == "bbvm::time" || first_name == "bbvm::server" || first_name == "bbvm::sqlite" || first_name == "bbvm::list") {
                first_name = first_name.substr(6);
                bbassertexplain(tokens[start + 1].name == "(", "Invalid syntax", "Missing ( after " + first_name, show_position(start));
                if (first_name == "list") {
                    bbassertexplain(tokens[start + 2].name == ")", 
                              "Unexpected arguments.", 
                              "`"+first_name + "` accepts no arguments. Create lists of more arguments by pushing elements to "
                              "an empty list, or by separating values by commas like this: `l=1,2,3;`.",
                              show_position(start+2));
                }
                 else {
                    bbassertexplain(tokens[start + 2].name == ")", 
                        "Unexpected arguments.", 
                        "`"+first_name +"` accepts no arguments.",
                        show_position(start+2));
                }
                std::string var = create_temp();
                ret += first_name + " " + var + "\n";
                return var;
            }


            if (first_name == "time" || first_name == "server"  || first_name == "sqlite" || first_name == "list") {
                bbassert(tokens[start + 1].name == "(", "Missing ( after " + first_name);
                if (first_name == "list") {
                    bbassertexplain(tokens[start + 2].name == ")",
                              "Unexpected arguments.",
                              "`"+first_name + "` accepts no arguments. Create lists of more arguments "
                              "by pushing elements to an empty list, or by separating values by commas like this: `l=1,2,3;`.",
                              show_position(start+2));
                } else {
                    bbassertexplain(tokens[start + 2].name == ")",
                             "Unexpected arguments.",
                             "`"+first_name +"` accepts no arguments.",
                             show_position(start+2));
                }
                std::string var = create_temp();
                ret += first_name + " " + var + "\n";
                return var;
            }

            if (first_name == "new") {
                std::string var = first_name == "default" ? "#" : create_temp();
                std::string called = create_temp();
                std::string parsed = parse_expression(start + 1, end, 
                                                      tokens[start + 1].name 
                                                      != "{");
                if (first_name == "new" && ret.substr(ret.size() - 4) == "END\n") ret = ret.substr(0, ret.size() - 4) + "return # this\nEND\n";
                bbassertexplain(parsed != "#", "Expecting value.", "An expression that computes no value was given to `" + first_name+"`.", show_position(start+1));
                auto toret = first_name + " " + var + " " + parsed + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            
            size_t chain = find_last_end(start, end, "|");
            if (chain!=MISSING) {
                std::string var = create_temp();
                std::string callable("");
                if(end==chain+1) callable = tokens[chain+1].name;
                if (callable == "int" || callable == "float" || 
                    callable == "str" || callable == "file" || 
                    callable == "bool" ||
                    callable == "list" || callable == "map" || 
                    callable == "move" || callable == "clear" || callable == "pop" 
                    || callable == "push" || callable == "random" || 
                    callable == "len" || callable == "next" || 
                    callable == "vector" || callable == "iter" 
                    || callable=="vector::zero" || callable=="vector::consume" 
                    || callable=="vector::alloc" || callable=="list::element"
                    || callable=="list::gather" || callable=="list::gather" ||
                    callable == "add" || callable == "sub" || 
                    callable == "min" || callable == "max" || 
                    callable == "sum" || 
                    callable == "call" || callable == "range" || 
                    callable == "print" || callable == "read" ||
                    callable == "bbvm::int" || callable == "bbvm::float" || 
                    callable == "bbvm::str" || callable == "bbvm::file" || 
                    callable == "bbvm::bool" ||
                    callable == "bbvm::list" || callable == "bbvm::map" || 
                    callable == "bbvm::move" || callable == "bbvm::clear" || 
                    callable == "bbvm::pop" || callable == "bbvm::push" || callable == "bbvm::random" || 
                    callable == "bbvm::len" || callable == "bbvm::next" || 
                    callable == "bbvm::vector" || callable == "bbvm::iter" || 
                    callable == "bbvm::add" || callable == "bbvm::sub" || 
                    callable == "bbvm::min" || callable == "bbvm::max" || 
                    callable == "bbvm::sum" || 
                    callable == "bbvm::call" || callable == "bbvm::range" || 
                    callable == "bbvm::print" || callable == "bbvm::read" 
                    ) {
                        if(callable.size()>=6 && callable.substr(0, 6)=="bbvm::")
                            callable = callable.substr(6);
                    auto toret = callable+" " + var + " " + parse_expression(start, chain - 1) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                }
                else {
                    std::string parsed_args = create_temp();
                    std::string temp = create_temp();
                    ret += "BEGIN " + parsed_args + "\n";
                    ret += "list::element "+temp+" "+parse_expression(start, chain - 1)+"\n";
                    ret += "IS args " + temp + "\n";
                    ret += "END\n";
                    auto toret = "call " + var + " " + parsed_args + " " + parse_expression(chain+1, end) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                }
                return var;
            }

            size_t call = find_last_end(start, end, "(");
            if (call != MISSING && find_end(call + 1, end, ")", true) == end) {
                if (call == start)  // if it's just a redundant parenthesis
                    return parse_expression(start + 1, end - 1);
                std::string var = create_temp();
                if (tokens[call + 1].name == "{")
                    bbassertexplain(find_end(call + 1, end, "}", true) != end - 1,  
                              "Unexpected code block.",
                              "Cannot directly enclose brackets inside a method call's parenthesis to avoid code smells. Instead, you can place "
                              "any code inside the parethesis to transfer evaluated content to the method. This looks like this: `func(x=1;y=2)`.",
                              show_position(call+1));
                size_t conditional = find_end(call + 1, end, "::"); // call conditional
                std::string parsed_args;
                if (conditional == MISSING) {
                    if (find_end(call + 1, end, "=") != MISSING || find_end(call + 1, end, "as") != MISSING)  // if there are equalities, we are on kwarg mode 
                        parsed_args = parse_expression(call + 1, end - 1, true, true);
                    else if (call + 1 >= end ) parsed_args = "#"; // if we call with no argument whatsoever
                    else if (find_end(call + 1, end, ",") == MISSING && 
                               find_end(call + 1, end, "=") == MISSING && 
                               find_end(call + 1, end, "as") == MISSING && 
                               find_end(call + 1, end, ":") == MISSING && 
                               find_end(call + 1, end, ";") == MISSING) {  // if there is a list of only one element 
                        parsed_args = create_temp();
                        ret += "BEGIN " + parsed_args + "\n";
                        ret += "list::element args " + parse_expression(call + 1, end - 1) + "\n";
                        ret += "END\n";
                    } 
                    else {
                        parsed_args = create_temp();
                        ret += "BEGIN " + parsed_args + "\n";
                        ret += "IS args " + parse_expression(call + 1,  end - 1) + "\n";
                        ret += "END\n";
                    }
                } 
                else if (find_end(call + 1, end, ",") == MISSING) {
                    parsed_args = create_temp();
                    ret += "BEGIN " + parsed_args + "\n";
                    ret += "list::element args " + parse_expression(call + 1, conditional - 1) + "\n";
                    parse(conditional + 1, end - 1);
                    ret += "END\n";
                }
                else {
                    parsed_args = create_temp();
                    ret += "BEGIN " + parsed_args + "\n";
                    ret += "IS args " + parse_expression(call + 1, conditional - 1) + "\n";
                    parse(conditional + 1, end - 1);
                    ret += "END\n";
                }
                auto toret = "call " + var + " " + parsed_args + " " +  parse_expression(start, call - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            size_t arrayaccess = find_last_end(start, end, "[");
            size_t access = find_last_end(start, end, ".");
            if (arrayaccess != MISSING && arrayaccess+1>access+1) {
                size_t arrayend = find_end(arrayaccess + 1, end, "]", true);
                bbassertexplain(arrayend == end, "Invalid syntax.", "Array access `]` ended before expression end.", show_position(arrayend));
                std::string var = create_temp();
                auto toret = "at " + var + " " + parse_expression(start, arrayaccess - 1) 
                                         + " " + parse_expression(arrayaccess + 1, arrayend - 1) +  "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            if (access != MISSING) {
                std::string var = create_temp();
                auto toret = "get " + var + " " + parse_expression(start, 
                                                             access - 1) + " " 
                                                             + parse_expression(
                                                             access + 1, end) 
                                                             + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

        
        if(tokens[start].name=="(") bberrorexplain("Invalid syntax.", "Malformed parenthesis or function call.", show_position(start));
        bberrorexplain("Invalid syntax.", "Did you forget the closing `;` here? ~ This issue occurs because the next invalid statement could not be understood: `"+to_string(start, end)+"`.", show_position(start));
        /*} catch (const BBError& e) {
            if (tokens[start].line != tokens[end].line)
                throw e;
            std::string linestr = to_string(start, end);
            linestr.resize(40, ' ');
            throw BBError(e.what() + ("\n   \x1B[34m\u2192\033[0m " + linestr + " \t\x1B[90m " +tokens[start].toString()+": "+to_string(start,end));
        }*/
       return "";
    }
    void parse(int start, int end) {
        int statement_start = start;
        //try {
            while (statement_start <= end) {
                size_t statement_end = find_end(statement_start + 1, end, ";");//, end - start == tokens.size() - 1 && statement_start<end);
                if (statement_end == MISSING) statement_end = end + 1;
                parse_expression(statement_start, statement_end - 1);
                statement_start = statement_end + 1;
            }
        /*} catch (const BBError& e) {
            std::string linestr = to_string(start, end);
            linestr.resize(40, ' ');
            throw BBError(e.what() + ("\n   \x1B[34m\u2192\033[0m " + linestr 
                                      + " \t\x1B[90m " + "at" + " line " + 
                                      std::to_string(tokens[start].line)));
        }*/
    }
};

int Parser::tmp_var = 0;


void sanitize(std::vector<Token>& tokens) {
    std::vector<Token> updatedTokens;
    std::stack<size_t> last_open;
    std::stack<std::string> last_open_type;
    for (size_t i = 0; i < tokens.size(); ++i) {
        /*if (tokens[i].name == "{" && i>0 && tokens[i-1].name!="=" && tokens[i-1].name!="as" && tokens[i-1].name!=")") {
            updatedTokens.emplace_back("=", tokens[i].file, tokens[i].line, false);
        }*/
        if(tokens[i].name == "{" || tokens[i].name == "(" || tokens[i].name == "[") {
            last_open.push(i);
            last_open_type.push(tokens[i].name);
        }
        if(tokens[i].name == "}" || tokens[i].name == ")" || tokens[i].name == "]") {
            if(!last_open.size()) bberrorexplain("Unexpected symbol.", "Imbalanced parenthesis or bracket closes here.", Parser::show_position(tokens, i));
            if(last_open_type.top()=="{") bbassertexplain(tokens[i].name=="}", "Unexpected symbol.", "Never closed this parenthesis or bracket. This check is performed before applying macros to ensure that code is well-formed.", Parser::show_position(tokens, last_open.top()));
            if(last_open_type.top()=="(") bbassertexplain(tokens[i].name==")", "Unexpected symbol.", "Never closed this parenthesis or bracket. This check is performed before applying macros to ensure that code is well-formed.", Parser::show_position(tokens, last_open.top()));
            if(last_open_type.top()=="[") bbassertexplain(tokens[i].name=="]", "Unexpected symbol.", "Never closed this parenthesis or bracket. This check is performed before applying macros to ensure that code is well-formed.", Parser::show_position(tokens, last_open.top()));
            last_open.pop();
            last_open_type.pop();
        }

        if (tokens[i].name == "\\") bberror("A stray `\\` was encountered.\n" + Parser::show_position(tokens, i));

        if (tokens[i].name.size() >= 3 && tokens[i].name.substr(0, 3) == "_bb")
            bberrorexplain("Unexpected symbol.", "Variable name `" + tokens[i].name + "` cannot start with _bb. "
                    "Names starting with this prefix are reserved for VM local temporaries created by the compiler or macros. This check ensures that you cannot mess with the compiler's validity.",
                    Parser::show_position(tokens, i));
        
        if ((tokens[i].name=="=" || tokens[i].name=="as") && i 
            && (tokens[i-1].name=="|")) { // "this is handled here, other operations by the same parse, allow |as for this specifically"
            int start = i-2;
            while(start>=0) {
                if(tokens[start].name=="final" || tokens[start].name==";")
                    break;
                start -= 1;
                if(tokens[start].name=="(" || tokens[start].name=="{")// || tokens[start].name=="[") 
                    break;
                bbassertexplain(tokens[start].name!="}", "Unexpected symbol.", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a bracket `}`. Please resort to var = expression assignment.", Parser::show_position(tokens, i));
                bbassertexplain(tokens[start].name!=")", "Unexpected symbol.", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a parenthesis `)`. Please resort to var = expression assignment.", Parser::show_position(tokens, i));
                //bbassert(tokens[start].name!="]", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a parenthesis `]`. Please resort to var = expression assignment.\n"+Parser::show_position(tokens, i));
                bbassertexplain(tokens[start].name!="=", "Unexpected symbol.", "Previous code has not been balanced and there is a `=` before `"+tokens[i-1].name+ tokens[i].name +"`.", Parser::show_position(tokens, i));
            }
            //if(start==-1 || tokens[start].name==";" || tokens[start].name=="{")
                start += 1;
            updatedTokens.pop_back();
            updatedTokens.push_back(tokens[i]);
            updatedTokens.insert(updatedTokens.end(), tokens.begin()+start, tokens.begin()+i);
            continue;
        }

        /*if(tokens[i].name=="-" && i<tokens.size()-1 && tokens[i+1].name==">") {
            i += 1;
            updatedTokens.emplace_back("return", tokens[i].file, tokens[i].line, true); // => as return statement
            continue;
        }*/

        if(tokens[i].name=="<" && i<tokens.size()-1 && tokens[i+1].name=="<") {
            updatedTokens.emplace_back("<<", tokens[i].file, tokens[i].line, false);
            i += 1;
            continue;
        }


        if (tokens[i].name=="-" && (i==0 || (tokens[i-1].name=="=" || tokens[i-1].name=="as" || tokens[i-1].name=="{" ||  tokens[i-1].name=="[" || tokens[i-1].name=="(" || tokens[i-1].name==",")) 
            && i < tokens.size() - 1 && (tokens[i + 1].builtintype == 3 || tokens[i + 1].builtintype == 4)) {
            tokens[i].name = "-" + tokens[i + 1].name;
            tokens[i].builtintype = tokens[i + 1].builtintype;
            i += 1;
            // negative floats
            if (tokens[i].builtintype == 3 && i < tokens.size() - 2 && 
                tokens[i + 1].name == "." && tokens[i + 2].builtintype == 3) {
                tokens[i].name = "-"+tokens[i].name+"." +tokens[i + 2].name;
                tokens[i].builtintype = 4;
                updatedTokens.push_back(tokens[i]);
                i += 2;
            }
            else
                updatedTokens.push_back(tokens[i-1]);
            continue;
        }
        if (tokens[i].builtintype == 3 && i < tokens.size() - 2 && 
            tokens[i + 1].name == "." && tokens[i + 2].builtintype == 3) {
            tokens[i].name = tokens[i].name+"." +tokens[i + 2].name;
            tokens[i].builtintype = 4;
            updatedTokens.push_back(tokens[i]);
            i += 2;
            continue;
        }
        if (tokens[i].name == "." && i < tokens.size() - 1 && 
            tokens[i + 1].name.size()>1 && tokens[i + 1].name[0] == '\\' && 
            (i == 0 || tokens[i - 1].name != "this")) {
            bberror("The pattern `.\\` is not allowed."
                    "\n   \033[33m!!!\033[0m Variables starting with `\\` are considered private and not"
                    "\n        meant to be accessed directly as struct fields"
                    "\n        They can still be final or accessed from within the class's scope"
                    "\n        through  `this\\field`. This syntax works only if `this` is explicitly"
                    "\n        accessed (e.g., `temp=this;temp\\field;` fails.)\n"
                    +Parser::show_position(tokens, i));

        }
        if (tokens[i].name.size()>1 && tokens[i].name[0] == '\\' && i > 0 && tokens[i - 1].name == "this") {
            // convert this\property to this.\property
            updatedTokens.emplace_back(".", tokens[i].file, tokens[i].line, false);
            updatedTokens.push_back(tokens[i]);
            continue;
        }

        if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 1 && tokens[i + 1].name == "x") {
            updatedTokens.emplace_back("next", tokens[i].file, tokens[i].line, true);
            updatedTokens.emplace_back("(", tokens[i].file, tokens[i].line, true);
            updatedTokens.emplace_back("args", tokens[i].file, tokens[i].line, true);
            updatedTokens.emplace_back(")", tokens[i].file, tokens[i].line, true);
            i += 1;
            continue;
        }

        if (tokens[i].name == "." && i < tokens.size() - 1 && tokens[i+1].name == "this") {
            bberror("Directly accessing `.this` as a field is not allowed."
                    "\n   \033[33m!!!\033[0m You may assign it to a new accessible variable per `scope=this;`,"
                    "\n       This error message prevents the pattern `obj.this\\field`, that would be able"
                    "\n       to leak infomration. The prevention works because private fields like `\\field`"
                    "\n       are only directly accessible from the keyword `this` like so: `this\\field`.\n"
                    + Parser::show_position(tokens, i));

        }
        if ((tokens[i].name == "<" || tokens[i].name == ">" || 
             tokens[i].name == "=" || tokens[i].name == "!") && 
             i < tokens.size() - 1 && tokens[i + 1].name == "=") {
            tokens[i].name += "=";
            updatedTokens.push_back(tokens[i]);
            ++i;
            continue;
        }

        /*if ((tokens[i].name == "#" || tokens[i].name == "!") && ((i >= tokens.size() - 1) || 
            (tokens[i + 1].name != "include" && tokens[i + 1].name != "local"
             && tokens[i + 1].name != "macro" && tokens[i + 1].name != "stringify" 
             && tokens[i + 1].name != "symbol"
             && tokens[i + 1].name != "anon" && tokens[i + 1].name != "x"
             && tokens[i + 1].name != "spec" && tokens[i + 1].name != "fail" 
             && tokens[i + 1].name != "of"
             && tokens[i + 1].name != "access"
             && tokens[i + 1].name != "modify"
             && tokens[i + 1].name != "comptime"))) {
            bberror("Invalid preprocessor instruction after `"+tokens[i].name+"` symbol."
                    "\n   \033[33m!!!\033[0m This symbol signifies preprocessor directives."
                    "\n        Valid directives are the following patterns:"
                    "\n        - `!include @str` inlines a file."
                    "\n        - `!comptime @expr` evaluates an expression during compilation (it is constant during runtime)."
                    "\n        - `(!of @expression)` or (!anon @expression)` assigns the expression to a temporary variable just after the last command."
                    "\n        - `!x` is a shorthand for `next(args)`."
                    "\n        - `!macro {@expression} as {@implementation}` defines a macro."
                    "\n        - `!local {@expression} as {@implementation}` defines a macro that is invalidated when the current file ends."
                    "\n        - `!stringify (@tokens)` converts the tokens into a string at compile time."
                    "\n        - `!symbol (@tokens)` converts the tokens into a symbol name at compile time."
                    "\n        - `!access @str` grants read access to locations starting with the provided string (`!comptime` also gets these rights but cannot add to it)."
                    "\n        - `!modify @str` grants read and write access to locations starting with the provided string (`!comptime` also gets these rights but cannot add to it)."
                    "\n        - `!fail @message` creates a compile-time failure.\n"
                    + Parser::show_position(tokens, i));
        }*/

        updatedTokens.push_back(tokens[i]);

        /*if ((tokens[i].name == "if" || tokens[i].name == "while") && ((i==0) || 
            (tokens[i - 1].name != ";" && tokens[i - 1].name != "{" && tokens[i - 1].name != "}" && tokens[i - 1].name != "do" && tokens[i - 1].name != "default")))
            bberror("`"+tokens[i].name+"` statements must be preceded by one of `do`, `default`, '{', '}', or ';'. "
                    "\n   \033[33m!!!\033[0m These statements must be preceded by"
                    "\n       one of the tokens `do`, `default`, '{', '}', or ';'\n"
                    + Parser::show_position(tokens, i));*/
        
        /*if (tokens[i].name == "do" && i > 0 && tokens[i - 1].name == "=")
            bberror("Replace `= do` with `as do`. "
                    "\n   \033[33m!!!\033[0m Try statements may encounter neither exceptions "
                    "\n        nor value returns. As a blanket protection against missing"
                    "\n        value errors other than your error handling, use `as` instead of `=` .\n"
                    + Parser::show_position(tokens, i-1));*/

        if (tokens[i].name == "else" && i > 0 && tokens[i - 1].name == ";")
            bberrorexplain("Unexpected symbol.", 
                    "`else` cannot be the next statement after `;`. You may have failed to close brackets "
                    "or are using a bracketless if, which should not have `;` after its first statement.",
                    Parser::show_position(tokens, i));
                    
        if (tokens[i].name == ")" && i > 0 && tokens[i - 1].name == ";")
            bberrorexplain("Unexpected symbol.",
                    "The pattern ';)' is not allowed. This error appears so that expressions inside parentheses "
                    "remain as consise as possible while keeping only one viable syntax.",
                    Parser::show_position(tokens, i-1));

        if((tokens[i].name == "while" || tokens[i].name == "if" || 
             tokens[i].name == "catch") && i < tokens.size() - 1 && 
             tokens[i + 1].name != "(")
            bberrorexplain("Unexpected symbol.", "`(` should always follow `" + tokens[i].name + "` but " + tokens[i + 1].name + " was encountered.", Parser::show_position(tokens, i+1));
        if((tokens[i].name == "new") && i < tokens.size() - 1 && tokens[i + 1].name != "{")
            bberrorexplain("Unexpected symbol.",
                    "`{` should always follow `" + tokens[i].name + "` but `" + tokens[i + 1].name + "` was encountered. "
                    "Since nitializing a struct based on a code block variable and no arguments "
                    "is a code smell, explicitly inline the block this: `" + tokens[i].name + " {block:}`.",
                    Parser::show_position(tokens, i+1));

        if(tokens[i].name == "}") {
            if(i >= tokens.size() - 1) updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
            else if(tokens[i + 1].name == ";") bberrorexplain("Unexpected symbol.", "The syntax `};` is invalid. Both symbols terminate expressions. Use only `}`.", Parser::show_position(tokens, i));
            else if(tokens[i + 1].name == ":") bberrorexplain("Unexpected symbol.", "The syntax `}:` is invalid. Inlining a just-declared code block is equivalent to running its code immediately. Maybe you did not mean to add brackets?", Parser::show_position(tokens, i));
            else if(tokens[i + 1].name != "." && tokens[i + 1].name != ")" && 
                     tokens[i + 1].name != "," && tokens[i + 1].name != "+" && 
                     tokens[i + 1].name != "-" && tokens[i + 1].name != "*" && 
                     tokens[i + 1].name != "/" && tokens[i + 1].name != "^" && 
                     tokens[i + 1].name != "<<" && tokens[i + 1].name != "<" && tokens[i + 1].name != ">" && 
                     tokens[i + 1].name != "==" && tokens[i + 1].name != "!=" && tokens[i + 1].name != "<=" && tokens[i + 1].name != ">=" &&   
                     tokens[i + 1].name != "%" && tokens[i + 1].name != "else" && tokens[i + 1].name != "|" && tokens[i + 1].name != "(" && tokens[i + 1].name != "[")
                updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
        } 
        else if(tokens[i].name == ":") {
            if (i >= tokens.size() - 1) updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
            else if (tokens[i + 1].name == ";") bberrorexplain("Unexpected symbol.", ":; is an invalid syntax. Use only `:`.", Parser::show_position(tokens, i));
            else updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
        }
    }

    if(last_open.size()) bberrorexplain("Unexpected symbol.", "Never closed this parenthesis or bracket. This check is performed before applying macros to ensure that code is well-formed.", Parser::show_position(tokens, last_open.top()));
            

    tokens = (updatedTokens);
}

class Macro {
public:
    std::vector<Token> from;
    std::vector<Token> to;
    std::string tied_to_file;
    Macro() {}
};

void macros(std::vector<Token>& tokens, const std::string& first_source) {
    std::vector<Token> updatedTokens;
    std::vector<std::shared_ptr<Macro>> macros;
    std::unordered_set<std::string> doNotAllowOtherImports;
    std::unordered_map<std::string, std::string> previousImports;
    previousImports[first_source] = "main file: "+first_source;
    int inclusionDepth = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        /*if ((tokens[i].name == "iter" || tokens[i].name == "bbvm::iter") && i>0 && tokens[i-1].name != "as" && tokens[i-1].name != "=" && tokens[i-1].name != ",")
            bberror("`"+tokens[i].name+"` statements must be preceded by one of `=`, `as`, or ','. "
                    "\n   \033[33m!!!\033[0m This way, you may only directly assign to an iterator or add it to a list."
                    "\n       For example, the pattern `A = 1,2,3; while(x as next(bbvm::iter(A))) {}` that leads to an infinite loop is prevented."
                    "\n       You may instead consider the preprocessor #of directive to precompute the contents of a parenthesis"
                    "\n       before the current commend. Here is an example `A = 1,2,3; while(x as next(#of bbvm::iter(A))) {}`.\n"
                    + Parser::show_position(tokens, i));*/
        if(tokens[i].name == "." && i<tokens.size()-1 && tokens[i+1].name==".") {
            bberrorexplain("Unexpected symbol.", 
                "More than one fullstop can only follow `this` to indicate values obtained from its declration closure.", 
                Parser::show_position(tokens, i));
        }
        if(tokens[i].name == "{") inclusionDepth++;
        if(tokens[i].name == "}") {
            inclusionDepth--;
            if(inclusionDepth<0) bberrorexplain("Unexpected symbol.", "Imbalanced bracket closes here.", Parser::show_position(tokens, i));
        }
        if(tokens[i].name == "this" && i<tokens.size()-2 && tokens[i+1].name=="." && tokens[i+2].name==".") {
                ++i; // skip this
                int originali = i;
                int countWedges = 1;
                ++i;
                while(i<tokens.size()) {
                    if(tokens[i].name==".") ++countWedges; else break;
                    ++i;
                }
                bbassertexplain(i<tokens.size(), "Invalid syntax.", "Runaway `.` encountered at the end of file.", Parser::show_position(tokens, i));
                int pos = updatedTokens.size();
                bool alreadyDone = false;
                std::string closureName = tokens[i].name;
                for(int k=0;k<countWedges;++k) closureName = "."+closureName;
                std::string nextName = closureName.substr(1);

                updatedTokens.emplace(updatedTokens.end(), closureName, tokens[i].file, tokens[i].line, true);
                int prevInMethod = 0;
                bool hasProgressed = false;
                int depth = 0;
                while(true) {
                    bbassertexplain(pos>=0, 
                            "Too many closure escapes.",
                            "You tried to compute a closure of `this` by following it with a number of fots `.`. "
                            "Each of those dots escapes one closure, but their total number is higher than the maximal closure "
                            "nesting that would be possible. ~ ~ "
                            "Each closure can only correspond to structs being created with `new` statements or code blocks. "
                            "Here is an example `value=1; a = new{float=>this..value} print(a|float);`. ~ ~ "
                            "A common mistake that makes this error appear is thinking that functions can capture closure. "
                            "They do not. When they appear to do so, they are actually struct methods or functions defined "
                            "within struct methods, in which case closure values are stored in the struct. ~ ",
                            Parser::show_position(tokens, originali));
                    if(updatedTokens[pos].name=="}") depth++;
                    if(updatedTokens[pos].name=="{") {if(depth>0)depth--; else hasProgressed=false;}
                    if(depth!=0 || hasProgressed) {
                        --pos;
                        continue;
                    }
                    /*if(pos>=3 && updatedTokens[pos].name==";" 
                        && updatedTokens[pos-1].name==nextName
                        && updatedTokens[pos-2].name=="="
                        && updatedTokens[pos-3].name==closureName) {
                            alreadyDone = true;
                        }*/
                    if(pos>=5 && updatedTokens[pos].name==";" 
                        && updatedTokens[pos-1].name==nextName
                        && updatedTokens[pos-2].name=="."
                        && updatedTokens[pos-3].name=="this"
                        && updatedTokens[pos-4].name=="="
                        && updatedTokens[pos-5].name==closureName) {
                            alreadyDone = true;
                        }
                        
                    if(updatedTokens[pos].name=="{" && (pos==0 || updatedTokens[pos-1].name=="new" 
                            || updatedTokens[pos-1].name=="=" 
                            || updatedTokens[pos-1].name=="return"
                            || updatedTokens[pos-1].name=="as")) {
                        //bool isClassDefinition = false;
                        bbassert(hasProgressed || !prevInMethod || pos==0 || updatedTokens[pos-1].name=="new", "Attempted to obtain a closure from within a closure.\n"
                                                    +Parser::show_position(tokens, originali)
                                                    +"\nHere is where there was an attempt to obtain the closure::\n"+Parser::show_position(updatedTokens, prevInMethod)
                                                    +"\nHere is the parent from which the closure would be obtained (it's not a struct declaration):\n"+Parser::show_position(updatedTokens, pos));
                        if(!alreadyDone) { 
                            updatedTokens.emplace(updatedTokens.begin()+pos+1, closureName, tokens[i].file, tokens[i].line, true);
                            updatedTokens.emplace(updatedTokens.begin()+pos+2, "=", tokens[i].file, tokens[i].line, true);
                            updatedTokens.emplace(updatedTokens.begin()+pos+3, "this", tokens[i].file, tokens[i].line, true);
                            updatedTokens.emplace(updatedTokens.begin()+pos+4, ".", tokens[i].file, tokens[i].line, true);
                            updatedTokens.emplace(updatedTokens.begin()+pos+5, nextName, tokens[i].file, tokens[i].line, true);
                            updatedTokens.emplace(updatedTokens.begin()+pos+6, ";", tokens[i].file, tokens[i].line, true);
                        }
                        // if(updatedTokens[pos-1].name!="new") prevInMethod = pos; else prevInMethod = 0;
                        closureName = nextName;
                        nextName = closureName.substr(1);
                        hasProgressed = true;
                        --countWedges;
                        if(countWedges==0) break;
                        alreadyDone = false;
                    }

                    --pos;
                }
            ++i;
            //std::cout<<Parser::to_string(updatedTokens, 0, updatedTokens.size())<<"\n";
        }
        else if((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && (tokens[i + 1].name == "of" || tokens[i + 1].name == "anon")) {
                bbassertexplain(tokens[i - 1].name == "(" || tokens[i - 1].name == "[", 
                          "Unexpected symbol.",
                          "Unexpected `!of` encountered after `"+tokens[i - 1].name +"`. "
                          "Each `!of` (or equivalently `!anon`) declaration can only start after a parenthesis or square bracket. "
                          "Here is an example `A = 1,2,3; while(x as next(!of bbvm::iter(A))) {}`.",
                          Parser::show_position(tokens, i));
                size_t iend = i+2;
                int depth = 1;
                while(iend<tokens.size()) {
                    if(tokens[iend].name=="(" || tokens[iend].name=="{" || tokens[iend].name=="[") depth += 1;
                    if(tokens[iend].name==")" || tokens[iend].name=="}" || tokens[iend].name=="]") depth -= 1;
                    if(depth==0) break;
                    iend += 1;
                }
                bbassertexplain(iend<tokens.size() && (tokens[iend].name==")" || tokens[iend].name=="]"), 
                    "Invalid syntax.",
                    tokens[i - 1].name == "("?"`(!of @code)` or `(!anon @code)` statement was never closed with a right symbol.":"`[!of @code]` or `(!anon @code)` statement was never closed with a right symbol.",
                    Parser::show_position(tokens, i-1)
                );
                int position = updatedTokens.size();
                //depth = 0;
                while(position>0)  {
                    position -= 1;
                    /*if(updatedTokens[position].name=="(" || updatedTokens[position].name=="{" || updatedTokens[position].name=="[")
                        depth -= 1;
                    if(updatedTokens[position].name==")" || updatedTokens[position].name=="}" || updatedTokens[position].name=="]")
                        depth += 1;*/
                    if(updatedTokens[position].name==";" || updatedTokens[position].name=="{") {
                        position += 1;
                        break;
                    }
                }
                std::vector<Token> newTokens;
                std::string temp = Parser::create_macro_temp();
                newTokens.emplace_back(temp, tokens[i].file, tokens[i].line, true);
                newTokens.emplace_back("=", tokens[i].file, tokens[i].line, true);
                newTokens.insert(newTokens.end(), tokens.begin()+i+2, tokens.begin()+iend);
                newTokens.emplace_back(";", tokens[i].file, tokens[i].line, true);

                updatedTokens.insert(updatedTokens.begin()+position, newTokens.begin(), newTokens.end());
                updatedTokens.emplace_back(temp, tokens[i].file, tokens[i].line, true);
                i = iend;
        }
        if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && tokens[i + 1].name == "fail") {
            std::string message;
            size_t pos = i+2;
            std::string involved = Parser::show_position(tokens, i);
            while(pos<tokens.size()) {
                if(tokens[pos].name==";" || pos==i+3) break;
                if(tokens[pos].builtintype==1) message += tokens[pos].name.substr(1, tokens[pos].name.size()-2) + " ";
                else message += tokens[pos].name + " ";
                pos++;
                std::string inv = Parser::show_position(tokens, pos);
                if(involved.find(inv)==std::string::npos) involved += "\n"+inv;
            }
            replaceAll(message, "\\n", "\n");
            bberror(message+"\n"+involved);
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && (i < tokens.size()-2) && tokens[i+1].name=="access") {
            bbassert(tokens[i+2].builtintype==1, "`!access` should always be followed by a string\n"+Parser::show_position(tokens, i+2));
            std::string libpath = tokens[i+2].name;    
            std::string source = libpath.substr(1, libpath.size() - 2);
            if(top_level_file!=tokens[i].file.back()) {
                bbassertexplain(isAllowedLocation(source), "Unexpected `!access` for new permissions.",
                                            "This preprocessor directive creates permissions only if encountered at the top-level (aka main) "
                                            "file being parsed. Permissions then passed on to all `!include` and `!comptime` directives, like the one that has "
                                            "just been interrupted. Here, you cannot add new permissions that are not already present. "
                                            "The permissions being requested can be added your main file at an earlier stage after "
                                            "reviewing them. Add either a generalization or the following: `!access "+ tokens[i+2].name+"` "
                                            "Beware that permissions are transferred to your main application if not further modified.",
                                            Parser::show_position(tokens, i));

            }
            else {
                addAllowedLocation(source);
            }
            i += 2;
            continue;
        }
        if ((tokens[i].name == "#" || tokens[i].name == "!") && (i < tokens.size()-3) && tokens[i+1].name=="modify") {
            bbassert(tokens[i+2].builtintype==1, "`!modify` should always be followed by a string\n"+Parser::show_position(tokens, i+2));
            std::string libpath = tokens[i+2].name;    
            std::string source = libpath.substr(1, libpath.size() - 2);
            if(top_level_file!=tokens[i].file.back()) {
                bbassertexplain(isAllowedWriteLocation(source), "Unexpected `!modify` for new permissions.",
                                            "This preprocessor directive creates permissions only if generated at the top-level (aka main) "
                                            "file being parsed. It is then passed on to all `!include` and `!comptime` directives, like the one that has "
                                            "just been interrupted. Here, you cannot add new permissions that are not already present. "
                                            "The permissions being requested can be added your main file at an earlier stage after "
                                            "reviewing them. Add either a generalization or the following: `!modify "+tokens[i+2].name+"` "
                                            "Beware that permissions are transferred to your main application if not further modified.",
                                            Parser::show_position(tokens, i));

            }
            else {
                addAllowedWriteLocation(source);
            }
            i += 2;
            continue;
        }

        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && tokens[i + 1].name == "spec") {
            size_t specNameEnd = MISSING;
            size_t specNameStart = MISSING;
            if (i > 1) {
                bbassertexplain(tokens[i - 1].name == "{", 
                          "Unexpected symbol.",
                          "Each `#spec` declaration can only reside in named code blocks. These refer to code blocks being declared and assigned to at least one variable.",
                          Parser::show_position(tokens, i));
                bbassertexplain(tokens[i - 2].name == "=" || tokens[i - 2].name == "as", 
                          "Invalid syntax.",
                          "Each `#spec` declaration can only reside in named code blocks. These refer to code blocks being declared and assigned to at least one variable.",
                          Parser::show_position(tokens, i));
                int position = i - 1;
                int depth = 0;
                while (position > 0) {
                    position -= 1;
                    if (tokens[position].name == "[" || 
                        tokens[position].name == "(" || 
                        tokens[position].name == "{")
                        depth += 1;
                    if (tokens[position].name == "]" || 
                        tokens[position].name == ")" || 
                        tokens[position].name == "}")
                        depth -= 1;
                    if (depth == 0)
                        break;
                }
                bbassertexplain(depth == 0, 
                          "Unexpected symbol.",
                          "Each `#spec` declaration can only reside in named code blocks. These refer to code blocks being declared and assigned to at least one variable.",
                          Parser::show_position(tokens, i));
                specNameEnd = position - 1;
                specNameStart = specNameEnd;
                depth = 0;
                while (specNameStart >= 0) {
                    if (tokens[specNameStart].name == "[" || 
                        tokens[specNameStart].name == "(" || 
                        tokens[specNameStart].name == "{")
                        depth += 1;
                    if (tokens[specNameStart].name == "]" || 
                        tokens[specNameStart].name == ")" || 
                        tokens[specNameStart].name == "}")
                        depth -= 1;
                    if ((tokens[specNameStart].name == ";" || 
                        tokens[specNameStart].name == "final") && depth == 0) {
                        break;
                    }
                    specNameStart -= 1;
                }
                specNameStart += 1;
                bbassertexplain(depth >= 0, 
                          "Unexpected symbol.",
                          "Each `#spec` declaration can only reside in named code blocks. These refer to code blocks being declared and assigned to at least one variable.",
                          Parser::show_position(tokens, i));
            }
            int depth = 1;
            size_t position = i + 2;
            size_t specend = position;
            while (position < tokens.size() - 1) {
                position += 1;
                if (tokens[position].name == "[" || tokens[position].name == 
                    "(" || tokens[position].name == "{")
                    depth += 1;
                if (tokens[position].name == "]" || tokens[position].name == 
                    ")" || tokens[position].name == "}")
                    depth -= 1;
                if (depth == 1 && tokens[position].name == ";" && 
                    specend == i + 2)
                    specend = position;
                if (depth == 0 && (tokens[position].name == ";")) {
                    position += 1;
                    break;
                }
            }
            bbassertexplain(specNameEnd < tokens.size(), 
                        "Unexpected symbol.",
                        "Each `#spec` declaration can only reside in named code blocks. These refer to code blocks being declared and assigned to at least one variable.",
                        Parser::show_position(tokens, i));
            bbassertexplain(depth == 0, "Invalid syntax.", "Imbalanced parantheses or brackets.", Parser::show_position(tokens, i));


            std::vector<Token> newTokens;
            newTokens.emplace_back("final", tokens[i].file, tokens[i].line, false);
            if (specNameEnd == MISSING || specNameEnd < specNameStart) {
                bberrorexplain("Unexpected symbol.", "`#spec` cannot be declared here.", Parser::show_position(tokens, i));
            } 
            else {
                //std::cout<<Parser::to_string(tokens, specNameStart, specNameStart + 1)<<"."<<Parser::to_string(tokens, i + 2, specend)<<"\n";
                newTokens.insert(newTokens.end(), tokens.begin() + specNameStart, tokens.begin() + specNameStart + 1);
                newTokens.emplace_back(".", tokens[i].file, tokens[i].line, false);
            }
            newTokens.insert(newTokens.end(), tokens.begin() + i + 2, tokens.begin() + specend + 1);

            tokens.insert(tokens.begin() + position, newTokens.begin(), newTokens.end());
            tokens.erase(tokens.begin() + i, tokens.begin() + specend + 1);
            i -= 1;
        } 
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "symbol") {
            bbassertexplain(tokens[i+2].name=="(", "Unexpected symbol.",
                "The symbol `!symbol` should be followed by `(`. The only valid syntax is `!symbol(@tokens)`.",
                Parser::show_position(tokens, i));
            size_t pos = i+3;
            std::string created_string;
            int depth = 1;
            while(pos<tokens.size()) {
                if (tokens[pos].name == "(" || tokens[pos].name == "[" || tokens[pos].name == "{")
                    depth += 1;
                else if (tokens[pos].name == ")" || tokens[pos].name == "]" || tokens[pos].name == "}") 
                    depth -= 1;
                if(depth==0 && tokens[pos].name==")")
                    break;
                if(tokens[pos].builtintype==1) // if is string
                    created_string += tokens[pos].name.substr(1, tokens[pos].name.size()-2);
                else
                    created_string += tokens[pos].name;
                ++pos;
            }
            bbassertexplain(depth==0, "Syntax error.", 
                "`!symbol(@tokens)` parenthesis was never closed.",
                Parser::show_position(tokens, i));
            //updatedTokens.emplace_back(created_string, tokens[i].file, tokens[i].line, false);
            //i = pos;
            tokens.erase(tokens.begin()+i, tokens.begin()+pos+1); // +1 to remove the closing parenthesis
            tokens.emplace(tokens.begin()+i, created_string, tokens[i].file, tokens[i].line, true);
            i = i-1;
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "stringify") {
            bbassertexplain(tokens[i+2].name=="(", "Unexpected symbol.",
                "!stringify` should be followed by a parenthesis. The only valid syntax is `!stringify(@tokens)`.",
                Parser::show_position(tokens, i));
            size_t pos = i+3;
            std::string created_string;
            int depth = 1;
            while(pos<tokens.size()) {
                if (tokens[pos].name == "(" || tokens[pos].name == "[" || tokens[pos].name == "{")
                    depth += 1;
                else if (tokens[pos].name == ")" || tokens[pos].name == "]" || tokens[pos].name == "}") 
                    depth -= 1;
                if(depth==0 && tokens[pos].name==")")
                    break;
                //if(created_string.size())
                //    created_string += " ";
                if(tokens[pos].builtintype==1) // if is string
                    created_string += tokens[pos].name.substr(1, tokens[pos].name.size()-2);
                else
                    created_string += tokens[pos].name;
                ++pos;
            }
            created_string = "\""+created_string+"\"";
            bbassertexplain(depth==0, "Invalid syntax.", "`!stringify(@tokens)` parenthesis was never closed.", Parser::show_position(tokens, i));
            //updatedTokens.emplace_back("\""+created_string+"\"", tokens[i].file, tokens[i].line, false);
            //i = pos;
            tokens.erase(tokens.begin()+i, tokens.begin()+pos+1); // +1 to remove the closing parenthesis
            tokens.emplace(tokens.begin()+i, created_string, tokens[i].file, tokens[i].line, true);
            i = i-1;
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && tokens[i + 1].name == "comptime") {
            int starti = i;
            int depth = 0;
            i += 2;
            std::string starter = tokens[i].name;
            //bbassert(tokens[starti+2].name=="(", "`!comptime` should always be followed by a parenthesis\n"+ Parser::show_position(tokens, starti))
            std::string newCode;
            while(i<tokens.size()) {
                if(tokens[i].name=="{") depth++;
                if(tokens[i].name=="}") {depth--;if(depth==0 && starter=="{") {++i;newCode += tokens[i].name+" ";break;}};
                if(tokens[i].name=="(") depth++;
                if(tokens[i].name==")") {depth--;if(depth==0 && starter=="(") {++i;newCode += tokens[i].name+" ";break;}};
                if(depth<0) {--i;break;}
                if(tokens[i].name==";" && depth==0) break;
                newCode += tokens[i].name+" ";
                ++i;
            }
            bbassertexplain(i<tokens.size(), "Invalid syntax.", "`!comptime` never ended (it reached the end of file)", Parser::show_position(tokens, starti));
            bbassertexplain(newCode.size()>1, "Expecting value.", "!`comptime` encloses an empty expression.", Parser::show_position(tokens, starti));
            if(newCode[newCode.size()-2]!='}') newCode += ";";  // skip trailing space with -2 instead of -1
            newCode = compileFromCode(newCode, "!comptime in "+first_source);
            newCode = optimizeFromCode(newCode, true); // always minify at comptime
            newCode = singleThreadedVMForComptime(newCode, first_source);
            
            if(newCode!="#") updatedTokens.emplace_back(newCode, tokens[starti].file, tokens[starti].line, true);
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "include") {
            std::string libpath = tokens[i + 2].name;
            size_t libpathend = i+2;
            // find what is actually being imported (account for #include !stringify(...) statement)
            if(libpath=="#" || libpath=="!") {
                bbassertexplain(i<tokens.size()-5 && tokens[i+3].name=="stringify", "Unexpected symbol.", 
                    "`!include` should be followed by either a string, `!stringify`, or `!comptime`. but another preprocessor instruction follows.",
                    Parser::show_position(tokens, i));
                bbassertexplain(tokens[i+4].name=="(", "Unexpected symbol.",
                    "`"+libpath+tokens[i+3].name+"` should be followed by a parenthesis. The only valid syntax is `!include !stringify(@tokens)` or !include !comptime(@expression)`.",
                    Parser::show_position(tokens, i));
                if(tokens[i+3].name=="stringify") {
                    libpathend = i+5;
                    libpath = "\"";
                    int depth = 1;
                    while(libpathend<tokens.size()) {
                        if (tokens[libpathend].name == "(" || tokens[libpathend].name == "[" || tokens[libpathend].name == "{") depth += 1;
                        else if (tokens[libpathend].name == ")" || tokens[libpathend].name == "]" || tokens[libpathend].name == "}") depth -= 1;
                        if(depth==0 && tokens[libpathend].name==")") break;
                        //if(created_string.size())
                        //    created_string += " ";
                        if(tokens[libpathend].builtintype==1) // if is string
                            libpath += tokens[libpathend].name.substr(1, tokens[libpathend].name.length()-2);
                        else libpath += tokens[libpathend].name;
                        ++libpathend;
                    }
                    libpath += "\"";
                } else {// TODO: add comptime interpretation here
                    
                }
            }
            else if(libpath=="{") {
                int depth = 0;
                std::string here = "!include at "+tokens[libpathend].file[tokens[libpathend].file.size()-1];
                while(libpathend<tokens.size()) {
                    if(tokens[libpathend].name=="{") depth++;
                    if(tokens[libpathend].name=="}") {
                        depth--;
                        if(depth==0) break;
                    }
                    if(depth) tokens[libpathend].file.push_back(here);
                    ++libpathend;
                    if(libpathend==tokens.size()) bberror("Failed to close !include {"+ Parser::show_position(tokens, i+2));
                }
                tokens.erase(tokens.begin()+libpathend);
                if(libpathend<tokens.size() && tokens[libpathend].name==";") tokens.erase(tokens.begin()+libpathend);
                i += 2;
                continue;
            }
            else bbassertexplain(libpath[0] == '"', "Unexpected symbol", "Was expecting. `\"` or `{`. Include statements should enclose code snippets or paths in quotations, like this: `!include \"libname\"`.", Parser::show_position(tokens, i+2));
            bbassertexplain(tokens[libpathend].name != ";", "Unexpected symbol.", " Include statements cannot be followed by `;`.", Parser::show_position(tokens, libpathend));
            
            // delegate .bbvm includes for later, because they need to inject bbvm instructions directly and not as something to be parsed
            std::string source = libpath.substr(1, libpath.size() - 2);
            if (source.size() >= 5 && source.substr(source.size() - 5) == ".bbvm") {
                auto rep_token = tokens[i];
                tokens.erase(tokens.begin() + i, tokens.begin() + libpathend+1);
                updatedTokens.emplace_back("!include", rep_token.file, rep_token.line, rep_token.printable);
                updatedTokens.emplace_back("\""+source+"\"", rep_token.file, rep_token.line, rep_token.printable);
                updatedTokens.emplace_back(";", rep_token.file, rep_token.line, false);
                i -= 1;
                continue;
            }
            // actually handle the include
            std::error_code ec;
            source = normalizeFilePath(source);
            if(std::filesystem::is_directory(source, ec)) source = source+"/.bb";
            else source += ".bb";

            for(int ii=0;ii<tokens[i].file.size()-1;++ii) if(tokens[i].file[ii]==source) {
                std::string circular("");
                /*for(int j=0;j<tokens[i].file.size()-1;++j) {
                    circular += "  \x1B[34m\u2192\033[0m   !include [no longer available]          \x1B[90m" + tokens[i].file[j] + " line "+std::to_string(tokens[i].line[j])+"\n";
                }*/
                bberrorexplain("Circular include.", "Includes can only have a hierarchical structure, but the following chain of dependencies loops back to a previous one.", circular+Parser::show_position(tokens, libpathend));
            }

            /*if (previousImports.find(source) != previousImports.end() && (inclusionDepth!=0 || doNotAllowOtherImports.find(source) != doNotAllowOtherImports.end())) {
                bberrorexplain("Too complicated include: "+source, "You are including this file multiple times, but this is allowed only if all its `!include` statements reside in the top scope, without being enclosed in any brackets. In that case, only the first include takes place.",  Parser::show_position(tokens, libpathend)+"\n"+previousImports.find(source)->second);
            }
            if (previousImports.find(source) != previousImports.end()) {
                tokens.erase(tokens.begin() + i, tokens.begin() + libpathend + 1);
                i -= 1;
                continue;
            }*/

            bbassertexplain(isAllowedLocationNoNorm(source), "Access denied for path: " + source,
                                      "This is a safety measure imposed by Blombly. "
                                      "You need to add read permissions to a location containting the prefix with `!access \"location\"`."
                                      "Permisions can only be granted this way from the virtual machine's entry point. "
                                      "They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.",
                                      Parser::show_position(tokens, libpathend));

            std::ifstream inputFile(source);
            /*std::ifstream inputFile(source);
            if (!inputFile.is_open()) {
                std::filesystem::path execFilePath = std::filesystem::path(std::filesystem::current_path().string()) / source;
                inputFile.open(execFilePath.string());
            }
            if (!inputFile.is_open()) {
                std::filesystem::path execFilePath = std::filesystem::path(blombly_executable_path) / source;
                inputFile.open(execFilePath.string());
            }*/

            if (!inputFile.is_open()) 
                bberrorexplain("Unable to open file: " + source,
                        "This issue makes it impossible to complete the include statement.",
                        Parser::show_position(tokens, i));

            std::string code = "";
            std::string line;
            while (std::getline(inputFile, line)) {
                code += line + "\n";
            }
            inputFile.close();

            std::vector<Token> newTokens = tokenize(code, source);
            for(auto& token : newTokens) {
                token.file.insert(token.file.begin(), tokens[i].file.begin(), tokens[i].file.end());
                token.line.insert(token.line.begin(), tokens[i].line.begin(), tokens[i].line.end());
            }
            sanitize(newTokens);
            
            previousImports[source] = Parser::show_position(tokens, libpathend);
            if(inclusionDepth) doNotAllowOtherImports.insert(source);

            tokens.erase(tokens.begin() + i, tokens.begin() + libpathend+1);
            tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end());

            i -= 1;
        } 
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 4 && 
                   (tokens[i + 1].name == "macro" || tokens[i + 1].name == "local")) {
            bbassertexplain(tokens[i + 2].name == "{", "Invalid macro.", "Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}`.", Parser::show_position(tokens, i));
            int macro_start = i + 2;
            int macro_end = macro_start;
            int depth = 1;
            int decl_end = macro_start;
            for (size_t pos = macro_start+1; pos < tokens.size(); ++pos) {
                if ((tokens[pos].name == "=" || tokens[pos].name == "as") && depth == 0) {
                    bbassertexplain(tokens[pos].name != "=", "Unexpected symbol.", "`=` was used instead of `as`. Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.", Parser::show_position(tokens, pos));
                    bbassertexplain(decl_end == macro_start, "Unexpected symbol", "Macro definition cannot have a second equality symbol. Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.", Parser::show_position(tokens, pos));
                    bbassertexplain(tokens[pos - 2].name == "}" && pos < tokens.size() - 1 && tokens[pos + 1].name == "{", 
                            "Invalid syntax", "Missing `{`. Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.", Parser::show_position(tokens, pos));
                    decl_end = pos;
                } else if (tokens[pos].name == ";" && depth == 0 && decl_end!=macro_start) {
                    macro_end = pos;
                    break;
                } else if (tokens[pos].name == "(" || tokens[pos].name == "[" || tokens[pos].name == "{") {
                    if(decl_end==macro_start) bbassertexplain(depth!=0, "Unexpected symbol.", "Missing `as` after macro `@expression` was closed. Your macro here  should follow the pattern `#"+tokens[i + 1].name+"{@expression} as {@implementation}'.", Parser::show_position(tokens, pos));
                    depth += 1;
                }
                else if (tokens[pos].name == ")" || tokens[pos].name == "]" || tokens[pos].name == "}") {
                    depth -= 1;
                }
                if (depth > 2 && macro_start == decl_end) bberrorexplain("Invalid syntax.", "Cannot nest parentheses or brackets in the expression part of macro definitions.", Parser::show_position(tokens, macro_start));
                bbassertexplain(depth >= 0, "Invalid syntax.", "Parentheses or brackets closed prematurely at macro definition.", Parser::show_position(tokens, macro_start));
            }
            bbassertexplain(depth == 0, "Invalid syntax.", "Imbalanced parentheses or brackets at #"+tokens[i + 1].name+" definition.", Parser::show_position(tokens, macro_start));
            bbassertexplain(macro_end != macro_start, "Invalid syntax.", "Macro was never closed.", Parser::show_position(tokens, macro_start));
            bbassertexplain(decl_end != macro_start, "Invalid syntax.", "Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}`.", Parser::show_position(tokens, macro_start));
            std::shared_ptr<Macro> macro = std::make_shared<Macro>();
            for (int pos = macro_start + 1; pos < decl_end - 2; ++pos) {
                //std::cout << tokens[pos].name << "\n";
                macro->from.push_back(tokens[pos]);
            }
            for (int pos = decl_end + 2; pos < macro_end - 1; ++pos) 
                macro->to.emplace_back(tokens[pos].name, macro->from[0].file, macro->from[0].line, tokens[pos].file, tokens[pos].line, tokens[pos].printable);
            bbassertexplain(macro->from[0].name[0] != '@', "Unexpected symbol.", "The first token of a "+tokens[i + 1].name+"'s @expression cannot be a variable starting with @.\n   \033[33m!!!\033[0m  This restriction facilitates unambiguous parsing.", Parser::show_position(tokens, macro_start));
            macros.insert(macros.begin(), macro);
            if(tokens[i + 1].name == "local") macro->tied_to_file = tokens[i].file[tokens[i].file.size()-1];
            i = macro_end;
        } 
        else {
            bool macro_found = false;
            for (const auto& macro : macros) {
                if (tokens[i].name == macro->from[0].name && (macro->tied_to_file.size()==0 || macro->tied_to_file==tokens[i].file[tokens[i].file.size()-1])) {
                    std::unordered_map<std::string, std::vector<Token>> replacement;
                    bool match = true;
                    size_t j = 0;
                    size_t k = i;
                    int depth = 0;
                    while (j < macro->from.size()) {
                        if (k >= tokens.size()) {
                            match = false;
                            break;
                        }
                        if (macro->from[j].name[0] == '@') {
                            std::string placeholder = macro->from[j].name;
                            if(macro->from[j].name.size()>=2 && macro->from[j].name[1] == '@') {
                                // macro defining macros
                                replacement[placeholder].emplace_back(macro->from[j].name, macro->from[j].file, macro->from[j].line, true);
                            }
                            else
                                while (k < tokens.size() 
                                    && (depth > 0 || tokens[k].name != ";" || tokens[k].name != "}" || tokens[k].name != "]" || tokens[k].name != ")") 
                                    && (j == macro->from.size() - 1  || tokens[k].name != macro->from[j + 1].name || depth > 0)) {
                                    if (tokens[k].name == "(" || tokens[k].name == "[" || tokens[k].name == "{") {
                                        depth += 1;
                                    }
                                    else if (tokens[k].name == ")" || tokens[k].name == "]" || tokens[k].name == "}") {
                                        depth -= 1;
                                        if(depth<0) break;
                                    }
                                    if(depth<0) break;
                                    if(depth==0 && j<macro->from.size()-1 && macro->from[j+1].name[0]!='@' && tokens[k].name==macro->from[j+1].name) break;
                                    if(depth==0 && tokens[k].name == ";" && j>=macro->from.size()-1) break;
                                    replacement[placeholder].push_back(tokens[k]);
                                    k++;
                                    /*if(depth==0) {
                                        break ; // break at the singleton point where we finished nesting
                                    }*/
                                }
                            j++;
                        } 
                        else if (macro->from[j].name != tokens[k].name) {
                            match = false;
                            break;
                        } 
                        else {
                            j++;
                            k++;
                        }
                    }

                    if (match) {
                        j = 0;
                        while (j < macro->to.size()) {
                            if (macro->to[j].name[0] == '@' && replacement.find(macro->to[j].name)==replacement.end()) {
                                if(macro->to[j].name.size()<=1 || macro->to[j].name[1] != '@') replacement[macro->to[j].name].emplace_back(Parser::create_macro_temp(), macro->to[j].file, macro->to[j].line);//, macro->to[j].printable);
                                else replacement[macro->to[j].name].emplace_back(macro->to[j].name.substr(1), macro->to[j].file, macro->to[j].line);
                                /*else
                                    bberror("Macro symbol `"+macro->to[j].name+"` was not defined."
                                            "\n   \033[33m!!!\033[0m This symbol was not a part of the macro's definition."
                                            "\n       To declare a new local variable not found in the declaration, rename"
                                            "\n       the symbol so that it starts with `@@`.\n"
                                            +macro->to[j].toString());*/
                                // TODO: this error message prevents macros in macros
                            }
                            j++;
                        }


                        std::vector<Token> newTokens;
                        for (const auto& token : macro->to) {
                            if (token.name[0] == '@') {
                                for (const auto& rep_token : replacement[token.name])
                                    newTokens.emplace_back(rep_token.name, tokens[i].file, tokens[i].line, rep_token.printable);
                            } 
                            else newTokens.emplace_back(token.name, tokens[i].file, tokens[i].line, token.printable);
                        }
                        tokens.erase(tokens.begin() + i, tokens.begin() + k);

                        if((newTokens[newTokens.size()-1].name==":" || newTokens[newTokens.size()-1].name==";" || newTokens[newTokens.size()-1].name=="}") && i<tokens.size() && tokens[i].name==";")  // must be i, not i+1, and after the erasure
                            tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end()-1);
                        else tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end());

                        //for(auto const& token : newTokens) std::cout << token.name << " ";
                        //std::cout << "\n";
                        i -= 1;
                        macro_found = true;
                        break;
                    }
                }
            }
            if (!macro_found) updatedTokens.push_back(tokens[i]);
        }
    }

    tokens = (updatedTokens);
}


std::string compileFromCode(const std::string& code, const std::string& source_) {
    std::string source = normalizeFilePath(source_);
    if(top_level_file.empty()) top_level_file = source;
    std::vector<Token> tokens = tokenize(code, source, true);
    sanitize(tokens);
    macros(tokens, source);
    Parser parser(tokens);
    parser.parse(0, tokens.size() - 1);
    std::string compiled = parser.get();
    return RESMOVE(compiled);
}


void compile(const std::string& source, const std::string& destination) {
    std::ifstream inputFile(source);
    bbassert(inputFile.is_open(), "Unable to open file: " + source);
    std::string code = "";
    std::string line;
    while (std::getline(inputFile, line)) code += line + "\n";
    inputFile.close();

    std::string compiled = compileFromCode(code, source);

    std::ofstream outputFile(destination);
    bbassert(outputFile.is_open(), "Unable to write to file: " + destination);
    outputFile << compiled;
    outputFile.close();
}


#endif