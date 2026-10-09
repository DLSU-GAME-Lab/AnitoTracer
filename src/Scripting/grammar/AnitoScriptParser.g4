parser grammar AnitoScriptParser;

options { tokenVocab = AnitoScriptLexer; }

script : componentDecl* EOF ;

// component Name : Trigger, Trigger { ... }
componentDecl
    : COMPONENT name=ID (COLON ID (COMMA ID)*)? LBRACE member* RBRACE
    ;

member
    : fieldDecl
    | methodDecl
    | eventHandler
    ;

fieldDecl
    : annotation* type name=ID (ASSIGN expr)? SEMI
    ;

methodDecl
    : annotation* FN name=ID LPAREN params? RPAREN (COLON type)? block
    ;

eventHandler
    : ON name=ID LPAREN params? RPAREN block
    ;

annotation
    : AT name=ID (LPAREN (STRING_LIT (COMMA STRING_LIT)*)? RPAREN)?
    ;

params : param (COMMA param)* ;
param  : name=ID (COLON type)? ;

type
    : primitiveType
    | ID
    ;

primitiveType
    : FLOAT_T
    | INT_T
    | BOOL_T
    | STRING_T
    | VEC3_T
    | VOID_T
    ;

block : LBRACE stmt* RBRACE ;

stmt
    : block                                                       # blockStmt
    | LET name=ID (COLON type)? ASSIGN expr SEMI                  # letStmt
    | IF LPAREN expr RPAREN stmt (ELSE stmt)?                     # ifStmt
    | WHILE LPAREN expr RPAREN stmt                               # whileStmt
    | FOR LPAREN forInit? SEMI expr? SEMI assignment? RPAREN stmt # forStmt
    | RETURN expr? SEMI                                           # returnStmt
    | BREAK SEMI                                                  # breakStmt
    | CONTINUE SEMI                                               # continueStmt
    | assignment SEMI                                             # assignStmt
    | expr SEMI                                                   # exprStmt
    ;

forInit
    : LET name=ID (COLON type)? ASSIGN expr
    | assignment
    ;

assignment
    : lhs=expr op=(ASSIGN | PLUS_ASSIGN | MINUS_ASSIGN | STAR_ASSIGN | SLASH_ASSIGN) rhs=expr
    ;

// Alternatives are ordered from highest to lowest precedence.
expr
    : expr DOT name=ID                                   # memberExpr
    | expr LPAREN argList? RPAREN                        # callExpr
    | expr LBRACK expr RBRACK                            # indexExpr
    | op=(MINUS | BANG) expr                             # unaryExpr
    | expr op=(STAR | SLASH | PERCENT) expr              # mulExpr
    | expr op=(PLUS | MINUS) expr                        # addExpr
    | expr op=(LT | GT | LE | GE) expr                   # relExpr
    | expr op=(EQ | NEQ) expr                            # eqExpr
    | expr AND expr                                      # andExpr
    | expr OR expr                                       # orExpr
    | <assoc=right> expr QUESTION expr COLON expr        # ternaryExpr
    | primary                                            # primaryExpr
    ;

argList : expr (COMMA expr)* ;

// Type keywords are valid callee names so that vec3(...) and float(...) parse as calls.
primary
    : INT_LIT                    # intLiteral
    | FLOAT_LIT                  # floatLiteral
    | STRING_LIT                 # stringLiteral
    | TRUE                       # trueLiteral
    | FALSE                      # falseLiteral
    | ID                         # identifier
    | primitiveType              # typeName
    | LPAREN expr RPAREN         # parenExpr
    ;
