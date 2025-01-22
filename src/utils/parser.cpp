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

constexpr int MISSING = -1;

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
const std::string PARSER_TRY = "try";
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

extern std::unordered_map<int, DataPtr> cachedData;

std::string singleThreadedVMForComptime(const std::string& code, const std::string& fileName) {
    Future::setMaxThreads(1);
    std::string result;
    bool hadError = false;
    try {
        {
            BMemory memory(nullptr, DEFAULT_LOCAL_EXPECTATION);
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

                ExecutionInstance executor(code, &memory, true);
                Result returnedValue = executor.run(code);
                DataPtr ret = returnedValue.get();
                if (ret.existsAndTypeEquals(FUTURE)) {
                    auto res2 = static_cast<Future*>(ret.get())->getResult();
                    ret = res2.get();
                    returnedValue = res2;
                }

                bbassert(!executor.hasReturned(), "`!comptime` must evaluate to a value but not run a return statement.");
                //bbassert(ret, "`!comptime` must evaluate to a non-missing value.");
                if(ret.isbool()) result = ret.unsafe_tobool()?"true":"false";
                else if (ret.isint() || ret.isfloat()) result = ret.torepr();
                else if(!ret.exists()) result = "#";
                else if (ret->getType() == STRING) result = "\"" + ret->toString(nullptr) + "\"";
                else bberror("`!comptime` must must evaluate to a float, int, str, or bool.");
            } catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        for (const auto& [key, data] : cachedData) data->removeFromOwner();
        cachedData.clear();
        BMemory::verify_noleaks();
    } catch (const BBError& e) {
        for (const auto& [key, data] : cachedData) data->removeFromOwner();
        cachedData.clear();
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }

    if (hadError) std::cerr << "Docs and bug reports: https://maniospas.github.io/Blombly\n";
    return RESMOVE(result);
}



extern void replaceAll(std::string &str, const std::string &from, const std::string &to);

class Parser {
private:
    std::vector<Token> tokens;
    static int tmp_var;
    std::string ret;
    std::string code_block_prepend;
    int find_end(int start, int end, const std::string& end_string, bool missing_error = false) {
        if(tokens[start].name==end_string) return start;
        if(start>=tokens.size() || start>=end) {
            if(missing_error) bberror("Unexpected end of code.\n"+show_position(tokens.size()-1));
            return MISSING;
        }
        int depth = 0;
        std::stack<int> last_open;
        std::stack<std::string> last_open_type;
        int last_open_value = start;
        last_open.push(start);
        for (int i = start; i <= end; ++i) {
            if (depth == 0 && tokens[i].name == end_string) return i;
            if (depth < 0) {
                int pos = last_open_value;
                std::string name = tokens[last_open.top()].name;
                if(name=="(") bberror("Closing `)` is missing.\n"+show_position(last_open.top()));
                if(name=="{") bberror("Closing `}` is missing.\n"+show_position(last_open.top()));
                if(name=="[") bberror("Closing `]` is missing.\n"+show_position(last_open.top()));
                bberror("Did not find ending `"+end_string+"`.\n"+show_position(start));
            }
            std::string name = tokens[i].name;
            if (name == "(" || name == "[" || name == "{") {
                    last_open.push(i);
                    last_open_type.push(tokens[i].name);
                    last_open_value = i;
                    depth += 1;
                }
            if (name == ")" || name == "]" || name == "}") {
                    last_open_value = last_open.top();
                    if(last_open_type.size()>=1) {
                        bbassert(name!=")" || last_open_type.top()=="(", "Closing `)` is missing.\n"+show_position(last_open.top()));
                        bbassert(name!="}" || last_open_type.top()=="{", "Closing `}` is missing.\n"+show_position(last_open.top()));
                        bbassert(name!="]" || last_open_type.top()=="[", "Closing `]` is missing.\n"+show_position(last_open.top()));
                        last_open.pop();
                        last_open_type.pop();
                    }
                    depth -= 1;
                }
        }
        if (missing_error) {
            int pos = depth<0?start:last_open.top();
            if(pos>=tokens.size()) bberror("Unexpected end of code.\n"+show_position(tokens.size()-1));
            if(end_string==";") {
                std::string name = tokens[pos].name;
                if(name=="(") bberror("Closing `)` is missing.\n"+show_position(pos));
                if(name=="{") bberror("Closing `}` is missing.\n"+show_position(pos));
                if(name=="[") bberror("Closing `]` is missing.\n"+show_position(pos));
                bberror("Closing `" + end_string + "` is missing.\n"+show_position(start));
            }
            else bberror("Closing `" + end_string + "` is missing.\n"+show_position(pos));
        }
        return MISSING;
    }
    
    int find_last_end(int start, int end, const std::string& end_string, bool missing_error = false) {
        int depth = 0;
        int pos = MISSING;
        int last_open = end;
        for (int i = start; i <= end; ++i) {
            if (depth == 0 && tokens[i].name == end_string) pos = i;
            if (tokens[i].name == "(" || tokens[i].name == "[" || tokens[i].name == "{") depth += 1;
            if (depth < 0) bberror("Imbalanced parentheses, brackets, or scopes\n"+show_position(last_open));
            if (tokens[i].name == ")" || tokens[i].name == "]" || 
                tokens[i].name == "}") {
                if(depth==0) last_open = i;
                depth -= 1;
            }
        }
        if (missing_error && pos == MISSING)  bberror("Closing " + end_string + " is missing\n"+show_position(last_open));
        return pos;
    }
public:
    const std::string& get() const {
        return ret;
    }
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
            if(prev.size()>1 && tokens[i].name.size()>1)
                expr += " ";
            prev = tokens[i].name;
            expr += prev;
        }
        return expr;
    }

    static std::string to_string(const std::vector<Token>& tokens, int start, int end) {
        std::string expr;
        std::string prev;
        for (int i = start; i <= end; i++) {
            if (!tokens[i].printable) 
                continue;
            if(prev.size()>1 && tokens[i].name.size()>1)
                expr += " ";
            prev = tokens[i].name;
            expr += prev;
        }
        return expr;
    }

    std::string show_position(int pos) const {
        return show_position(tokens, pos);
    }

    static std::string show_position(const std::vector<Token>& tokens, int pos) {
        std::string expr;
        for(int mac=tokens[pos].line.size()-1;mac>=0;--mac) {
            if(expr.size())
                expr += "\n";
            int start = pos;
            int end = pos;
            while(start>0 && (tokens[start-1].has(tokens[pos].line[mac], tokens[pos].file[mac]))) 
                start--;
            while(end<tokens.size()-1 && tokens[end+1].has(tokens[pos].line[mac], tokens[pos].file[mac])) 
                end++;

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
            for (int i = start; i <= end; i++)
                if (tokens[i].printable) {
                    expr += tokens[i].name;
                    if(i<end-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                                && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                 && tokens[i].name!="."&& tokens[i].name!="!")
                        expr += " ";
                }
            expr += " \x1B[90m "+tokens[pos].toString();
            expr += "\n     \x1B[31m ";
            for (int i = start; i < pos; i++)
                if (tokens[i].printable) {
                    for(int k=0;k<tokens[i].name.size();++k)
                        expr += "~";
                    if(i<end-1 && tokens[i+1].name!="(" && tokens[i+1].name!=")" && tokens[i+1].name!="{" && tokens[i+1].name!="}"&& tokens[i+1].name!="[" && tokens[i+1].name!="]"
                                && tokens[i].name!="(" && tokens[i].name!=")" && tokens[i].name!="{" && tokens[i].name!="}"&& tokens[i].name!="[" && tokens[i].name!="]"
                                 && tokens[i].name!="."&& tokens[i].name!="!")
                        expr += "~";
                }
            expr += "^";
            break; // TODO: decide whether to show full replacement stack (also uncomment to debug)
        }
        return expr;
    }

    void breakpoint(int start, int end) {
        if(!debug_info)
            return;
        std::string comm = to_string(start, end);
        if(comm.size()>40)
            comm.resize(40);
        ret += "%"+comm+" //"+tokens[start].file.back()+" line "+std::to_string(tokens[start].line.back())+"\n";
    }

    std::string parse_expression(int start, int end, bool request_block = false, 
                                 bool ignore_empty = false) {
            if (ignore_empty && start >= end)
                return "#";
            if(end>start+1)
                breakpoint(start, end);
        //try {
            bool is_final = tokens[start].name == "final";
            bbassert(start <= end || (request_block && 
                      code_block_prepend.size()), 
                      "Empty expression.\n"+show_position(start));
            bbassert(tokens[start].name != "#", 
                      "Expression cannot start with `#`.\n   \033[33m!!!\033[0m "
                      "This symbol is reserved for preprocessor directives "
                      "and should have been eliminated.\n"+show_position(start));
            if (is_final)
                start += 1;
            bbassert(start <= end, "Empty final expression");
            bbassert(tokens[start].name != "#", 
                      "Expression cannot start with `#`.\n   \033[33m!!!\033[0m "
                      "This symbol is reserved for preprocessor directives "
                      "and should have been eliminated.\n"+show_position(start));

            std::string first_name = tokens[start].name;
            if (start == end) {
                bbassert(code_block_prepend.size() == 0, 
                          "Can only set positional arguments for code blocks."
                          "\n   \033[33m!!!\033[0m Positional arguments were "
                          "declared on an assignment's left-hand-side\n       "
                          "but the right-hand-side is not an explicit code "
                          "block declaration.\n"+show_position(start));
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
                bbassert(!is_final, 
                          "Code block declarations cannot be final (only "
                          "variables holding the blocks can be final)\n"+show_position(start-1));
                std::string requested_var = create_temp();
                ret += "BEGIN " + requested_var + "\n";
                ret += code_block_prepend;
                code_block_prepend = "";
                parse(start + 1, end - 1);  // important to do a full parse
                ret += "END\n";
                return requested_var;
            }
            if (first_name == "(" && find_end(start + 1, end, ")", true) == end) {
                bbassert(!is_final, "Final is only an acceptable qualifier for variables (not parenthesis outcomes)");
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
            bbassert(code_block_prepend.size() == 0, 
                      "Positional arguments were declared on an assignment's "
                      "left-hand-side but the right-hand-side did not evaluate "
                      "to a code block\n"+show_position(start));

            if (first_name == "if" || first_name == "catch" || 
                first_name == "while") {
                int start_parenthesis = find_end(start, end, "(");
                int start_if_body = find_end(start_parenthesis + 1, end, ")", true) + 1;
                int condition_start_in_ret = ret.size();
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
                int body_end = first_name == "while" ? MISSING : 
                               find_end(start_if_body, end, "else");
                if (body_end == MISSING)
                    body_end = end;
                std::string bodyvar = create_temp();
                ret += "BEGIN " + bodyvar + "\n";
                if (body_end==MISSING && find_end(start_if_body, end, "else")!=MISSING) {
                    bbassert(first_name != "while", "`while` expressions cannot have an else branch.\n"+show_position(body_end));
                }
                if (tokens[start_if_body].name == "{") {
                    bbassert(find_end(start_if_body + 1, body_end, "}", true) == body_end || tokens[body_end].name=="else", 
                    "There is leftover code after closing `}`\n"+show_position(body_end));
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
                    bbassert(first_name != "while", "`while` expressions cannot have an else branch.\n"+show_position(body_end));
                    int else_end = end;
                    if (tokens[body_end + 1].name == "{") {
                        else_end = find_end(body_end + 2, end, "}", true);
                        bbassert(else_end == end, "There is leftover code after closing `}` for else\n"+show_position(else_end));
                    }
                    std::string endvar = create_temp();
                    ret += "BEGIN " + endvar + "\n";
                    if (tokens[body_end + 1].name == "{")
                        parse(body_end + 2, else_end - 1);
                    else
                        parse(body_end + 1, else_end);
                    ret += "END\n";
                    bodyvar += " " + endvar;
                    body_end = else_end;
                }
                bbassert(body_end == end, "`"+first_name + "` statement body terminated before end of expression.\n"+show_position(body_end));
                if(first_name!="while") // parse condition last to take advantage of blomblyvm hotpaths
                    condition = parse_expression(start + 1, start_if_body - 1);
                breakpoint(start, end);
                ret += first_name + " # " + condition + " " + bodyvar + "\n";
                return "#";
            }

            if (first_name == "default" || first_name == "defer" || first_name == "try") {
                std::string var = first_name != "try" ? "#" : create_temp();
                std::string called = create_temp();
                std::string parsed = parse_expression(start + 1, end, tokens[start + 1].name != "{");
                if (first_name == "new" && ret.size()>=4 && ret.substr(ret.size() - 4) == "END\n")
                    ret = ret.substr(0, ret.size() - 4) + "return # this\nEND\n";
                bbassert(parsed != "#", "An expression that computes no value was given to `" + first_name+"`\n"+show_position(start+1));
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            if (first_name == "return") {
                std::string parsed = parse_expression(start + 1, end);
                bbassert(parsed != "#", "An expression that computes no value  was given to `" + first_name+"`.\n"+show_position(start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            int assignment = find_end(start, end, "=");
            int asAssignment = find_end(start, end, "as");
            if ((asAssignment < assignment && asAssignment!=MISSING) || assignment==MISSING)
                assignment = asAssignment;

            if (assignment != MISSING) {
                bool negation = first_name=="not";
                if(negation) {
                    bbassert(asAssignment==assignment, "Keyword `not` can precede an `as` but not a `=` assignment (the latter does not return).");
                    start += 1;
                    first_name = tokens[start].name;
                }
                bbassert(assignment != start, "Missing a variable to assign to.\n"+show_position(assignment));
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
                    bbassert(asAssignment!=assignment, "Pattern `"+tokens[assignment-1].name+"as` is not allowed. Use `"+tokens[assignment-1].name+"=` or write the full self-operation\n"+show_position(assignment));
                }
                if(isSelfOperation)
                    bbassert(assignment-1 != start, "Missing a variable to operate and assign to.\n"+show_position(assignment-1));

                std::string returned_value = "#"; // this is set only by start_assigment!=MISSING
                int start_assignment = find_last_end(start, assignment, ".");
                int start_entry = find_last_end((start_assignment == MISSING ? start : start_assignment-isSelfOperation) + 1, assignment-isSelfOperation, "[");
                if (start_entry != MISSING && start_entry > start_assignment) {
                    int end_entry = find_end(start_entry + 1, assignment-isSelfOperation, "]", true);
                    bbassert(end_entry == assignment - 1 - isSelfOperation, "Non-empty expression between last closing `]` and `"+tokens[assignment-isSelfOperation].name+"`.\n"+show_position(end_entry+1));
                    std::string obj = parse_expression(start, start_entry - 1);
                    bbassert(obj != "#", "There is no expression outcome to assign to.\n"+show_position(start));
                    bbassert(!is_final, "Entries cannot be set to final.\n"+show_position(start-1));

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
                        bberror("`as` cannot be used for setting values outside of the current scope. Use only `=` and catch on failure. This prevents leaking missing values."); // proper integration is to have "asset" and "assetfinal" keywords, but I changed my mind: this is unsafe behavior
                        std::string temp = create_temp();
                        ret += "exists " + temp + " " + first_name + "\n";
                        return temp;
                    }
                    return "#";
                }
                if (start_assignment != MISSING) {
                    bbassert(start_assignment >= start + 1-isSelfOperation, "Assignment expression can not start with `.`.\n"+show_position(start));
                    int parenthesis_start = find_end(start_assignment + 1,  assignment - 1-isSelfOperation, "(");
                    std::string obj = parse_expression(start, start_assignment - 1);
                    bbassert(obj != "#", "There is no expression outcome to assign to.\n"+show_position(start)+"\n");
                    returned_value = obj;
                    if (parenthesis_start != MISSING) {
                        code_block_prepend = "";
                        int parenthesis_end = find_end(parenthesis_start + 1, 
                                                       assignment - 1-isSelfOperation, ")", 
                                                       true);
                        bbassert(parenthesis_end == assignment - 1-isSelfOperation, 
                                  "There is leftover code after last "
                                  "parenthesis in assignment's left hand side.\n"+show_position(parenthesis_end));
                        int j = parenthesis_start+1;
                        while(j<parenthesis_end) {
                            int next_j = find_end(j+1, parenthesis_end, ",");
                            if(next_j==MISSING)
                                next_j = parenthesis_end;
                            bbassert(next_j!=j, "Empty argument.\n"+show_position(j));
                            std::string name = tokens[next_j-1].name;
                            code_block_prepend += "next " + name + " args\n";

                            if (name == "bbvm::int" || name == "bbvm::float" || 
                                name == "bbvm::str" || name == "bbvm::file" || 
                                name == "bbvm::bool" ||
                                name == "bbvm::list" || name == "bbvm::map" || 
                                name == "bbvm::move" || name == "bbvm::clear" || name == "bbvm::pop" || name == "bbvm::push" || 
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
                                name == "try" || name == "new" || 
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

                            for(int jj=j;jj<next_j-1;++jj) {
                                std::string semitype = tokens[jj].name;
                                if(semitype=="float" || semitype=="int" || semitype=="str" || semitype=="bool" || semitype=="list" || semitype=="vector" || semitype=="iter"
                                    || semitype=="file" || semitype=="clear" || semitype=="move" 
                                    || semitype=="bbvm::float" || semitype=="bbvm::int" || semitype=="bbvm::str" || semitype=="bbvm::bool" || semitype=="bbvm::iter"
                                    || semitype=="bbvm::list" || semitype=="bbvm::vector" || semitype=="bbvm::file" || semitype=="bbvm::clear" || semitype=="bbvm::move" )
                                    code_block_prepend += semitype+" "+name+" "+name+"\n";
                                else {
                                    std::string args = create_temp();
                                    std::string temp = create_temp();
                                    code_block_prepend += "BEGIN " + args + "\n";
                                    code_block_prepend += "list "+temp+" "+name+"\n";
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
                        bberror("`as` cannot be used for setting values outside of the current scope. Use only `=` and catch on failure. This prevents leaking missing values."); // proper integration is to have "asset" and "assetfinal" keywords, but I changed my mind: this is unsafe behavior
                        std::string temp = create_temp();
                        ret += "exists " + temp + " " + first_name + "\n";
                        //bbassert(returned_value=="#", "Cannot set struct fields with `A.x as y`. Use `A.x = y` instread. To ignore errors instead use `if(y as y) A.x=y;` \n"+show_position(assignment));
                        return temp;
                    }
                    return returned_value;
                }

                int parenthesis_start = find_end(start + 1, assignment - 1-isSelfOperation, "(");
                bbassert(parenthesis_start == MISSING ? assignment == start + 1+isSelfOperation : parenthesis_start == start + 1+isSelfOperation, 
                          "Cannot understrand what to assign to left from the assignment.\n"+show_position(assignment));
                if (first_name == "bbvm::int" || first_name == "bbvm::float" || 
                    first_name == "bbvm::str" || first_name == "bbvm::file" || 
                    first_name == "bbvm::bool" ||
                    first_name == "bbvm::list" || first_name == "bbvm::map" || 
                    first_name == "bbvm::move" || first_name == "bbvm::clear" || first_name == "bbvm::pop" || first_name == "bbvm::push" || 
                    first_name == "bbvm::put" ||
                    first_name == "bbvm::len" || first_name == "bbvm::next" || 
                    first_name == "bbvm::vector" || first_name == "bbvm::iter" || 
                    first_name == "bbvm::add" || first_name == "bbvm::sub" || 
                    first_name == "bbvm::min" || first_name == "bbvm::max" ||  
                    first_name == "bbvm::sum" || 
                    first_name == "bbvm::call" || first_name == "bbvm::range" || 
                    first_name == "bbvm::print" || first_name == "bbvm::read") {
                    bberror("Cannot assign to bbvm implementation `" + first_name + "`.\n"+show_position(start));
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
                    first_name == "try" || first_name == "new" || 
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
                    bberror("Cannot assign to blombly keyword `" + first_name + "`."+
                            "\n   \033[33m!!!\033[0m For safety, all keywords are considered final.\n"
                            + show_position(start));
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
                        int parenthesis_end = find_end(parenthesis_start + 1, 
                                                       assignment - 1-isSelfOperation, ")", 
                                                       true);
                        bbassert(parenthesis_end == assignment - 1-isSelfOperation, 
                                  "There is leftover code after last "
                                  "parenthesis in assignment's left hand side.\n"+show_position(parenthesis_end));
                        int j = parenthesis_start+1;
                        while(j<parenthesis_end) {
                            int next_j = find_end(j+1, parenthesis_end, ",");
                            if(next_j==MISSING)
                                next_j = parenthesis_end;
                            bbassert(next_j!=j, "Empty argument.\n"+show_position(j));
                            std::string name = tokens[next_j-1].name;
                            code_block_prepend += "next " + name + " args\n";

                            if (name == "bbvm::int" || name == "bbvm::float" || 
                                name == "bbvm::str" || name == "bbvm::file" || 
                                name == "bbvm::bool" ||
                                name == "bbvm::list" || name == "bbvm::map" || 
                                name == "bbvm::move" || name == "bbvm::clear" || name == "bbvm::pop" || name == "bbvm::push" || 
                                name == "bbvm::len" || name == "bbvm::next" || 
                                name == "bbvm::vector" || name == "bbvm::iter" || 
                                name == "bbvm::add" || name == "bbvm::sub" || 
                                name == "bbvm::min" || name == "bbvm::max" ||  
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
                                name == "try" || name == "new" || 
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

                            for(int jj=j;jj<next_j-1;++jj) {
                                std::string semitype = tokens[jj].name;
                                if(semitype=="float" || semitype=="int" || semitype=="str" || semitype=="bool" || semitype=="list" || semitype=="vector" || semitype=="iter"
                                    || semitype=="clear" || semitype=="move" || semitype=="file" 
                                    || semitype=="bbvm::float" || semitype=="bbvm::int" || semitype=="bbvm::str" || semitype=="bbvm::bool" || semitype=="bbvm::iter"
                                    || semitype=="bbvm::list" || semitype=="bbvm::vector" || semitype=="bbvm::file"
                                    || semitype=="bbvm::clear" || semitype=="bbvm::move" )
                                    code_block_prepend += semitype+" "+name+" "+name+"\n";
                                else {
                                    std::string args = create_temp();
                                    std::string temp = create_temp();
                                    code_block_prepend += "BEGIN " + args + "\n";
                                    code_block_prepend += "list "+temp+" "+name+"\n";
                                    code_block_prepend += "IS args " + temp + "\n";
                                    code_block_prepend += "END\n";
                                    code_block_prepend += "call "+name+" "+args+" "+semitype+"\n";
                                }
                            }
                            j = next_j+1;
                        }

                }

                if(isSelfOperation) {
                    bbassert(asAssignment!=assignment, "Cannot have a self-operation to. This error should never occur as there is a previous assert. (This is just future-proofing.)");
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
                    bbassert(first_name.size() < 3 || first_name.substr(0, 3) != "_bb", 
                              "_bb variables cannot be made final\n"
                              "   \033[33m!!!\033[0m This is an internal or macro issue.\n"+show_position(start));
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

            bbassert(!is_final, "Only assignments to variables can be final\n" + show_position(start));
            bbassert(tokens[start].name != "#", 
                      "Expression cannot start with `#` here."
                      "\n   \033[33m!!!\033[0m To avoid code smells, you can set metadata"
                      "\n        with `@property = value;` or `final @property = value;`"
                      "\n        only before any other block commands and only"
                      "\n        and immediately assigning a block. Metadata are not inlined.\n"
                      + show_position(start));
            
            if (first_name == "bbvm::print" || first_name == "print") {
                std::string parsed = parse_expression(start + 1, end);
                bbassert(parsed != "#", "An expression that computes no value was given to `" + first_name+"`.\n"+show_position(start+1));
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
                bbassert(parsed != "#", "An expression that computes no value was given to `" + first_name+"`.\n"+show_position(tokens, start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += "fail " + var + " " + parsed + "\n";
                return var;
            }

            if (first_name == "return") {
                std::string parsed = parse_expression(start + 1, end);
                bbassert(parsed != "#", "An expression that computes no value  was given to `" + first_name+"`.\n"+show_position(start+1));
                std::string var = "#";
                breakpoint(start, end);
                ret += first_name + " " + var + " " + parsed + "\n";
                return var;
            }

            int epush = find_last_end(start, end, "<<");
            if (epush != MISSING) {
                std::string var = create_temp();
                ret += "push " + var + " " + parse_expression(start, epush - 1) + " " + parse_expression(epush + 1, end) + "\n";
                return var;
            }

            int listgen = find_end(start, end, ",");
            if (listgen != MISSING) {
                std::string list_vars = parse_expression(start, listgen - 1);
                while (listgen != MISSING) {
                    int next = find_end(listgen + 1, end, ",");
                    if (next == MISSING)
                        list_vars += " " + parse_expression(listgen + 1, end);
                    else
                        list_vars += " " + parse_expression(listgen + 1, next - 1);
                    listgen = next;
                }
                breakpoint(start, end);
                std::string var = create_temp();
                ret += "list " + var + " " + list_vars + "\n";
                return var;
            }

            int eand = find_last_end(start, end, "and");
            int eor = find_last_end(start, end, "or");
            if (eand != MISSING && eand > eor) {
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

            int eq = find_last_end(start, end, "==");
            int ne = find_last_end(start, end, "!=");
            if (eq != MISSING && eq > ne) {
                std::string var = create_temp();
                ret += "eq " + var + " " + parse_expression(start, eq - 1) + " " + parse_expression(eq + 1, end) + "\n";
                return var;
            }
            if (ne != MISSING) {
                std::string var = create_temp();
                ret += "neq " + var + " " + parse_expression(start, ne - 1) + " " + parse_expression(ne + 1, end) + "\n";
                return var;
            }

            int lt = find_last_end(start, end, "<");
            int gt = find_last_end(start, end, ">");
            int le = find_last_end(start, end, "<=");
            int ge = find_last_end(start, end, ">=");
            if (lt != MISSING && lt > gt && lt > ge && lt > le) {
                std::string var = create_temp();
                ret += "lt " + var + " " + parse_expression(start, lt - 1) + " " + parse_expression(lt + 1, end) + "\n";
                return var;
            }
            if (le != MISSING && le > gt && le > ge) {
                std::string var = create_temp();
                ret += "le " + var + " " + parse_expression(start, le - 1) + " " + parse_expression(le + 1, end) + "\n";
                return var;
            }
            if (gt != MISSING && gt > ge) {
                std::string var = create_temp();
                ret += "gt " + var + " " + parse_expression(start, gt - 1) + " " + parse_expression(gt + 1, end) + "\n";
                return var;
            }
            if (ge != MISSING) {
                std::string var = create_temp();
                ret += "ge " + var + " " + parse_expression(start, ge - 1) + " " + parse_expression(ge + 1, end) + "\n";
                return var;
            }

            int add = find_last_end(start, end, "+");
            int sub = find_last_end(start, end, "-");
            if (sub != MISSING && sub > add) {
                std::string var = create_temp();
                ret += "sub " + var + " " + parse_expression(start, sub - 1) + " " + parse_expression(sub + 1, end) + "\n";
                return var;
            }
            if (add != MISSING) {
                std::string var = create_temp();
                ret += "add " + var + " " + parse_expression(start, add - 1) + " " + parse_expression(add + 1, end) + "\n";
                return var;
            }
            int mul = find_last_end(start, end, "*");
            int div = find_last_end(start, end, "/");
            if (div != MISSING && div > mul) {
                std::string var = create_temp();
                ret += "div " + var + " " + parse_expression(start, div - 1) + " " + parse_expression(div + 1, end) + "\n";
                return var;
            }
            if (mul != MISSING) {
                std::string var = create_temp();
                ret += "mul " + var + " " + parse_expression(start, mul - 1) + " " + parse_expression(mul + 1, end) + "\n";
                return var;
            }
            int mod = find_last_end(start, end, "%");
            if (mod != MISSING) {
                std::string var = create_temp();
                ret += "mod " + var + " " + parse_expression(start, mod - 1) + " " + parse_expression(mod + 1, end) + "\n";
                return var;
            }
            int pow = find_last_end(start, end, "^");
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
                bbassert(tokens[start + 1].name == "(", "Missing ( just after `" + first_name+"`.\n"+show_position(start+1));
                bbassert(find_end(start + 2, end, ")") == end, "Leftover code after the last `)` for `" + first_name+"`.\n"+show_position(start+2));
                int separator = find_end(start + 2, end, ",");
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::")
                    first_name = first_name.substr(6);
                std::string temp = create_temp();
                if(separator==MISSING) {
                    auto toret = first_name + " " + temp + " " + parse_expression(start + 2, end - 1) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                    return temp;
                }
                int separator2 = find_end(separator + 1, end, ",");
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
                bbassert(tokens[start + 1].name == "(", "Missing ( just after `" + first_name+"`.\n"+show_position(start+1));
                bbassert(find_end(start + 2, end, ")") == end, "Leftover code after the last `)` for `" + first_name+"`.\n"+show_position(start+2));
                int separator = find_end(start + 2, end, ",");
                bbassert(separator != MISSING, "push requires at least two arguments.\n"+show_position(end));
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::") first_name = first_name.substr(6);
                auto toret = first_name + " # " + parse_expression(start + 2, separator - 1) + " " + parse_expression(separator + 1, end - 1) + "\n";
                breakpoint(start, end);
                ret += toret;
                return "#";
            }

            if (first_name == "bbvm::graphics" || first_name=="graphics") {
                bbassert(tokens[start + 1].name == "(", "Missing ( just after `" + first_name+"`.\n"+show_position(start+1));
                bbassert(find_end(start + 2, end, ")") == end, "Leftover code after the last `)` for `" + first_name+"`.\n"+show_position(start+2));
                int separator = find_end(start + 2, end, ",");
                bbassert(separator != MISSING, "graphics require three arguments.\n"+show_position(end));
                int separator2 = find_end(separator + 2, end, ",");
                bbassert(separator2 != MISSING, "graphics require three arguments.\n"+show_position(end));
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
                first_name == "bbvm::move" || first_name == "bbvm::clear" || first_name == "bbvm::pop" || 
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
                first_name == "clear" || first_name == "move" || first_name == "pop" || 
                first_name == "file" || first_name == "next" || 
                first_name == "list" || first_name == "map" || 
                first_name == "server" || first_name == "sqlite" || first_name == "graphics" || 
                first_name == "vector") {
                bbassert(tokens[start + 1].name == "(", "Missing '(' just after '" + first_name+"'.\n"+show_position(start+1));
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
                bbassert(parsed != "#", "An expression that computes no value was given to `" + first_name + "`.\n"+show_position(start+1));
                std::string var = create_temp();
                if(first_name.size()>=6 && first_name.substr(0, 6)=="bbvm::")
                    first_name = first_name.substr(6);
                auto toret = first_name + " " + var + " " + parsed + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            if (first_name == "bbvm::time" || first_name == "bbvm::random" || first_name == "bbvm::server" || first_name == "bbvm::sqlite" || first_name == "bbvm::list") {
                first_name = first_name.substr(6);
                bbassert(tokens[start + 1].name == "(", "Missing ( after " + first_name);
                if (first_name == "list") {
                    bbassert(tokens[start + 2].name == ")", "`"+first_name + "` accepts no arguments"
                              "\n   \033[33m!!!\033[0m Create lists of more arguments by pushing elements to"
                              "\n        an empty list, or by separating values by commas like this: `l=1,2,3;`.\n"
                              +show_position(start+2));
                } else {
                    bbassert(tokens[start + 2].name == ")", "`"+first_name +"` accepts no arguments.\n"+show_position(start+2));
                }
                std::string var = create_temp();
                ret += first_name + " " + var + "\n";
                return var;
            }


            if (first_name == "time" || first_name == "random" || first_name == "server"  || first_name == "sqlite" || first_name == "list") {
                bbassert(tokens[start + 1].name == "(", "Missing ( after " + first_name);
                if (first_name == "list") {
                    bbassert(tokens[start + 2].name == ")", "`"+first_name + "` accepts no arguments"
                              "\n   \033[33m!!!\033[0m Create lists of more arguments by pushing elements to"
                              "\n        an empty list, or by separating values by commas like this: `l=1,2,3;`.\n"
                              +show_position(start+2));
                } else {
                    bbassert(tokens[start + 2].name == ")", "`"+first_name +"` accepts no arguments.\n"+show_position(start+2));
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
                if (first_name == "new" && ret.substr(ret.size() - 4) == "END\n")
                    ret = ret.substr(0, ret.size() - 4) + "return # this\nEND\n";
                bbassert(parsed != "#", "An expression that computes no value was given to `" + first_name+"`\n"+show_position(start+1));
                auto toret = first_name + " " + var + " " + parsed + "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            
            int chain = find_last_end(start, end, "|");
            if (chain!=MISSING) {
                std::string var = create_temp();
                std::string callable("");
                if(end==chain+1)
                    callable = tokens[chain+1].name;
                if (callable == "int" || callable == "float" || 
                    callable == "str" || callable == "file" || 
                    callable == "bool" ||
                    callable == "list" || callable == "map" || 
                    callable == "move" || callable == "clear" || callable == "pop" || callable == "push" || 
                    callable == "len" || callable == "next" || 
                    callable == "vector" || callable == "iter" || 
                    callable == "add" || callable == "sub" || 
                    callable == "min" || callable == "max" || 
                    callable == "sum" || 
                    callable == "call" || callable == "range" || 
                    callable == "print" || callable == "read" ||
                    callable == "bbvm::int" || callable == "bbvm::float" || 
                    callable == "bbvm::str" || callable == "bbvm::file" || 
                    callable == "bbvm::bool" ||
                    callable == "bbvm::list" || callable == "bbvm::map" || 
                    callable == "bbvm::move" || callable == "bbvm::clear" || callable == "bbvm::pop" || callable == "bbvm::push" || 
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
                    ret += "list "+temp+" "+parse_expression(start, chain - 1)+"\n";
                    ret += "IS args " + temp + "\n";
                    ret += "END\n";
                    auto toret = "call " + var + " " + parsed_args + " " + parse_expression(chain+1, end) + "\n";
                    breakpoint(start, end);
                    ret += toret;
                }
                return var;
            }

            int call = find_last_end(start, end, "(");
            if (call != MISSING && find_end(call + 1, end, ")", true) == end) {
                if (call == start)  // if it's just a redundant parenthesis
                    return parse_expression(start + 1, end - 1);
                std::string var = create_temp();
                if (tokens[call + 1].name == "{")
                    bbassert(find_end(call + 1, end, "}", true) != end - 1,  
                              "Unexpected code block."
                              "\n   \033[33m!!!\033[0m Cannot directly enclose brackets inside a method"
                              "\n        call's parenthesis to avoid code smells. Instead, you can place"
                              "\n        any code inside the parethesis to transfer evaluated content to"
                              "\n        the method. This looks like this: `func(x=1;y=2)`\n"
                              +show_position(call+1));
                int conditional = find_end(call + 1, end, "::"); // call conditional
                std::string parsed_args;
                if (conditional == MISSING) {
                    if (find_end(call + 1, end, "=") != MISSING || find_end(call + 1, end, "as") != MISSING)  // if there are equalities, we are on kwarg mode 
                        parsed_args = parse_expression(call + 1, end - 1, true, true);
                    else if (call + 1 >= end ) {  // if we call with no argument whatsoever
                        parsed_args = "#";
                    } 
                    else if (find_end(call + 1, end, ",") == MISSING && 
                               find_end(call + 1, end, "=") == MISSING && 
                               find_end(call + 1, end, "as") == MISSING && 
                               find_end(call + 1, end, ":") == MISSING && 
                               find_end(call + 1, end, ";") == MISSING) {  // if there is a list of only one element 
                        parsed_args = create_temp();
                        ret += "BEGIN " + parsed_args + "\n";
                        ret += "list args " + parse_expression(call + 1, end - 1) + "\n";
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
                    ret += "list args " + parse_expression(call + 1, conditional - 1) + "\n";
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

            int arrayaccess = find_last_end(start, end, "[");
            if (arrayaccess != MISSING) {
                int arrayend = find_end(arrayaccess + 1, end, "]", true);
                bbassert(arrayend == end, "Array access `]` ended before expression end.\n"+show_position(arrayend));
                std::string var = create_temp();
                auto toret = "at " + var + " " + parse_expression(start, 
                                                            arrayaccess - 1) 
                                                            + " " + 
                                                            parse_expression(
                                                            arrayaccess + 1, 
                                                            arrayend - 1) + 
                                                            "\n";
                breakpoint(start, end);
                ret += toret;
                return var;
            }

            int access = find_last_end(start, end, ".");
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
            bberror("Unknown type of command: `"+to_string(start, end)+"`\n"+show_position(start));
        /*} catch (const BBError& e) {
            if (tokens[start].line != tokens[end].line)
                throw e;
            std::string linestr = to_string(start, end);
            linestr.resize(40, ' ');
            throw BBError(e.what() + ("\n   \x1B[34m\u2192\033[0m " + linestr + " \t\x1B[90m " +tokens[start].toString()+": "+to_string(start,end));
        }*/
    }
    void parse(int start, int end) {
        int statement_start = start;
        //try {
            while (statement_start <= end) {
                int statement_end = find_end(statement_start + 1, end, ";");//, end - start == tokens.size() - 1 && statement_start<end);
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
    for (size_t i = 0; i < tokens.size(); ++i) {
        /*if (tokens[i].name == "{" && i>0 && tokens[i-1].name!="=" && tokens[i-1].name!="as" && tokens[i-1].name!=")") {
            updatedTokens.emplace_back("=", tokens[i].file, tokens[i].line, false);
        }*/

        if (tokens[i].name == "\\") bberror("A stray `\\` was encountered.\n" + Parser::show_position(tokens, i));

        if (tokens[i].name.size() >= 3 && tokens[i].name.substr(0, 3) == "_bb")
            bberror("Variable name `" + tokens[i].name + "` cannot start with _bb."
                    "\n   \033[33m!!!\033[0m Names starting with this prefix are reserved"
                    "\n        for VM local temporaries created by the compiler.\n"
                     + Parser::show_position(tokens, i));
        
        if ((tokens[i].name=="=" || tokens[i].name=="as") && i 
            && (tokens[i-1].name=="|")) { // "this is handled here, other operations by the same parse, allow |as for this specifically"
            int start = i-2;
            while(start>=0) {
                if(tokens[start].name=="final" || tokens[start].name==";")
                    break;
                start -= 1;
                if(tokens[start].name=="(" || tokens[start].name=="{")// || tokens[start].name=="[") 
                    break;
                bbassert(tokens[start].name!="}", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a bracket `}`. Please resort to var = expression assignment.\n"+Parser::show_position(tokens, i));
                bbassert(tokens[start].name!=")", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a parenthesis `)`. Please resort to var = expression assignment.\n"+Parser::show_position(tokens, i));
                //bbassert(tokens[start].name!="]", "For safety, you cannot use `"+tokens[i-1].name+ tokens[i].name +"` when the left-hand-side contains a parenthesis `]`. Please resort to var = expression assignment.\n"+Parser::show_position(tokens, i));
                bbassert(tokens[start].name!="=", "Previous code has not been balanced and there is a `=` before `"+tokens[i-1].name+ tokens[i].name +"`\n"+Parser::show_position(tokens, i));
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

        if ((tokens[i].name == "#" || tokens[i].name == "!") && ((i >= tokens.size() - 1) || 
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
        }
        updatedTokens.push_back(tokens[i]);

        /*if ((tokens[i].name == "if" || tokens[i].name == "while") && ((i==0) || 
            (tokens[i - 1].name != ";" && tokens[i - 1].name != "{" && tokens[i - 1].name != "}" && tokens[i - 1].name != "try" && tokens[i - 1].name != "default")))
            bberror("`"+tokens[i].name+"` statements must be preceded by one of `try`, `default`, '{', '}', or ';'. "
                    "\n   \033[33m!!!\033[0m These statements must be preceded by"
                    "\n       one of the tokens `try`, `default`, '{', '}', or ';'\n"
                    + Parser::show_position(tokens, i));*/
        
        /*if (tokens[i].name == "try" && i > 0 && tokens[i - 1].name == "=")
            bberror("Replace `= try` with `as try`. "
                    "\n   \033[33m!!!\033[0m Try statements may encounter neither exceptions "
                    "\n        nor value returns. As a blanket protection against missing"
                    "\n        value errors other than your error handling, use `as` instead of `=` .\n"
                    + Parser::show_position(tokens, i-1));*/

        if (tokens[i].name == "else" && i > 0 && tokens[i - 1].name == ";")
            bberror("`else` cannot be the next statement after `;`. "
                    "\n   \033[33m!!!\033[0m You may have failed to close brackets"
                    "\n        or are using a bracketless if, which should not have `;`"
                    "\n        after its first statement. \n"
                    + Parser::show_position(tokens, i));
                    
        if (tokens[i].name == ")" && i > 0 && tokens[i - 1].name == ";")
            bberror("The pattern ';)' is not allowed. "
                    "\n   \033[33m!!!\033[0m This error appears so that expressions inside parentheses"
                    "\n        remain as consise as possible while keeping only one viable syntax.\n"
                    + Parser::show_position(tokens, i-1));

        if ((tokens[i].name == "while" || tokens[i].name == "if" || 
             tokens[i].name == "catch") && i < tokens.size() - 1 && 
             tokens[i + 1].name != "(")
            bberror("Invalid `"+tokens[i].name+"` syntax."
                    "\n   \033[33m!!!\033[0m `(` should always follow `" + tokens[i].name + "` but " + tokens[i + 1].name + " encountered.\n" 
                    + Parser::show_position(tokens, i+1));
        if ((tokens[i].name == "new") && i < tokens.size() - 1 && tokens[i + 1].name != "{")
            bberror("Invalid `"+tokens[i].name+"` syntax."
                    "\n   \033[33m!!!\033[0m `{` should always follow `" + tokens[i].name + "` but `" + tokens[i + 1].name + "` encountered."
                    "\n        Since nitializing a struct based on a code block variable and no arguments"
                    "\n        is a code smell, explicitly inline the block this: `" + tokens[i].name + " {block:}`.\n" 
                    + Parser::show_position(tokens, i+1));

        if (tokens[i].name == "}") {
            if (i >= tokens.size() - 1)
                updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
            else if (tokens[i + 1].name == ";")
                bberror("The syntax `};` is invalid."
                        "\n   \033[33m!!!\033[0m Both symbols terminate expressions."
                        "\n       Use only } instead.\n" 
                        + Parser::show_position(tokens, i));
            else if (tokens[i + 1].name == ":")
                bberror("The syntax `}:` is invalid."
                        "\n   \033[33m!!!\033[0m  Inlining a just-declared code block"
                        "\n       is equivalent to running its code immediately."
                        "\n       Maybe you did not mean to add brackets?\n" 
                        + Parser::show_position(tokens, i));
            else if (tokens[i + 1].name != "." && tokens[i + 1].name != ")" && 
                     tokens[i + 1].name != "," && tokens[i + 1].name != "+" && 
                     tokens[i + 1].name != "-" && tokens[i + 1].name != "*" && 
                     tokens[i + 1].name != "/" && tokens[i + 1].name != "^" && 
                     tokens[i + 1].name != "%" && tokens[i + 1].name != "else")// && tokens[i + 1].name != "|")
                updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
        } else if (tokens[i].name == ":") {
            if (i >= tokens.size() - 1)
                updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
            else if (tokens[i + 1].name == ";")
                bberror(":; is an invalid syntax. Use only `:`.\n" + Parser::show_position(tokens, i));
            else
                updatedTokens.emplace_back(";", tokens[i].file, tokens[i].line, false);
        }
    }
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
    std::unordered_set<std::string> previousImports;
    previousImports.insert(first_source);

    for (size_t i = 0; i < tokens.size(); ++i) {
        /*if ((tokens[i].name == "iter" || tokens[i].name == "bbvm::iter") && i>0 && tokens[i-1].name != "as" && tokens[i-1].name != "=" && tokens[i-1].name != ",")
            bberror("`"+tokens[i].name+"` statements must be preceded by one of `=`, `as`, or ','. "
                    "\n   \033[33m!!!\033[0m This way, you may only directly assign to an iterator or add it to a list."
                    "\n       For example, the pattern `A = 1,2,3; while(x as next(bbvm::iter(A))) {}` that leads to an infinite loop is prevented."
                    "\n       You may instead consider the preprocessor #of directive to precompute the contents of a parenthesis"
                    "\n       before the current commend. Here is an example `A = 1,2,3; while(x as next(#of bbvm::iter(A))) {}`.\n"
                    + Parser::show_position(tokens, i));*/
        if(tokens[i].name == "." && i<tokens.size()-1 && tokens[i+1].name==".") {
            bberror("`Unexpected `..`. "
                    "\n   \033[33m!!!\033[0m More than one fullstop can only follow `this` to indicate values"
                    "\n       obtained from its declration closure.\n"
                    + Parser::show_position(tokens, i));
        }
        
        if (tokens[i].name == "this" && i<tokens.size()-2 && tokens[i+1].name=="." && tokens[i+2].name==".") {
                ++i; // skip this
                int originali = i;
                int countWedges = 1;
                ++i;
                while(i<tokens.size()) {
                    if(tokens[i].name==".") ++countWedges; else break;
                    ++i;
                }
                bbassert(i<tokens.size(), "Runaway `.` at end of file.\n"+Parser::show_position(tokens, i));
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
                    if(updatedTokens[pos].name=="}") depth++;
                    if(updatedTokens[pos].name=="{") {if(depth>0)depth--; else hasProgressed=false;}
                    bbassert(pos>=0, 
                            "Closure of `this` specified by a number of `.` that is higher than the actual code recursion."
                            "\n   \033[33m!!!\033[0m  Each `.` declaration can only correspond to structs being created with `new` statements or block declarations."
                            "\n        Here is an example `value=1; a = new{float=>this..value} print(a|float);`.\n"
                            +Parser::show_position(tokens, originali));
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
                        
                    if(updatedTokens[pos].name=="{" && (updatedTokens[pos-1].name=="new" 
                            || updatedTokens[pos-1].name=="=" 
                            || updatedTokens[pos-1].name=="return"
                            || updatedTokens[pos-1].name=="as")) {
                        //bool isClassDefinition = false;
                        bbassert(hasProgressed || !prevInMethod || updatedTokens[pos-1].name=="new", "Attempted to obtain a closure from within a closure.\n"
                                                    +Parser::show_position(tokens, i)
                                                    +"\nHere is where there was an attempt to obtain the closure::\n"+Parser::show_position(updatedTokens, prevInMethod)
                                                    +"\nHere is the parent from which the closure would be obtained (it's not a struct declaration):\n"+Parser::show_position(updatedTokens, pos));
                        if(!alreadyDone) { 
                            /*if(updatedTokens[pos-1].name!="new" && !prevInMethod) {
                                updatedTokens.emplace(updatedTokens.begin()+pos+1, closureName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+2, "=", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+3, "this", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+4, ".", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+5, nextName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+6, ";", tokens[i].file, tokens[i].line, true);
                            }
                            else*/ {
                                //if(tokens[i].name=="this" && countWedges==1) {
                                //    nextName = "."+nextName;
                                //    ++countWedges;
                                //}
                                /*updatedTokens.emplace(updatedTokens.begin()+pos+1, closureName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+2, "=", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+3, nextName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+4, ";", tokens[i].file, tokens[i].line, true);*/
                                updatedTokens.emplace(updatedTokens.begin()+pos+1, closureName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+2, "=", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+3, "this", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+4, ".", tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+5, nextName, tokens[i].file, tokens[i].line, true);
                                updatedTokens.emplace(updatedTokens.begin()+pos+6, ";", tokens[i].file, tokens[i].line, true);
                            }
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
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && (tokens[i + 1].name == "of" || tokens[i + 1].name == "anon")) {
                bbassert(tokens[i - 1].name == "(" || tokens[i - 1].name == "[", 
                          "Unexpected `!of` encountered after `"+tokens[i - 1].name +"`."
                          "\n   \033[33m!!!\033[0m  Each `!of` (or equivalently `!anon`) declaration can only start after a parenthesis or square bracket."
                          "\n        Here is an example `A = 1,2,3; while(x as next(!of bbvm::iter(A))) {}`.\n"
                          +Parser::show_position(tokens, i));
                int iend = i+2;
                int depth = 1;
                while(iend<tokens.size()) {
                    if(tokens[iend].name=="(" || tokens[iend].name=="{" || tokens[iend].name=="[")
                        depth += 1;
                    if(tokens[iend].name==")" || tokens[iend].name=="}" || tokens[iend].name=="]")
                        depth -= 1;
                    if(depth==0)
                        break;
                    iend += 1;
                }
                bbassert(iend<tokens.size() && (tokens[iend].name==")" || tokens[iend].name=="]"), tokens[i - 1].name == "("?"`(!of @code)` or `(!anon @code)` statement was never closed with a right symbol.":"`[!of @code]` or `(!anon @code)` statement was never closed with a right symbol.");
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
            int pos = i+2;
            std::string involved = Parser::show_position(tokens, i);
            while(pos<tokens.size()) {
                if(tokens[pos].name==";" || pos==i+3)
                    break;
                if(tokens[pos].builtintype==1)
                    message += tokens[pos].name.substr(1, tokens[pos].name.size()-2) + " ";
                else
                    message += tokens[pos].name + " ";
                pos++;
                std::string inv = Parser::show_position(tokens, pos);
                if(involved.find(inv)==std::string::npos)
                    involved += "\n"+inv;
            }
            replaceAll(message, "\\n", "\n");
            bberror(message+"\n"+involved);
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && (i < tokens.size()-3) && tokens[i+1].name=="access") {
            bbassert(tokens[i+2].builtintype==1, "`!access` should always be followed by a string\n"+Parser::show_position(tokens, i+2));
            std::string libpath = tokens[i+2].name;    
            std::string source = libpath.substr(1, libpath.size() - 2);
            if(top_level_file!=tokens[i].file.back()) {
                bbassert(isAllowedLocation(source), "Unexpected `!access` for new permissions"
                                            "\n  \033[33m!!!\033[0m This preprocessor directive creates permissions only if encountered at the top-level (aka main)"
                                            "\n      file being parsed. Permissions then passed on to all `!include` and `!comptime` directives, like the one that has"
                                            "\n      just been interrupted. Here, you cannot add new permissions that are not already present."
                                            "\n  \033[33m!!!\033[0m The permissions being requested can be added your main file at an earlier stage after"
                                            "\n      reviewing them. Add either a generalization or the following: `!access "+ tokens[i+2].name+"`"
                                            "\n      Beware that permissions are transferred to your main application if not further modified.\n"+Parser::show_position(tokens, i));

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
                bbassert(isAllowedWriteLocation(source), "Unexpected `!modify` for new permissions"
                                            "\n  \033[33m!!!\033[0m This preprocessor directive creates permissions only if generated at the top-level (aka main)"
                                            "\n      file being parsed. It is then passed on to all `!include` and `!comptime` directives, like the one that has"
                                            "\n      just been interrupted. Here, you cannot add new permissions that are not already present."
                                            "\n  \033[33m!!!\033[0m The permissions being requested can be added your main file at an earlier stage after"
                                            "\n      reviewing them. Add either a generalization or the following: `!modify "+tokens[i+2].name+"`"
                                            "\n      Beware that permissions are transferred to your main application if not further modified.\n"+Parser::show_position(tokens, i));

            }
            else {
                addAllowedWriteLocation(source);
            }
            i += 2;
            continue;
        }

        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 3 && tokens[i + 1].name == "spec") {
            int specNameEnd = MISSING;
            int specNameStart = MISSING;
            if (i > 1) {
                bbassert(tokens[i - 1].name == "{", 
                          "Unexpected `#spec` encountered."
                          "\n   \033[33m!!!\033[0m  Each `#spec` declaration can only reside in named code blocks."
                          "\n        These refer to code blocks being declared and assigned to at least one variable.\n"
                          +Parser::show_position(tokens, i));
                bbassert(tokens[i - 2].name == "=" || tokens[i - 2].name == "as", 
                          "Invalid `#spec` syntax."
                          "\n   \033[33m!!!\033[0m  Each `#spec` declaration can only reside in named code blocks."
                          "\n        These refer to code blocks being declared and assigned to at least one variable.\n"
                          +Parser::show_position(tokens, i));
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
                bbassert(depth == 0, 
                          "Unexpected `#spec` encountered."
                          "\n   \033[33m!!!\033[0m  Each `#spec` declaration can only reside in named code blocks."
                          "\n        These refer to code blocks being declared and assigned to at least one variable.\n"
                          +Parser::show_position(tokens, i));
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
                bbassert(depth >= 0, 
                          "Unexpected `#spec` encountered."
                          "\n   \033[33m!!!\033[0m  Each `#spec` declaration can only reside in named code blocks."
                          "\n        These refer to code blocks being declared and assigned to at least one variable.\n"
                          +Parser::show_position(tokens, i));
            }
            int depth = 1;
            int position = i + 2;
            int specend = position;
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
            bbassert(specNameEnd < tokens.size(), 
                        "Unexpected `#spec` encountered."
                          "\n   \033[33m!!!\033[0m  Each `#spec` declaration can only reside in named code blocks."
                          "\n        These refer to code blocks being declared and assigned to at least one variable.\n"
                        +Parser::show_position(tokens, i));
            bbassert(depth == 0, "Imbalanced parantheses or brackets.\n" + Parser::show_position(tokens, i));


            std::vector<Token> newTokens;
            newTokens.emplace_back("final", tokens[i].file, tokens[i].line, false);
            if (specNameEnd == MISSING || specNameEnd < specNameStart) {
                bberror("`#spec` cannot be declared here.\n" + Parser::show_position(tokens, i));
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
        }  else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "symbol") {
            bbassert(tokens[i+2].name=="(", "Missing `(`."
            "\n   \033[33m!!!\033[0m  `!symbol` should be followed by a parenthesis. "
            "\n       The only valid syntax is `!symbol(@tokens)`.\n"
            + Parser::show_position(tokens, i));
            int pos = i+3;
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
            bbassert(depth==0, "Missing `)`."
                                "\n   \033[33m!!!\033[0m  `!symbol(@tokens)` parenthesis was never closed.\n"
                                + Parser::show_position(tokens, i));
            //updatedTokens.emplace_back(created_string, tokens[i].file, tokens[i].line, false);
            //i = pos;
            tokens.erase(tokens.begin()+i, tokens.begin()+pos+1); // +1 to remove the closing parenthesis
            tokens.emplace(tokens.begin()+i, created_string, tokens[i].file, tokens[i].line, true);
            i = i-1;
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "stringify") {
            bbassert(tokens[i+2].name=="(", "Missing `(`."
            "\n   \033[33m!!!\033[0m  `!stringify` should be followed by a parenthesis. "
            "\n       The only valid syntax is `!stringify(@tokens)`.\n"
            + Parser::show_position(tokens, i));
            int pos = i+3;
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
            bbassert(depth==0, "Missing `)`."
                                "\n   \033[33m!!!\033[0m  `!stringify(@tokens)` parenthesis was never closed.\n"
                                + Parser::show_position(tokens, i));
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
            bbassert(i<tokens.size(), "`!comptime` never ended (it reached the end of file)\n" + Parser::show_position(tokens, starti));
            bbassert(newCode.size()>1, "!`comptime` encloses an empty expression\n" + Parser::show_position(tokens, starti));
            if(newCode[newCode.size()-2]!='}') newCode += ";";  // skip trailing space with -2 instead of -1
            newCode = compileFromCode(newCode, "!comptime in "+first_source);
            newCode = optimizeFromCode(newCode, true); // always minify at comptime
            newCode = singleThreadedVMForComptime(newCode, first_source);
            
            if(newCode!="#") updatedTokens.emplace_back(newCode, tokens[starti].file, tokens[starti].line, true);
        }
        else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 2 && tokens[i + 1].name == "include") {
            std::string libpath = tokens[i + 2].name;
            int libpathend = i+2;
            // find what is actually being imported (account for #include !stringify(...) statement)
            if(libpath=="#" || libpath=="!") {
                bbassert(i<tokens.size()-5 && tokens[i+3].name=="stringify", "Missing `stringify`."
                    "\n   \033[33m!!!\033[0m  `!include` should be followed by either a string, `!stringify`, or `!comptime`. "
                    "\n       but another preprocessor isntruction follows `#`.\n"
                    + Parser::show_position(tokens, i));
                bbassert(tokens[i+4].name=="(", "Missing `(`."
                    "\n   \033[33m!!!\033[0m  `!"+tokens[i+3].name+"` should be followed by a parenthesis. "
                    "\n       The only valid syntax is `!include !stringify(@tokens)` or !include !comptime(@expression)`.\n"
                + Parser::show_position(tokens, i));
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
            else 
                bbassert(libpath[0] == '"', 
                        "Invalid `!include` syntax."
                        "\n   \033[33m!!!\033[0m  Include statements should enclose paths in quotations, like this: `#include \"libname\"`.\n" 
                        + Parser::show_position(tokens, i+2));
            bbassert(tokens[libpathend].name != ";", 
                      "Unexpected `;` encountered."
                      "\n   \033[33m!!!\033[0m  Include statements cannot be followed by `;`.\n" 
                      + Parser::show_position(tokens, libpathend));

            // actually handle the import
            std::string source = libpath.substr(1, libpath.size() - 2);
            std::error_code ec;
            source = normalizeFilePath(source);
            if(std::filesystem::is_directory(source, ec))
                source = source+"/.bb";
            else 
                source += ".bb";

            if (previousImports.find(source) != previousImports.end()) {
                tokens.erase(tokens.begin() + i, tokens.begin() + libpathend + 1);
                i -= 1;
                continue;
            }

            bbassert(isAllowedLocationNoNorm(source), "Access denied for path: " + source +
                                      "\n   \033[33m!!!\033[0m This is a safety measure imposed by Blombly."
                                      "\n       You need to add read permissions to a location containting the prefix with `!access \"location\"`."
                                      "\n       Permisions can only be granted this way from the virtual machine's entry point."
                                      "\n       They transfer to all subsequent running code as well as to all following `!comptime` preprocessing.\n"
                                      + Parser::show_position(tokens, libpathend));

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
                bberror("Unable to open file: " + source +
                        "\n   \033[33m!!!\033[0m  This issue makes it impossible to complete the include statement.\n"
                        + Parser::show_position(tokens, i));

            std::string code = "";
            std::string line;
            while (std::getline(inputFile, line)) {
                code += line + "\n";
            }
            inputFile.close();

            std::vector<Token> newTokens = tokenize(code, source);
            sanitize(newTokens);

            previousImports.insert(source);

            tokens.erase(tokens.begin() + i, tokens.begin() + libpathend+1);
            tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end());

            i -= 1;
        } else if ((tokens[i].name == "#" || tokens[i].name == "!") && i < tokens.size() - 4 && 
                   (tokens[i + 1].name == "macro" || tokens[i + 1].name == "local")) {
            bbassert(tokens[i + 2].name == "{", "Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}`.\n"+Parser::show_position(tokens, i));
            int macro_start = i + 2;
            int macro_end = macro_start;
            int depth = 1;
            int decl_end = macro_start;
            for (size_t pos = macro_start+1; pos < tokens.size(); ++pos) {
                if ((tokens[pos].name == "=" || tokens[pos].name == "as") && depth == 0) {
                    bbassert(tokens[pos].name != "=", "`=` was used instead of `as`.\n   \033[33m!!!\033[0m  Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.\n"+Parser::show_position(tokens, pos));
                    bbassert(decl_end == macro_start, "Macro definition cannot have a second equality symbol.\n   \033[33m!!!\033[0m  Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.\n"+Parser::show_position(tokens, pos));
                    bbassert(tokens[pos - 2].name == "}" && pos < tokens.size() - 1 && tokens[pos + 1].name == "{", 
                             "Missing `{`.\n   \033[33m!!!\033[0m  Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}'.\n"+Parser::show_position(tokens, pos));
                    decl_end = pos;
                } else if (tokens[pos].name == ";" && depth == 0 && decl_end!=macro_start) {
                    macro_end = pos;
                    break;
                } else if (tokens[pos].name == "(" || tokens[pos].name == "[" || 
                           tokens[pos].name == "{") {
                    if(decl_end==macro_start)
                        bbassert(depth!=0, "Missing `as` after macro `@expression` was closed.\n   \033[33m!!!\033[0m  Your macro here  should follow the pattern `#"+tokens[i + 1].name+"{@expression} as {@implementation}'.\n"+Parser::show_position(tokens, pos));
                    depth += 1;
                }
                else if (tokens[pos].name == ")" || tokens[pos].name == "]" || 
                         tokens[pos].name == "}") {
                    depth -= 1;
                }
                if (depth > 2 && macro_start == decl_end)
                    bberror("Cannot nest parentheses or brackets in the expression part of macro definitions.\n" + Parser::show_position(tokens, macro_start));
                bbassert(depth >= 0, "Parentheses or brackets closed prematurely at macro definition.\n" + Parser::show_position(tokens, macro_start));
            }
            bbassert(depth == 0, "Imbalanced parentheses or brackets at #"+tokens[i + 1].name+" definition.\n" + Parser::show_position(tokens, macro_start));
            bbassert(macro_end != macro_start, "Macro was never closed.\n" + Parser::show_position(tokens, macro_start));
            bbassert(decl_end != macro_start, "Your macro here should follow the pattern `#"+tokens[i + 1].name+" {@expression} as {@implementation}`. \n"
                      + Parser::show_position(tokens, macro_start));
            std::shared_ptr<Macro> macro = std::make_shared<Macro>();
            for (int pos = macro_start + 1; pos < decl_end - 2; ++pos) {
                //std::cout << tokens[pos].name << "\n";
                macro->from.push_back(tokens[pos]);
            }
            for (int pos = decl_end + 2; pos < macro_end - 1; ++pos) 
                macro->to.emplace_back(tokens[pos].name, macro->from[0].file, macro->from[0].line, tokens[pos].file, tokens[pos].line, tokens[pos].printable);
            bbassert(macro->from[0].name[0] != '@', "The first token of a "+tokens[i + 1].name+"'s @expression cannot be a variable starting with @.\n   \033[33m!!!\033[0m  This restriction facilitates unambiguous parsing.\n"
                      + Parser::show_position(tokens, macro_start));
            macros.insert(macros.begin(), macro);
            if(tokens[i + 1].name == "local")
                macro->tied_to_file = tokens[i].file[tokens[i].file.size()-1];
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
                                replacement[placeholder].emplace_back(macro->from[j].name.substr(1), macro->from[j].file, macro->from[j].line, true);
                            }
                            else
                                while (k < tokens.size() && (depth > 0 || tokens[k].name != ";" 
                                    || tokens[k].name != "}" || tokens[k].name != "]" 
                                    || tokens[k].name != ")") && (j == macro->from.size() - 1 
                                    || (tokens[k].name != macro->from[j + 1].name || depth > 0))) {
                                    if (tokens[k].name == "(" || tokens[k].name == "[" || 
                                        tokens[k].name == "{") {
                                        depth += 1;
                                    } else if (tokens[k].name == ")" || tokens[k].name == "]" || 
                                            tokens[k].name == "}") {
                                        depth -= 1;
                                    }
                                    if(depth<0) {
                                        break; // break off
                                    }
                                    replacement[placeholder].push_back(tokens[k]);
                                    k++;
                                    /*if(depth==0) {
                                        break ; // break at the singleton point where we finished nesting
                                    }*/
                                }
                            j++;
                        } else if (macro->from[j].name != tokens[k].name) {
                            match = false;
                            break;
                        } else {
                            j++;
                            k++;
                        }
                    }

                    if (match) {
                        j = 0;
                        while (j < macro->to.size()) {
                            if (macro->to[j].name[0] == '@' && replacement.find(macro->to[j].name)==replacement.end()) {
                                //if(macro->to[j].name.size()>1 && macro->to[j].name[1] == '@')
                                replacement[macro->to[j].name].emplace_back(Parser::create_macro_temp(), macro->to[j].file, macro->to[j].line);//, macro->to[j].printable);
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
                                    newTokens.emplace_back(rep_token.name, tokens[i].file, tokens[i].line, rep_token.file, rep_token.line, rep_token.printable);
                            } else {
                                newTokens.emplace_back(token.name, tokens[i].file, tokens[i].line, token.file, token.line, token.printable);
                            }
                        }
                        tokens.erase(tokens.begin() + i, tokens.begin() + k);

                        if((newTokens[newTokens.size()-1].name==":" || newTokens[newTokens.size()-1].name==";" || newTokens[newTokens.size()-1].name=="}") && i<tokens.size() && tokens[i].name==";")  // must be i, not i+1, and after the erasure
                            tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end()-1);
                        else
                            tokens.insert(tokens.begin() + i, newTokens.begin(), newTokens.end());
                        i -= 1;
                        macro_found = true;
                        break;
                    }
                }
            }
            if (!macro_found) {
                updatedTokens.push_back(tokens[i]);
            }
        }
    }

    tokens = (updatedTokens);
}


std::string compileFromCode(const std::string& code, const std::string& source) {
    if(top_level_file.empty()) top_level_file = source;
    std::vector<Token> tokens = tokenize(code, source);
    sanitize(tokens);
    macros(tokens, source);
    Parser parser(tokens);
    parser.parse(0, tokens.size() - 1);
    std::string compiled = parser.get();
    return std::move(compiled);
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