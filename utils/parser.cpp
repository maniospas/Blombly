#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include "stringtrim.cpp"
#include "tokenizer.cpp"
#include "common.h"
#define MISSING -1


const std::string PARSER_BUILTIN="BUILTIN";
const std::string PARSER_BEGIN="BEGIN";
const std::string PARSER_BEGINFINAL="BEGINFINAL";
const std::string PARSER_END="END";
const std::string PARSER_RETURN="return";
const std::string PARSER_FINAL="final";
const std::string PARSER_AT="at";
const std::string PARSER_IS="IS";
const std::string PARSER_CALL="call";
const std::string PARSER_GET="get";
const std::string PARSER_SET="set";
const std::string PARSER_PUT="put";
const std::string PARSER_WHILE="while";
const std::string PARSER_IF="if";
const std::string PARSER_INLINE="inline";
const std::string PARSER_TRY="try";
const std::string PARSER_DEFAULT="default";
const std::string PARSER_NEW="new";
const std::string PARSER_PRINT="print";
const std::string PARSER_COPY="copy";
const std::string ANON = "_bb";

class Parser {
private:
    std::vector<Token> tokens;
    int tmp_var;
    std::string ret;
    int find_end(int start, int end, const std::string& end_string, bool missing_error=false) {
        int depth = 0;
        for(int i=start;i<=end;++i) {
            if(depth==0 && tokens[i].name==end_string)
                return i;
            if(tokens[i].name=="(" || tokens[i].name=="[" || tokens[i].name=="{")
                depth += 1;
            if(depth<0)
                bberror("Imbalanced parantheses, brackets, or scopes");
            if(tokens[i].name==")" || tokens[i].name=="]" || tokens[i].name=="}")
                depth -= 1;
        }
        if(missing_error) {
            bberror("Closing "+end_string+" is missing");
        }
        return MISSING;
    }
    int find_last_end(int start, int end, const std::string& end_string, bool missing_error=false) {
        int depth = 0;
        int pos = MISSING;
        for(int i=start;i<=end;++i) {
            if(depth==0 && tokens[i].name==end_string)
                pos = i;
            if(tokens[i].name=="(" || tokens[i].name=="[" || tokens[i].name=="{")
                depth += 1;
            if(depth<0)
                bberror("Imbalanced parantheses, brackets, or scopes starting from line "+std::to_string(tokens[start].line));
            if(tokens[i].name==")" || tokens[i].name=="]" || tokens[i].name=="}")
                depth -= 1;
        }
        if(missing_error && pos==MISSING) {
            bberror("Closing "+end_string+" is missing starting from line "+std::to_string(tokens[start].line));
        }
        return pos;
    }
public:
    const std::string& get() const {
        return ret;
    }
    Parser(const std::vector<Token>& tokens): tokens(tokens), tmp_var(0) {}
    std::string create_temp() {
        std::string ret = "_bb"+std::to_string(tmp_var);
        tmp_var += 1;
        return ret;
    }

    std::string to_string(int start, int end) const {
        std::string expr;
        for(int i=start;i<=end;i++)
            if(tokens[i].printable)
                expr += tokens[i].name + " ";
        return expr;
    }
    
    /*std::string parse_lhs(int start, int end) {
        if(start==end)
            return tokens[start].name;
        
    }*/

    std::string parse_expression(int start, int end, bool request_block=false) {
        try{
        // check if final or empty expression
        bool is_final = tokens[start].name=="final";
        bbassert(start<=end, "Empty expression");
        bbassert(tokens[start].name!="#", "Expression cannot start with `#` here\n    because this is not the top-level expression of a code block.");
        if(is_final)
            start += 1;
        bbassert(start<=end, "Empty final expression");
        bbassert(tokens[start].name!="#", "Expression cannot start with `#` here\n    because this is not the top-level expression of a code block.");

        // expresion parsing that is basically just a variable name
        std::string first_name = tokens[start].name;
        if(start==end) {
            int type = tokens[start].builtintype;
            if(type) {
                std::string var = create_temp();
                if(type==1)
                    ret += PARSER_BUILTIN+" "+var+" "+first_name+"\n";
                if(type==2)
                    ret += PARSER_BUILTIN+" "+var+" B"+first_name+"\n";
                if(type==3)
                    ret += PARSER_BUILTIN+" "+var+" I"+first_name+"\n";
                if(type==4)
                    ret += PARSER_BUILTIN+" "+var+" F"+first_name+"\n";
                return var;
            }
            return first_name;
        }

        // code block declaration
        if(request_block) {
            std::string requested_var = create_temp();
            ret += "BEGIN "+requested_var+"\n";
            parse(start, end);  // important to do a full parse
            ret += "END\n";
            return requested_var;
        }
        if(first_name=="{" && find_end(start+1, end, "}", true)==end) {
            bbassert(!is_final, "Code block declarations cannot be final (only variables holding the blocks can be final)");
            std::string requested_var = create_temp();
            ret += "BEGIN "+requested_var+"\n";
            parse(start+1, end-1);  // important to do a full parse
            ret += "END\n";
            return requested_var;
        }

        // assignment
        int assignment = find_end(start, end, "=");
        if(assignment!=MISSING) {
            bbassert(assignment!=start, "Missing variable to assign to");
            int start_assignment = find_last_end(start, assignment, ".");
            if(start_assignment!=-1) {
                bbassert(start_assignment>=start+1, "Assignment expression can not start with `.`.");
                std::string obj = parse_expression(start, start_assignment-1);
                bbassert(obj!="#", "Empty expression before last get membership");
                ret += (is_final?"setfinal # ":"set # ")+obj+" "+parse_expression(start_assignment+1, assignment-1)+" "+parse_expression(assignment+1, end)+"\n";
                return "#";
            }   
            bbassert(assignment==start+1, "Can only assign to one variable");
            // TODO: do not allow assignment to keywords
            ret += PARSER_IS+" "+first_name+" "+parse_expression(assignment+1,end)+"\n";
            if(is_final)
                ret += "final # "+first_name+"\n";
            return "#";
        }
        bbassert(!is_final, "Only assignments to variables can be final");
        bbassert(tokens[start].name!="#", "Only assignments can start with `#`\n    because this sets code block properties after its declaration.");


        if(first_name=="print" 
            || first_name=="return" 
            || first_name=="len" 
            || first_name=="iter"
            || first_name=="int"
            || first_name=="float"
            || first_name=="str"
            || first_name=="bool"
            || first_name=="push"
            || first_name=="pop"
            || first_name=="File"
            //|| first_name=="List"
            || first_name=="Vec") {
            //if(first_name!="return")
            //    bbassert(tokens[start+1].name=="(", "Missing ( just after "+first_name);
            std::string parsed = parse_expression(start+1, end);
            bbassert(parsed!="#", "An expression that computes no value was given to "+first_name);
            std::string var = (first_name=="print" || first_name=="return")?"#":create_temp();
            ret += first_name+" "+var+" "+parsed+"\n";
            return var;
        }

        // logical statements
        int eand = find_last_end(start, end, "and");
        int eor = find_last_end(start, end, "or");
        if(eand!=MISSING && eand>eor) {
            std::string var = create_temp();
            ret += "and "+var+" "+parse_expression(start, eand-1)+" "+parse_expression(eand+1, end)+"\n";
            return var;
        }
        if(eor!=MISSING) {
            std::string var = create_temp();
            ret += "or "+var+" "+parse_expression(start, eor-1)+" "+parse_expression(eor+1, end)+"\n";
            return var;
        }

        // equalities
        int eq = find_last_end(start, end, "==");
        int ne = find_last_end(start, end, "!=");
        if(eq!=MISSING && eq>ne) {
            std::string var = create_temp();
            ret += "eq "+var+" "+parse_expression(start, eq-1)+" "+parse_expression(eq+1, end)+"\n";
            return var;
        }
        if(ne!=MISSING) {
            std::string var = create_temp();
            ret += "neq "+var+" "+parse_expression(start, ne-1)+" "+parse_expression(ne+1, end)+"\n";
            return var;
        }
        
        // inequalities
        int lt = find_last_end(start, end, "<");
        int gt = find_last_end(start, end, ">");
        int le = find_last_end(start, end, "<=");
        int ge = find_last_end(start, end, ">=");
        if(lt!=MISSING && lt>gt && lt>ge && lt>le) {
            std::string var = create_temp();
            ret += "lt "+var+" "+parse_expression(start, lt-1)+" "+parse_expression(lt+1, end)+"\n";
            return var;
        }
        if(le!=MISSING && le>gt && le>ge) {
            std::string var = create_temp();
            ret += "le "+var+" "+parse_expression(start, le-1)+" "+parse_expression(le+1, end)+"\n";
            return var;
        }
        if(gt!=MISSING && gt>ge) {
            std::string var = create_temp();
            ret += "gt "+var+" "+parse_expression(start, gt-1)+" "+parse_expression(gt+1, end)+"\n";
            return var;
        }
        if(ge!=MISSING) {
            std::string var = create_temp();
            ret += "ge "+var+" "+parse_expression(start, ge-1)+" "+parse_expression(ge+1, end)+"\n";
            return var;
        }
        
        // arithmetic operations
        int add = find_last_end(start, end, "+");
        int sub = find_last_end(start, end, "-");
        if(sub!=MISSING && sub>add) {
            std::string var = create_temp();
            ret += "sub "+var+" "+parse_expression(start, sub-1)+" "+parse_expression(sub+1, end)+"\n";
            return var;
        }
        if(add!=MISSING) {
            std::string var = create_temp();
            ret += "add "+var+" "+parse_expression(start, add-1)+" "+parse_expression(add+1, end)+"\n";
            return var;
        }
        int mul = find_last_end(start, end, "*");
        int div = find_last_end(start, end, "/");
        if(div!=MISSING && div>mul) {
            std::string var = create_temp();
            ret += "div "+var+" "+parse_expression(start, div-1)+" "+parse_expression(div+1, end)+"\n";
            return var;
        }
        if(mul!=MISSING) {
            std::string var = create_temp();
            ret += "mul "+var+" "+parse_expression(start, mul-1)+" "+parse_expression(mul+1, end)+"\n";
            return var;
        }
        int mod = find_last_end(start, end, "%");
        if(mod!=MISSING) {
            std::string var = create_temp();
            ret += "mod "+var+" "+parse_expression(start, mod-1)+" "+parse_expression(mod+1, end)+"\n";
            return var;
        }
        int pow = find_last_end(start, end, "^");
        if(pow!=MISSING) {
            std::string var = create_temp();
            ret += "pow "+var+" "+parse_expression(start, pow-1)+" "+parse_expression(pow+1, end)+"\n";
            return var;
        }

        // 
        if(first_name=="new" || first_name=="default" || first_name=="try") {
            std::string var = first_name=="default"?"#":create_temp();
            std::string called = create_temp();
            std::string parsed = parse_expression(start+1, end);
            if(first_name=="new" && ret.substr(ret.size()-4)=="END\n")
                ret = ret.substr(0, ret.size()-4)+"return # this\nEND\n";  // new should return this by default 
            bbassert(parsed!="#", "An expression that computes no value was given to "+first_name);
            ret += first_name+" "+var+" "+parsed+"\n";
            return var;
        }

        if(tokens[end].name==":") {
            std::string var = create_temp();
            ret += "inline "+var+" "+parse_expression(start, end-1)+"\n";
            return var;
        }

        int call = find_last_end(start, end, "(");
        if(call!=MISSING && find_end(call+1, end, ")", true)==end) {
            //bbassert(find_end(start+1, end, ")")==end, "Imbalanced method call parenthesis at line "+std::to_string(tokens[start].line));
            if(call==start) // if it's just a redundant parenthesis
                return parse_expression(start+1, end-1);
            std::string var = create_temp();
            ret += "call "+var+" "+(call+1<end-1?parse_expression(call+1, end-1):"#")+" "+parse_expression(start, call-1)+"\n";
            return var;
        }
        

        int access = find_last_end(start, end, ".");
        if(access!=MISSING) {
            std::string var = create_temp();
            ret += "get "+var+" "+parse_expression(start, access-1)+" "+parse_expression(access+1, end)+"\n";
            return var;
        }

        return "#";
        //bberror("Unknown type of command at line "+std::to_string(tokens[start].line));
        }
        catch(const BBError& e) {
            if(tokens[start].line!=tokens[end].line)
                throw e;
            std::string linestr = to_string(start, end);
            linestr.resize(40, ' ');
            throw BBError(e.what()+(u8"\n   \x1B[34m\u2192\033[0m "+linestr+" \t\x1B[90m "+"at"+" line "+std::to_string(tokens[start].line)));
        }
    }
    void parse(int start, int end) {
        int statement_start = start;
        try {
            while(statement_start<=end) {
                int statement_end = find_end(statement_start+1, end, ";", end-start==tokens.size()-1);
                if(statement_end==MISSING)
                    statement_end = end+1;
                parse_expression(statement_start, statement_end-1);
                statement_start = statement_end + 1;
            }
        }
        catch(const BBError& e) {
            std::string linestr = to_string(start, end);
            linestr.resize(40, ' ');
            throw BBError(e.what()+(u8"\n   \x1B[34m\u2192\033[0m "+linestr+" \t\x1B[90m "+"at"+" line "+std::to_string(tokens[start].line)));
        }
    }

};

void sanitize(std::vector<Token>& tokens) {
    std::vector<Token> updatedTokens;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if(tokens[i].name.size()>=3 && tokens[i].name.substr(0, 3)=="_bb")
            bberror("Variable name "+tokens[i].name+" cannot start with _bb, as this is reserved for VM local temporaries.");
        if(tokens[i].builtintype==3 && i<tokens.size()-2 && tokens[i+1].name=="." && tokens[i+2].builtintype==3) {
            tokens[i].name += "."+tokens[i+2].name;
            tokens[i].builtintype = 4;
            updatedTokens.push_back(tokens[i]);
            i += 2;
            continue;
        }
        if((tokens[i].name=="<" || tokens[i].name==">" || tokens[i].name=="=" || tokens[i].name=="!") 
            && i<tokens.size()-1 && tokens[i+1].name=="=") {
            tokens[i].name += "=";
            updatedTokens.push_back(tokens[i]);
            ++i;
            continue;
        }
        updatedTokens.push_back(tokens[i]);
        if (tokens[i].name == "default" || tokens[i].name == "new" || tokens[i].name == "try") {
            if (i < tokens.size()-1 && tokens[i+1].name!="{")
                bberror("A { symbol should always follow `"+tokens[i].name+"` but `"+tokens[i+1].name+"` found.\n    To aply one a code block variable (which is a code smell), inline like this `"+tokens[i].name+" {block:}`.\n    Apply the fix at line "+std::to_string(tokens[i].line)); 
        }
        else if (tokens[i].name == "}") {
            if (i >= tokens.size()-1) 
                updatedTokens.push_back(Token(";", tokens[i].line, false));
            else if(tokens[i + 1].name == ";")
                bberror("The syntax }; is invalid because both symbols terminate expressions. Use only } at line "+std::to_string(tokens[i].line));
            else if(tokens[i + 1].name == ":")
                bberror("The syntax }: is invalid because running inline a just-declared code block\n    is equivalent to running its code immediately.\n    Consider removing the brackets at line "+std::to_string(tokens[i].line));
            else if(tokens[i + 1].name != "." 
                    && tokens[i + 1].name != ")" 
                    && tokens[i + 1].name != ","
                    && tokens[i + 1].name != "+"
                    && tokens[i + 1].name != "-"
                    && tokens[i + 1].name != "*"
                    && tokens[i + 1].name != "/"
                    && tokens[i + 1].name != "^"
                    && tokens[i + 1].name != "%"
                    && tokens[i + 1].name != "else")
                updatedTokens.push_back(Token(";", tokens[i].line, false));
        }
        else 
        if (tokens[i].name == ":") {
            if (i >= tokens.size()-1) 
                updatedTokens.push_back(Token(";", tokens[i].line, false));
            else if(tokens[i + 1].name == ";")
                bberror(":; is an invalid syntax. Use only : at line "+std::to_string(tokens[i].line));
            else
                updatedTokens.push_back(Token(";", tokens[i].line, false));
        }
    }
    tokens = std::move(updatedTokens);
}


void compile(const std::string& source, const std::string& destination) {
    // load the source code from the source file
    std::ifstream inputFile(source);
    if (!inputFile.is_open())  
        bberror("Unable to open file: "+source);
    std::string code = "";
    std::string line;
    while (std::getline(inputFile, line)) 
        code += line+"\n";
    inputFile.close();
        
    // create a compiled version of the code
    std::vector<Token> tokens = tokenize(code);
    sanitize(tokens);
    Parser parser(tokens);
    parser.parse(0, tokens.size()-1);
    std::string compiled = parser.get();

    // save the compiled code to the destination file
    std::ofstream outputFile(destination);
    if (!outputFile.is_open())  
        bberror("Unable to write to file: " + destination);
    outputFile << compiled;
    outputFile.close();
}
