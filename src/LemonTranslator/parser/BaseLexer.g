lexer grammar BaseLexer;

QUOTED_ID : '`' ('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')* '`'
    ;

/** Keywords */
FILTER  :   'filter' | 'Filter' | 'FILTER' ;
JOIN    :   'join' | 'Join' | 'JOIN' ;
PRINT   :   'print' | 'Print' | 'PRINT' ;
AGGREGATE : 'aggregate' | 'Aggregate' | 'AGGREGATE';

GLA     :   'gla' | 'Gla' | 'GLA';
GF      :   'gf' | 'Gf' | 'GF';
GTRAN   : 'gt' | 'Gt' | 'GT';
GIST    :   'gist' | 'Gist' | 'GIST';
GI      :   'gi'    | 'Gi'  | 'GI';
TYPE    :   'type' | 'Type' | 'TYPE';

CREATE : 'create' | 'Create' | 'CREATE';
DROP : 'drop' | 'Drop' | 'DROP';

EQUAL   :   '=' ;

FUNCTION : 'function' | 'Function' | 'FUNCTION';
RELATION : 'relation' | 'Relation' | 'RELATION';
OPKEYWORD : 'operator' | 'Operator' | 'OPERATOR';
DATATYPE : 'datatype' | 'Datatype' | 'DATATYPE';
SYNONYM : 'synonym' | 'Synonym' | 'SYNONYM';
TEMPLATE : 'template' | 'Template' | 'TEMPLATE';

SEPARATOR : 'separator' | 'Separator' | 'SEPARATOR' ;
LIMIT: 'limit' | 'Limit' | 'LIMIT';
ATTRIBUTES : 'attributes' | 'Attributes' | 'ATTRIBUTES' ;
TEXTLOADER : 'textloader' | 'Textloader' | 'TextLoader' | 'TEXTLOADER' ;
FILE : 'file' | 'File' | 'FILE' ;

FLUSH : 'flush' | 'Flush' | 'FLUSH';
QUIT : 'quit' | 'Quit' | 'QUIT';


TYPEOF : 'typeof' | 'TypeOf' | 'TYPEOF';

// matching operator
// we use an operator since it needs a special treatment
// when we get a partial function instantiation, we might be able to use that
// Usage: MATCH(expression, pattern)

MATCH_DP : 'match' | 'Match' | 'MATCH';

// Cases. Only binary supported
// Use: CASE( test, true_expression, false_expression)

CASE_DP :   'case' | 'Case' | 'CASE';
WHEN: 'when' | 'When' | 'WHEN';
THEN: 'then' | 'Then' | 'THEN';
ELSE: 'else' | 'Else' | 'ELSE';

UDF : 'udf' | 'Udf' | 'UDF';

NULL_T : 'null' | 'Null' | 'NULL';

/* general stuff */

fragment TRUE_T : 'true' | 'True' | 'TRUE';
fragment FALSE_T : 'false' | 'False' | 'FALSE';
BOOL_T : TRUE_T | FALSE_T ;

ID  :   ('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')*
    ;

INT :   '-'? '0'..'9'+ 'L'?
    ;

FLOAT
    :   '-'? ('0'..'9')+ '.' ('0'..'9')* EXPONENT? ('f' | 'L')?
    |   '-'? '.' ('0'..'9')+ EXPONENT? ('f' | 'L')?
    |   '-'? ('0'..'9')+ EXPONENT ('f' | 'L')?
    ;

COMMENT
    :   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;

WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;

STRING
    :  '\'' ( ESC_SEQ | ~('\''|'\\') )* '\''
    |  '"' ( ESC_SEQ | ~('\\'|'"') )* '"'
    ;

SEMICOLON : ';' ;
COMMA : ',' ;
COLON : ':' ;
DOT : '.' ;
LPAREN : '(' ;
RPAREN : ')' ;
LSQ : '[' ;
RSQ : ']' ;
QMARK : '?' ;
INTDIV : '//';

// Namespace qualifier
NS : '\\';

/** operators */
LAND : '&&';
LOR : '||';
BAND : '&'; 
BOR : '|';
XOR : '^';
LNOT : '~';

/** comparison operators */
ISEQUAL : '==';
NEQUAL : '!=';
LS : '<';
GT : '>';
LE : '<=';
GE : '>=';
NOT : '!';
ASSIGN : ':=';
REF : '@';

/* shifts and arithmetic */
SLEFT : '<<';
SRIGHT : '>>';
PLUS : '+';
MINUS : '-';
TIMES : '*';
DIVIDE : '/';
MOD : '%';

ARROW : '->';
DARROW : '=>';
LCB : '{';
RCB : '}';
POUND: '#';

fragment
EXPONENT : ('e'|'E') ('+'|'-')? ('0'..'9')+ ;

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
ESC_SEQ
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UNICODE_ESC
    |   OCTAL_ESC
    ;

fragment
OCTAL_ESC
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
UNICODE_ESC
    :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;

fragment
OP
    : '+'|'-'|'*'|'/'|'%'|'<'|'>'|'<='|'>='|'=='|'!='|'<<'|'>>'|'!'|'~'|'^' ;

