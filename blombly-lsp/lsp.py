import sys
sys.stdout.reconfigure(encoding='utf-8')
sys.stdin.reconfigure(encoding='utf-8')

from utils import *
from pathlib import Path
from pygls.server import LanguageServer

server = LanguageServer("blombly", "v0.0.1")

@server.feature('textDocument/completion')
def completions(ls, params):
    doc = ls.workspace.get_text_document(params.text_document.uri)
    current_uri = doc.uri
    lines = doc.source.splitlines()
    line_num = params.position.line
    char_num = params.position.character

    is_in_string = False
    current_string_prefix = ""
    string_token_start = None

    #if line_num < len(lines):
    for start, text, ttype in tokenize_line(lines[line_num], include_symbols=True):
        if ttype == 0 and start <= char_num <= start + len(text):
            is_in_string = True
            string_token_start = start
            current_string_prefix = text[1:char_num - start] if text.startswith('"') else text[:char_num - start]
            break

    if is_in_string and (char_num - string_token_start) < 3:
        string_candidates = [s for s in sorted(indexed_strings) if s.startswith('"' + current_string_prefix)]
        completions_list = [{"label": s, "kind": 1} for s in string_candidates]
        return {"isIncomplete": False, "items": completions_list}

    if hasattr(params, "context") and params.context is not None:
        trigger_char = getattr(params.context, "triggerCharacter", None)
        trigger_after_dot = trigger_char == '.'
    else: trigger_after_dot = line_num < len(lines) and char_num > 0 and lines[line_num][char_num - 1] == '.'

    current_symbols = {token_text for line in doc.source.splitlines() for _, token_text, ttype in tokenize_line(line, include_symbols=True) if ttype == 5}
    other_symbols = {sym for uri, syms in indexed_symbols.items() if uri != current_uri for sym in syms}
    completions_list = [{"label": sym, "kind": 5} for sym in sorted(current_symbols)] + [{"label": sym, "kind": 5} for sym in sorted(other_symbols)]
    if not trigger_after_dot: completions_list += [{"label": kw, "kind": 14} for kw in keywords+preprocessor] + [{"label": op, "kind": 24} for op in operator_completions]
    return {"isIncomplete": False, "items": completions_list}

@server.feature('textDocument/semanticTokens/full')
def semantic_tokens(ls, params):
    """
    Generate semantic tokens for the document.
    
    First, tokenize each line (using include_symbols=True so that symbol tokens are present).
    Then, perform a post‑processing pass to check for patterns:
    
      (c) If a symbol (type 5) is immediately followed by a left parenthesis "(" (token type 4)
          and, after scanning ahead to a matching right parenthesis, an operator "=" or "=>"
          is detected, reclassify that symbol as a function (type 8).
      
      (d) If a symbol (type 5) is followed by an operator "=" and then by the keyword "new",
          reclassify that symbol as a namespace (type 9).
      
      (e) Additionally, if a symbol (type 5) is immediately followed by a dot operator (token type 7 with ".")
          and then by another symbol (type 5), reclassify the left-hand symbol as a namespace (type 9).
    
    (Note: These heuristics are simplistic and may fail in complex cases.)
    """
    doc = ls.workspace.get_text_document(params.text_document.uri)
    lines = doc.source.splitlines()
    token_entries = []

    for line_num, line in enumerate(lines):
        tokens = tokenize_line(line, include_symbols=True)
        i = 0
        while i < len(tokens):
            start, text, ttype = tokens[i]
            if ttype == 5 and (i + 1) < len(tokens):
                # Function pattern.
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
            if ttype == 5 and (i + 2) < len(tokens):
                # Namespace pattern: symbol = new.
                op_token = tokens[i+1]
                new_token = tokens[i+2]
                if op_token[1] == "=" and new_token[1] == "new":
                    tokens[i] = (start, text, 9)  # mark as namespace
            if ttype == 5 and (i + 2) < len(tokens):
                # New namespace pattern: symbol followed by dot and another symbol.
                dot_token = tokens[i+1]
                next_sym = tokens[i+2]
                if dot_token[1] == '.' and dot_token[2] == 7 and next_sym[2] == 5:
                    tokens[i] = (start, text, 9)  # mark as namespace
            i += 1

        for start, token_text, ttype in tokens:
            token_entries.append((line_num, start, len(token_text), ttype))
    token_entries.sort(key=lambda t: (t[0], t[1]))

    data = []
    prev_line = 0
    prev_char = 0
    for line, char, length, ttype in token_entries:
        delta_line = line - prev_line if data else line
        delta_start = char - prev_char if data and delta_line == 0 else char
        data.extend([delta_line, delta_start, length, ttype, 0])
        prev_line, prev_char = line, char

    return {"data": data}

@server.feature('textDocument/definition')
def definition(ls, params):
    """
    Handle "go to definition" requests.
    Look up the token at the given position; if it is a symbol, function, or namespace,
    return all definition locations that were indexed.
    """
    doc = ls.workspace.get_text_document(params.text_document.uri)
    lines = doc.source.splitlines()
    line_num = params.position.line
    char_num = params.position.character

    token_found = None
    tokens_line = tokenize_line(lines[line_num], include_symbols=True)
    for start, text, ttype in tokens_line:
        if start <= char_num <= start + len(text):
            token_found = (start, text, ttype)
            break
    if token_found is None: return None

    token_text = token_found[1]
    token_type = token_found[2]
    if token_type not in (5, 8, 9): return None
    defs = symbol_definitions.get(token_text, [])
    return defs

@server.feature('initialize')
def on_initialize(ls, params):
    # The semantic tokens legend now includes:
    # 0: string, 1: number, 2: keyword, 3: macro, 4: bracket, 5: symbol, 6: comment,
    # 7: operator, 8: function, 9: namespace, 10: fexpr.
    update_indexed_symbols(ls.workspace.root_path if ls.workspace.root_path else ".")
    ls.server_capabilities.semantic_tokens_provider = {
        "legend": {
            "tokenTypes": [
                "string", "number", "keyword", "macro", "bracket", "symbol",
                "comment", "operator", "function", "variable", "fexpr"
            ],
            "tokenModifiers": []
        },
        "full": True,
        "range": False
    }
    ls.server_capabilities.text_document_sync = {
        "openClose": True,  # Detect when documents open/close
        "change": 2,  # Incremental sync (1 = full, 2 = incremental)
        #"save": {"includeText": True},  # Detect when files are saved
    }
    ls.server_capabilities.definition_provider = True 

@server.feature('textDocument/didChange')
def did_change(ls, params):
    """
    Triggered when a file changes.
    Update symbols for the modified file only.
    """
    uri = params.text_document.uri
    file_path = Path(uri).resolve() 
    indexed_symbols.clear()
    indexed_strings.clear()
    symbol_definitions.clear()
    update_indexed_symbols(ls.workspace.root_path if ls.workspace.root_path else ".")


if __name__ == '__main__':
    server.start_io()
