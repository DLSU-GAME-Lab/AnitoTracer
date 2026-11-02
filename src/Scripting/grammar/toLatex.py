import os
import re
import glob
import subprocess

def parse_lexer_tokens(lexer_path):
    """Extract token literal mappings (e.g. COMPONENT -> 'component') and lexer rule names."""
    lexer_map = {}
    lexer_tokens = set()

    if not lexer_path or not os.path.exists(lexer_path):
        return lexer_map, lexer_tokens

    with open(lexer_path, "r", encoding="utf-8") as f:
        content = f.read()

    for line in content.splitlines():
        line = line.strip()
        m = re.match(r"^([A-Z_][A-Z0-9_]*)\s*:\s*'([^']+)'\s*;", line)
        if m:
            lexer_map[m.group(1)] = m.group(2)
        else:
            m2 = re.match(r"^([A-Z_][A-Z0-9_]*)\s*:", line)
            if m2 and not line.startswith("fragment"):
                lexer_tokens.add(m2.group(1))

    return lexer_map, lexer_tokens

def clean_antlr_rule_text(text):
    """Strip ANTLR actions, comments, element labels, and options."""
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    text = re.sub(r'//.*', '', text)
    text = re.sub(r'\{.*?\}', '', text, flags=re.DOTALL)
    text = re.sub(r'\[.*?\]', '', text)
    text = re.sub(r'#[ \t]*[a-zA-Z0-9_]+', '', text)
    text = re.sub(r'<[a-zA-Z0-9_=\s]+>', '', text)
    return text

def split_top_level_alternatives(body):
    """Split rule body on '|' outside parenthetical groups."""
    alternatives = []
    current_alt = []
    depth = 0

    for char in body:
        if char == '(':
            depth += 1
            current_alt.append(char)
        elif char == ')':
            depth -= 1
            current_alt.append(char)
        elif char == '|' and depth == 0:
            alternatives.append("".join(current_alt).strip())
            current_alt = []
        else:
            current_alt.append(char)

    if current_alt:
        alternatives.append("".join(current_alt).strip())

    return [alt for alt in alternatives if alt]

def categorize_rule(rule_name):
    """Categorize grammar rules into paper section headings."""
    r = rule_name.lower()
    if r in ['script', 'program']:
        return "Production for the program"
    elif any(k in r for k in ['component', 'member', 'event']):
        return "Productions for component declarations"
    elif any(k in r for k in ['field', 'method', 'decl', 'annotation', 'param']):
        return "Productions for member declarations"
    elif 'type' in r:
        return "Productions for types"
    elif any(k in r for k in ['stmt', 'block', 'if', 'while', 'for', 'return', 'break', 'continue', 'init']):
        return "Productions for statements"
    elif any(k in r for k in ['expr', 'assign', 'primary', 'arg']):
        return "Productions for expressions"
    else:
        return "Other Productions"

def esc_latex(s):
    """Escape LaTeX special characters safely."""
    chars = {
        '\\': r'\textbackslash{}',
        '&': r'\&',
        '%': r'\%',
        '$': r'\$',
        '#': r'\#',
        '_': r'\_',
        '{': r'\{',
        '}': r'\}',
        '~': r'\textasciitilde{}',
        '^': r'\textasciicircum{}',
    }
    pattern = re.compile('|'.join(re.escape(k) for k in chars.keys()))
    return pattern.sub(lambda m: chars[m.group(0)], s)

def process_token(token, lexer_map, mode="latex"):
    """Format single tokens for BNF notation."""
    token = re.sub(r'^[a-zA-Z_][a-zA-Z0-9_]*=', '', token)
    if not token:
        return ""

    suffix = ""
    if token[-1] in ['*', '+', '?']:
        suffix = token[-1]
        t_base = token[:-1]
    else:
        t_base = token

    if t_base in lexer_map:
        val = lexer_map[t_base]
        if mode == "latex":
            return f"\\textbf{{{esc_latex(val)}}}{suffix}"
        else:
            return f"**{val}**{suffix}"
    elif re.match(r'^[a-z]', t_base):
        if mode == "latex":
            return f"$\\langle\\text{{{esc_latex(t_base)}}}\\rangle${suffix}"
        else:
            return f"<{t_base}>{suffix}"
    elif t_base.startswith("'") and t_base.endswith("'"):
        val = t_base[1:-1]
        if mode == "latex":
            return f"\\textbf{{{esc_latex(val)}}}{suffix}"
        else:
            return f"**{val}**{suffix}"
    else:
        if mode == "latex":
            return f"\\textbf{{{esc_latex(t_base)}}}{suffix}"
        else:
            return f"**{t_base}**{suffix}"

def format_alternative(alt_str, lexer_map, mode="latex"):
    alt_str = re.sub(r'\b[a-zA-Z_][a-zA-Z0-9_]*\s*=\s*', '', alt_str)
    tokens = alt_str.split()
    formatted = [process_token(t, lexer_map, mode) for t in tokens if t]
    
    res = " ".join(formatted)
    if not res:
        return "$\\boldsymbol{\\varepsilon}$" if mode == "latex" else "**ε**"
    return res

def parse_grammar_files(parser_path, lexer_path):
    lexer_map, lexer_tokens = parse_lexer_tokens(lexer_path)

    with open(parser_path, "r", encoding="utf-8") as f:
        content = f.read()

    clean_p = clean_antlr_rule_text(content)
    rule_pattern = re.compile(r'([a-z][a-zA-Z0-9_]*)\s*:(.*?);', re.DOTALL)

    non_terminals = []
    terminals = set()
    rules_data = []

    for match in rule_pattern.finditer(clean_p):
        rule_name = match.group(1).strip()
        body = match.group(2).strip()
        non_terminals.append(rule_name)

        alts = split_top_level_alternatives(body)
        rules_data.append((rule_name, alts))

        for alt in alts:
            alt_clean = re.sub(r'\b[a-zA-Z_][a-zA-Z0-9_]*\s*=\s*', '', alt)
            for t in alt_clean.split():
                t_clean = re.sub(r'[*+?()]', '', t)
                if not t_clean:
                    continue
                if t_clean in lexer_map:
                    terminals.add(lexer_map[t_clean])
                elif t_clean in lexer_tokens:
                    terminals.add(t_clean)
                elif t_clean.startswith("'") and t_clean.endswith("'"):
                    terminals.add(t_clean[1:-1])

    return non_terminals, sorted(list(terminals)), rules_data, lexer_map

def generate_markdown_paper(language_name, non_terminals, terminals, rules_data, lexer_map):
    md = []
    md.append("Gabriel Paulo Rayo\t2026  ")
    md.append("**ANITOSCRIPT CONTEXT FREE GRAMMAR**  ")
    md.append(f"**{language_name} Programming Language**\n")
    
    md.append("The grammar **G = (V, T, S, P)** of our interpreter is defined by:\n")
    
    v_str = ", ".join([f"\\<{nt}\\>" for nt in non_terminals])
    md.append(f"**V** = {{ {v_str} }}  ")
    
    t_str = ", ".join([t for t in terminals])
    md.append(f"**T** = {{ {t_str} }}  ")
    
    start_symbol = non_terminals[0] if non_terminals else "script"
    md.append(f"**S** = \\<{start_symbol}\\>  ")
    md.append("**P** has the following productions:\n")

    categorized = {}
    for name, alts in rules_data:
        cat = categorize_rule(name)
        categorized.setdefault(cat, []).append((name, alts))

    for cat, cat_rules in categorized.items():
        md.append(f"**{cat}**\n")
        for name, alts in cat_rules:
            first = True
            for alt in alts:
                alt_formatted = format_alternative(alt, lexer_map, mode="markdown")
                if first:
                    md.append(f"\\<{name}\\>\t🡪 {alt_formatted}  ")
                    first = False
                else:
                    md.append(f"\t🡪 {alt_formatted}  ")
        md.append("")

    return "\n".join(md)

def generate_latex_paper(language_name, non_terminals, terminals, rules_data, lexer_map):
    v_items = [f"$\\langle\\text{{{esc_latex(nt)}}}\\rangle$" for nt in non_terminals]
    v_str = ", ".join(v_items)

    t_items = [f"\\textbf{{{esc_latex(t)}}}" for t in terminals]
    t_str = ", ".join(t_items)

    start_symbol = esc_latex(non_terminals[0]) if non_terminals else "script"

    latex = r"""\documentclass[11pt,a4paper]{article}
\usepackage[utf8]{inputenc}
\usepackage{amsmath,amssymb}
\usepackage{geometry}
\usepackage{longtable}
\usepackage{array}
\geometry{margin=1in}

\begin{document}

\noindent
Gabriel Paulo Rayo \hfill 2026\\

\vspace{1.5em}

\begin{center}
\textbf{\Large ANITOSCRIPT CONTEXT FREE GRAMMAR} \\[0.5em]
\textbf{\large """ + language_name + r""" Programming Language}
\end{center}

\vspace{1.5em}

\noindent
The grammar \textbf{G = (V, T, S, P)} of our interpreter is defined by:

\vspace{0.8em}
\noindent
\textbf{V} = \{ """ + v_str + r""" \}

\vspace{0.8em}
\noindent
\textbf{T} = \{ """ + t_str + r""" \}

\vspace{0.8em}
\noindent
\textbf{S} = $\langle\text{""" + start_symbol + r"""}\rangle$

\vspace{0.8em}
\noindent
\textbf{P} has the following productions:

\vspace{1.5em}
"""

    categorized = {}
    for name, alts in rules_data:
        cat = categorize_rule(name)
        categorized.setdefault(cat, []).append((name, alts))

    for cat, cat_rules in categorized.items():
        latex += f"\\noindent\\textbf{{{cat}}}\n\n"
        latex += r"\begin{longtable}{r c l}" + "\n"
        for name, alts in cat_rules:
            name_esc = esc_latex(name)
            first = True
            for alt in alts:
                alt_formatted = format_alternative(alt, lexer_map, mode="latex")
                if first:
                    latex += f"$\\langle\\text{{{name_esc}}}\\rangle$ & $\\rightarrow$ & {alt_formatted} \\\\\n"
                    first = False
                else:
                    latex += f" & $\\rightarrow$ & {alt_formatted} \\\\\n"
        latex += r"\end{longtable}" + "\n\n"

    latex += r"\end{document}" + "\n"
    return latex

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))

    parser_files = glob.glob(os.path.join(script_dir, "*Parser.g4")) or glob.glob(os.path.join(script_dir, "*.g4"))
    lexer_files = glob.glob(os.path.join(script_dir, "*Lexer.g4"))

    parser_path = parser_files[0] if parser_files else None
    lexer_path = lexer_files[0] if lexer_files else None

    if not parser_path:
        print("❌ Error: No .g4 files found in script directory.")
        exit(1)

    grammar_name = os.path.basename(parser_path).replace("Parser.g4", "").replace(".g4", "")
    print(f"Scanning grammar: {grammar_name}")

    non_terminals, terminals, rules_data, lexer_map = parse_grammar_files(parser_path, lexer_path)

    # 1. Output Markdown paper
    md_content = generate_markdown_paper(grammar_name, non_terminals, terminals, rules_data, lexer_map)
    md_filename = os.path.join(script_dir, f"{grammar_name}_CFG_Paper.md")
    with open(md_filename, "w", encoding="utf-8") as f:
        f.write(md_content)
    print(f"📄 Markdown paper generated: {os.path.basename(md_filename)}")

    # 2. Output LaTeX paper
    latex_content = generate_latex_paper(grammar_name, non_terminals, terminals, rules_data, lexer_map)
    tex_filename = os.path.join(script_dir, f"{grammar_name}_CFG_Paper.tex")
    with open(tex_filename, "w", encoding="utf-8") as f:
        f.write(latex_content)
    print(f"📄 LaTeX paper generated: {os.path.basename(tex_filename)}")

    # 3. Compile PDF via pdflatex
    try:
        res = subprocess.run(
            ["pdflatex", "-interaction=nonstopmode", os.path.basename(tex_filename)],
            cwd=script_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        if res.returncode == 0:
            print(f"🎉 Success! PDF generated: {grammar_name}_CFG_Paper.pdf")
        else:
            print("⚠️ Note: pdflatex generated the .tex output. Check log if PDF compilation had warnings.")
    except Exception as e:
        print(f"ℹ️ pdflatex executable not invoked directly. Outputs saved as .tex and .md.")