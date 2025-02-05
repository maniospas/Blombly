import glob
import os
from pathlib import Path


operator_words = ["=>", "<<", "|", ":", "and", "or", "not", "<", ">", "<=", ">=", "=", "+", "-", "/", "^", "%", "+=", "-=", "/=", "^=", "%=", "!=", "=="]
operator_completions = operator_words
preprocessor = ["!include", "!access", "!modify", "!macro", "!of"]
keywords = [
    "final", "new", "return", "while", "in", "if", "else",
    "default", "do", "catch", "assert", "as", "fail", "print", "read",
    "float", "bool", "int", "str", "push", "pop", "next", 
    "false", "true", "sqlite", "graphics", "test", "defer"
]

# Global dictionaries/sets for indexing.
indexed_symbols = {}      # keys: file URIs, values: sets of symbol names.
indexed_strings = set()   # a set of string literals (including incomplete ones) across files.
symbol_definitions = {}   # keys: symbol names; values: list of definition locations.

def tokenize_line(line, include_symbols=False):
    """
    Tokenize a line character-by-character (without regex).

    Token types:
      0 = string literal (delimited by double quotes; may be incomplete)
      1 = number literal (digits, with at most one dot)
      2 = keyword (if it matches one of `keywords`)
      3 = macro (preprocessor directive; any token starting with '!')
      4 = bracket (any of: { } ( ) [ ] < >)
      5 = symbol (a word that is not a keyword, macro, or operator;
                  emitted only if include_symbols is True)
      6 = comment (a line comment starting with //; the rest of the line)
      7 = operator (e.g. =>, <<, ., |, :, =, and operator words like "and")
      
    For semantic highlighting only:
      8 = function (if a symbol appears in a function definition pattern)
      9 = namespace (if a symbol is assigned using "new" or appears in a dotted expression)
      10 = f-expression (an interpolated expression inside a string literal)
    
    A word token is a consecutive sequence of letters, digits, underscores, or exclamation marks.
    
    Returns a list of tuples: (start_index, token_text, token_type_index)
    """
    tokens = []
    i = 0
    n = len(line)
    state = "default"
    token_start = None
    token_value = ""
    dot_used = False

    while i < n:
        ch = line[i]
        if state == "default":
            # Check for line comment.
            if ch == '/' and (i + 1) < n and line[i + 1] == '/':
                tokens.append((i, line[i:], 6))
                break

            # Check for multi-character operator tokens.
            if i + 1 < n and line[i:i+2] in ('=>', '<<'):
                tokens.append((i, line[i:i+2], 7))
                i += 2
                continue

            # Check for single-character operator tokens (including dot).
            if ch in ".|:= ":
                if ch in ".|:=":
                    tokens.append((i, ch, 7))
                    i += 1
                    continue

            if ch.isspace():
                i += 1
                continue

            # Brackets.
            if ch in "{}()[]<>":
                tokens.append((i, ch, 4))
                i += 1
                continue

            # String literal start.
            if ch == '"':
                state = "string"
                token_start = i  # mark beginning of string literal
                token_value = ch
                i += 1
                continue

            # Number literal.
            elif ch.isdigit():
                state = "number"
                token_start = i
                token_value = ch
                dot_used = False
                i += 1
                continue

            # Word (or operator word or macro).
            elif ch.isalpha() or ch == '_' or ch == '!':
                state = "word"
                token_start = i
                token_value = ch
                i += 1
                continue
            else:
                i += 1
                continue

        elif state == "string":
            # In a string literal, check for f-expression marker: "!{"
            if ch == '!' and (i+1) < n and line[i+1] == '{':
                if token_value != "":
                    tokens.append((token_start, token_value, 0))
                i += 2
                expr_start = i
                expr_value = ""
                while i < n and line[i] != '}':
                    expr_value += line[i]
                    i += 1
                if i < n and line[i] == '}':
                    i += 1
                tokens.append((expr_start, expr_value, 10))
                token_start = i
                token_value = ""
                continue
            else:
                token_value += ch
                i += 1
                if ch == '"':  # string literal completed
                    tokens.append((token_start, token_value, 0))
                    state = "default"
                    token_value = ""
                    token_start = None

        elif state == "number":
            if ch.isdigit():
                token_value += ch
                i += 1
            elif ch == '.' and not dot_used:
                token_value += ch
                dot_used = True
                i += 1
            else:
                tokens.append((token_start, token_value, 1))
                state = "default"
                token_value = ""
                token_start = None
                dot_used = False
                continue

        elif state == "word":
            if ch.isalnum() or ch == '_' or ch == '!':
                token_value += ch
                i += 1
            else:
                if   token_value in keywords:       tokens.append((token_start, token_value, 2))
                elif token_value.startswith('!'):   tokens.append((token_start, token_value, 3))
                elif token_value in operator_words: tokens.append((token_start, token_value, 7))
                elif include_symbols:               tokens.append((token_start, token_value, 5))
                state = "default"
                token_value = ""
                token_start = None
                continue

    if state == "string": tokens.append((token_start, token_value, 0))
    elif state == "number": tokens.append((token_start, token_value, 1))
    elif state == "word":
        if token_value in keywords:         tokens.append((token_start, token_value, 2))
        elif token_value.startswith('!'):   tokens.append((token_start, token_value, 3))
        elif token_value in operator_words: tokens.append((token_start, token_value, 7))
        elif include_symbols:               tokens.append((token_start, token_value, 5))
    return tokens


def update_indexed_symbols(root):
    """
    Recursively scan for all .bb files, tokenize their content (with symbols enabled),
    and update the global dictionaries:
      - indexed_symbols: maps file URIs to sets of symbol names.
      - indexed_strings: contains all string literal tokens.
      - symbol_definitions: maps symbol names (for functions and namespaces)
        to lists of definition locations.
    This update is done as tokens are produced.
    """
    for filename in glob.glob(root+"/**/*.bb", recursive=True) + glob.glob(root+"/**/.bb", recursive=True):
        print(filename)
        abs_path = os.path.abspath(filename)
        uri = Path(abs_path).as_uri()
        try:
            with open(filename, 'r') as f: content = f.read()
            symbols = set()
            lines = content.splitlines()
            for line_num, line in enumerate(lines):
                tokens = tokenize_line(line, include_symbols=True)
                # Process tokens for definition classification.
                i = 0
                while i < len(tokens):
                    start, text, ttype = tokens[i]
                    # Function pattern: symbol followed by "(" then later an operator "=" or "=>"
                    if ttype == 5 and (i + 1) < len(tokens):
                        next_token = tokens[i+1]
                        if next_token[1] == "(" and next_token[2] == 4:
                            j = i + 2
                            found_close = False
                            while j < len(tokens):
                                if tokens[j][1] == ")" and tokens[j][2] == 4:
                                    found_close = True
                                    break
                                j += 1
                            if found_close and (j + 1) < len(tokens):
                                following = tokens[j+1]
                                if following[2] == 7 and following[1] in ("=", "=>"):
                                    tokens[i] = (start, text, 8)  # mark as function
                    # Namespace pattern: symbol = new.
                    if ttype == 5 and (i + 2) < len(tokens):
                        op_token = tokens[i+1]
                        new_token = tokens[i+2]
                        if op_token[1] == "=": tokens[i] = (start, text, 9)  # mark as namespace
                    # New namespace pattern: symbol followed by dot and another symbol.
                    #if ttype == 5 and (i + 2) < len(tokens):
                    #    dot_token = tokens[i+1]
                    #    next_sym = tokens[i+2]
                    #    if dot_token[1] == '.' and dot_token[2] == 7 and next_sym[2] == 5:
                    #        tokens[i] = (start, text, 9)  # mark as namespace
                    i += 1

                # Record definitions and update string/symbol indexes.
                for start, token_text, ttype in tokens:
                    if ttype in (8, 9):
                        loc = {"uri": uri, "range": {"start": {"line": line_num, "character": start}, "end": {"line": line_num, "character": start + len(token_text)}}}
                        symbol_definitions.setdefault(token_text, []).append(loc)
                    if ttype == 5: symbols.add(token_text)
                    if ttype == 0: indexed_strings.add(token_text)
            indexed_symbols[uri] = symbols
        except Exception as e:
            print(f"Error processing file {filename}: {e}")

