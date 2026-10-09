lexer grammar AnitoScriptLexer;

// Keywords
COMPONENT : 'component';
ON        : 'on';
FN        : 'fn';
LET       : 'let';
IF        : 'if';
ELSE      : 'else';
WHILE     : 'while';
FOR       : 'for';
RETURN    : 'return';
BREAK     : 'break';
CONTINUE  : 'continue';
TRUE      : 'true';
FALSE     : 'false';

// Built-in types
FLOAT_T   : 'float';
INT_T     : 'int';
BOOL_T    : 'bool';
STRING_T  : 'string';
VEC3_T    : 'vec3';
VOID_T    : 'void';

// Operators and punctuation
PLUS_ASSIGN  : '+=';
MINUS_ASSIGN : '-=';
STAR_ASSIGN  : '*=';
SLASH_ASSIGN : '/=';
EQ     : '==';
NEQ    : '!=';
LE     : '<=';
GE     : '>=';
AND    : '&&';
OR     : '||';
ASSIGN : '=';
LT     : '<';
GT     : '>';
PLUS   : '+';
MINUS  : '-';
STAR   : '*';
SLASH  : '/';
PERCENT: '%';
BANG   : '!';
QUESTION : '?';
COLON  : ':';
SEMI   : ';';
COMMA  : ',';
DOT    : '.';
AT     : '@';
LPAREN : '(';
RPAREN : ')';
LBRACE : '{';
RBRACE : '}';
LBRACK : '[';
RBRACK : ']';

// Literals
FLOAT_LIT
    : DIGIT+ '.' DIGIT* EXPONENT? 'f'?
    | '.' DIGIT+ EXPONENT? 'f'?
    | DIGIT+ EXPONENT 'f'?
    | DIGIT+ 'f'
    ;
INT_LIT    : DIGIT+ ;
STRING_LIT : '"' ( ~["\\\r\n] | '\\' . )* '"' ;

ID : [a-zA-Z_] [a-zA-Z_0-9]* ;

WS            : [ \t\r\n]+ -> skip ;
LINE_COMMENT  : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;

fragment DIGIT    : [0-9] ;
fragment EXPONENT : [eE] [+-]? DIGIT+ ;
