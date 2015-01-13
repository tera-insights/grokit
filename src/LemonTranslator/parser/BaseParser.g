parser grammar BaseParser;

/* Do not set options here */

tokens {
  NEWSTATEMENT;
  OLDSTATEMENT;
  ATTS;
  ATT;
  FUNCTION; /* function definition */
  OPDEF; /* operator definition */
  OPERATOR;
  UOPERATOR; /* unary opearator */
  DELWAYPOINT;
  DELQUERY;
  CRDATATYPE;
  CRSYNONYM;
  FCT;
  TPATT;
  ATTFROM;
  ATTWT; /* attribute with types */
  ATTSWT; /* list of attributes with types */
  ATT_SYNTH; /* synthesized attribute */
  ATT_REL; /* relational attribute */
  RUN__;
  QUERRY__;
  WAYPOINT__;
  SELECT__;
  TERMCONN;
  SCANNER__;
  WRITER__;
  LIST;
  CRRELATION;
  FLUSHTOKEN;
  QUITTOKEN;
  FILE__;
  TEXTLOADER__;
  ATTC;
  JOIN_IN; /* type of join */
  JOIN_NOTIN;
  SYNTHESIZE__;
  FUNCTEMPLATE;
  TYPE_;
  CR_TMPL_FUNC;
  STATE__;
  STATE_LIST;
  CLIST;
  TUPLE_COUNT;
  DELETE_RELATION;
  DELETE_CONTENT;
  CACHE_WP;
  STORE_MAP;
  STORE_MAP_ENTRY;
  LOAD_FILTER;
  LOAD_FILTER_RANGE;
  LOAD_FILTER_IN;

  CLUSTER_;
  CLUSTER_ON; 

  // Parameters for initialization of objects
  PARAMETERS__;

  // Used for JSON
  JSON_INLINE;
  JSON_FILE;

  JSON_LITERAL;
  JSON_OBJECT;
  JSON_ARRAY;
  JSON_ELEM;

  // Used for print
  PRINT_TYPE;
  PRINT_HEADER;
  PRINT_NO_HEADER;

  // Used for case
  CASE_EXPR;
  CASE_BASE;
  CASE_TEST;
  CASE_DEFAULT;

  // Used for including libraries
  IMPORT_;

  // Used for aliasing
  ALIAS_;
  REFERENCE_;

  // Used for typeof
  TYPEOF_;

  // Used for methods
  METHOD;
  METHOD_NAME;
  METHOD_OBJECT;

  // Used for identifiers
  IDENT;
  NAMESPACE;

  // Functors
  FUNCTOR;

  // Used for named expressions
  NAMED_EXPR_LIST;
  NAMED_EXPR;
  EXPR_NAME;
  EXPR_VALUE;

  // Used in definitions
  DEFINITION;   // Marks the node as a definition
  CT_ARGS;      // constructor arguments
  EXPR_LIST;    // list of expressions
  TEMPLATE_DEF; // Template definition

  // Template Arguments
  TARG;         // Template argument
  TARG_NAME;    // Name of named argument
  TARG_VALUE;   // Value of argument

  // Generalized Linear Aggregates
  GLA__;
  GLA_DEF;
  GLATEMPLATE;
  CRGLA;
  CR_TMPL_GLA;
  TYPEDEF_GLA;

  // Generalized Transforms
  GT__;
  GT_DEF;
  GTTEMPLATE;
  CRGT;
  CR_TMPL_GT;
  TYPEDEF_GT;

  // Generalized Filters
  GF__;
  GF_DEF;
  GFTEMPLATE;
  CRGF;
  CR_TMPL_GF;
  TYPEDEF_GF;

  // Filter exressions
  GF_EXPR;
  FIL_UNARY;
  FIL_BINARY;

  // GISTs
  GIST__;
  GIST_WP;
  GIST_DEF;
  GISTTEMPLATE;
  CRGIST;
  CR_TMPL_GIST;
  TYPEDEF_GIST;

  // GIs
  GI__;
  GI_WP;
  GI_DEF;
  GITEMPLATE;
  CRGI;
  CR_TMPL_GI;
  TYPEDEF_GI;

  // Additional stuff used for GI
  FILESPEC_LIST;
  FILESPEC_GLOB;
  FILESPEC_SIMPLE;
}


/*
  Including C++ headers in the "C" version of Antlr is tricky due to
  the extern "C" statements that get injected in the .h files. To
  avoid compilation problems, you have to make sure that any program
  that uses DataPath.h includes it last and includes all headers
  mentioned below first. This is annoying but there does not seem to
  be any way around it.

 */

@header {

#ifndef BASE_GRAMMAR_INCLUDE
#define BASE_GRAMMAR_INCLUDE

 #include <iostream>
 #include "SymbolicWaypointConfig.h"
 #include "LemonTranslator.h"
 #include "AttributeManager.h"
 #include "QueryManager.h"
 #include "ParserHelpers.h"

 #define P_ERR_IF(cond,msg...) {\
   if (cond) {\
         fprintf(stderr, "WARNING : ");\
     fprintf(stderr, msg);\
         fprintf(stderr, "\n");\
     ParserHelpers::haveErrors=true;\
   }\
    }

#define DP_CheckQuery(IsNew,query) {\
    QueryManager& qm = QueryManager::GetQueryManager();\
    if (!IsNew) {\
        std::string qName;\
        P_ERR_IF(qm.GetQueryName(qm.GetQueryID(query),qName), "Query Not registered");\
    } else {\
      QueryID bogus;\
      qm.AddNewQuery(query, bogus);\
    }\
}

#endif //BASE_GRAMMAR_INCLUDE

}

alias
@init{ std::string name; }
    : i=ID ASSIGN
    {
        name = (char*) $i.text->chars;
        name += "_";
        name += ParserHelpers::qry;
    }
    -> ^(ALIAS_ ID[$i, name.c_str()])
    | q=QUOTED_ID ASSIGN {
        name = (const char *) $q.text->chars;
        // Remove quotes
        name = name.substr(1, name.size() - 2);
    } -> ^(ALIAS_ ID[$q, name.c_str()])
    ;

aliasReference
@init{ std::string name; }
    : REF i=ID
    {
        name = (char*) $i.text->chars;
        name += "_";
        name += ParserHelpers::qry;
    }
    -> ^(REFERENCE_ ID[$i, name.c_str()])
    | REF q=QUOTED_ID {
        name = (const char *) $q.text->chars;
        // Remove quotes
        name = name.substr(1, name.size() - 2);
    } -> ^(REFERENCE_ ID[$q, name.c_str()])
    ;

nsComponent
    : identName
    | TYPE -> ID[$TYPE, $TYPE.text->chars]
    | GLA -> ID[$GLA, $GLA.text->chars]
    | GF -> ID[$GF, $GF.text->chars]
    | GTRAN -> ID[$GTRAN, $GTRAN.text->chars]
    | GIST -> ID[$GIST, $GIST.text->chars]
    | GI -> ID[$GI, $GI.text->chars]
    ;

identifierContent
    : (nsComponent NS!)* identName
    ;

identifier
    :   cont=identifierContent -> ^(IDENT $cont)
    ;

tmplType
    : n=identifier
    | TYPEOF LPAREN at=relOrSynAtt RPAREN
        -> ^(TYPEOF_[$TYPEOF, "TYPEOF"] $at)
    | explicitType
    ;

type
    : n=identifier
    | TYPEOF LPAREN at=relOrSynAtt RPAREN
        -> ^(TYPEOF_[$TYPEOF, "TYPEOF"] $at)
    | datatype
    ;

explicitType
    : explicitTypeNoAlias
    | aliasedExplicitType
    ;

glaDef
    : glaDefNoAlias
    | aliasedGlaDef
    | GLA COLON al=aliasReference -> $al
    ;

glaDefNoAlias
    : GLA COLON n=identifier t=templateSpec -> ^(GLA_DEF $n $t)
    ;

aliasedGlaDef
    : al=alias GLA COLON n=identifier t=templateSpec -> ^(GLA_DEF $n $t $al)
    ;

gfDef
    : gfDefNoAlias
    | aliasedGfDef
    | GF COLON al=aliasReference -> $al
    ;

gfDefNoAlias
    : GF COLON n=identifier t=templateSpec -> ^(GF_DEF $n $t)
    ;

aliasedGfDef
    : al=alias GF COLON n=identifier t=templateSpec -> ^(GF_DEF $n $t $al)
    ;

gtDef
    : gtDefNoAlias
    | aliasedGtDef
    | GTRAN COLON al=aliasReference -> $al
    ;

gtDefNoAlias
    : GTRAN COLON n=identifier t=templateSpec -> ^(GT_DEF $n $t)
    ;

aliasedGtDef
    : al=alias GTRAN COLON n=identifier t=templateSpec -> ^(GT_DEF $n $t $al)
    ;

gistDef
    : gistDefNoAlias
    | aliasedGistDef
    | GIST COLON al=aliasReference -> $al
    ;

gistDefNoAlias
    : GIST COLON n=identifier t=templateSpec -> ^(GIST_DEF $n $t)
    ;

aliasedGistDef
    : al=alias GIST COLON n=identifier t=templateSpec -> ^(GIST_DEF $n $t $al)
    ;

giDef
    : giDefNoAlias
    | aliasedGiDef
    | GI COLON al=aliasReference -> $al
    ;

giDefNoAlias
    : GI COLON n=identifier t=templateSpec -> ^(GI_DEF $n $t)
    ;

aliasedGiDef
    : al=alias GI COLON n=identifier t=templateSpec -> ^(GI_DEF $n $t $al)
    ;

datatype
    : datatypeNoAlias
    | aliasedDatatype
    | TYPE COLON al=aliasReference -> $al
    ;

datatypeNoAlias
    : TYPE COLON n=identifier t=templateSpec -> ^(TYPE_ $n $t)
    ;

aliasedDatatype
    : al=alias TYPE COLON n=identifier t=templateSpec -> ^(TYPE_ $n $t $al)
    ;

explicitTypeNoAlias
    : glaDefNoAlias
    | gfDefNoAlias
    | gtDefNoAlias
    | gistDefNoAlias
    | giDefNoAlias
    | datatypeNoAlias
    ;


aliasedExplicitType
    : aliasedGlaDef
    | aliasedGfDef
    | aliasedGtDef
    | aliasedGistDef
    | aliasedGiDef
    | aliasedDatatype
    ;

fragment functorName
    : ID
    | STRING
    ;

fragment functor
    : fName=functorName ob=LPAREN cont=templateParamListContents RPAREN -> ^(FUNCTOR $fName ^(LIST[$ob, "LIST"] $cont))
    ;

relOrSynAtt
    : rel=identName DOT at=identName -> ^(ATT $at $rel)
    | at=identName -> ^(ATT $at)
    ;

templateSpec
    : /* nothing */
    | ob='<' cont=templateParamListContents '>' -> ^(TEMPLATE_DEF[$ob, "TEMPLATE_DEF"] $cont)
    ;

templateParamList
    : ob=LSQ cont=templateParamListContents RSQ -> ^(LIST[$ob,"LIST"] $cont)
    ;

fragment templateParamListContents
    : templateParam (COMMA! templateParam)* (COMMA!)?
    | /* nothing */
    ;

fragment templateParam
    : a=templateParamName b=templateParamValue -> ^(TARG $a $b)
    ;

fragment templateParamSep
    : EQUAL
    | DARROW
    ;

fragment templateAttribute
    : attribute
    | synthAttribute
    ;

fragment templateAttributeWrapped
    : tn=templateParamName ta=templateAttribute -> ^(TARG $tn ^(TARG_VALUE $ta))
    ;

fragment templateAttributeListContents
    : templateAttributeWrapped (COMMA! templateAttributeWrapped)* (COMMA!)?
    ;

fragment templateAttributeList
    : ob=LCB cont=templateAttributeListContents RCB -> ^(LIST[$ob, "LIST"] $cont)
    ;

fragment templateParamName
    : // nothing
    | i=STRING templateParamSep -> ^(TARG_NAME $i)
    | o=identName templateParamSep -> ^(TARG_NAME $o)
    | LNOT a=templateAttribute -> ^(TARG_NAME $a)
    ;

fragment templateParamValue
    : c=ctAtt -> ^( TARG_VALUE $c )
    | t=tmplType -> ^( TARG_VALUE $t )
    | POUND a=templateAttribute -> ^( TARG_VALUE $a )
    | tmp=templateParamList -> ^( TARG_VALUE $tmp )
    | attr=templateAttributeList -> ^( TARG_VALUE $attr )
    | functr=functor -> ^( TARG_VALUE $functr )
    | nul=NULL_T -> ^( TARG_VALUE $nul )
    ;

typeList
    : type (COMMA! type)*
    ;

ctAttList
    : /* nothing */ ->
    | ctAtt (COMMA! ctAtt )*
    ;

ctAtt  : STRING | INT | FLOAT | BOOL_T;

attListWTypes
    : /* nothing */ ->
    | attWType ( COMMA ! attWType )*
    ;

fragment attWType
    : att=ID COLON type -> ^(ATTWT $att type )
    ;


attListOptTypes
    : /*nothing */ ->
    | attOptType ( COMMA! attOptType )*
    ;

fragment attOptType
    : att=identName COLON type -> ^(ATTWT $att type )
    | rel=identName DOT att=identName -> ^(ATT_REL $rel $att)
    | att=identName -> ^(ATT_SYNTH $att)
    ;

attCList
    : /* nothing */ ->
    | attC ( COMMA ! attC )*
    ;

attC
    : attCElem -> ^(ATTC attCElem)
    ;

fragment attCElem
    : ID (COLON! type) (COLON! ID)*
    ;

idList     :    ID ( COMMA ID)*
    ;

// We get the list to which we add our arguments as an argument
attributeList :    attribute  ( COMMA! attribute)*  ;

// extended list
attributeEList :    (attribute|synthAttribute)  ( COMMA! (attribute|synthAttribute))*  ;

attribute
@init {std::string longName;}
    : rel=identName DOT att=identName {
        longName = (char* ) $rel.tree->getText($rel.tree)->chars;
        longName += "_";
        longName += (char *) $att.tree->getText($att.tree)->chars;
    } -> ATT[$att.start, longName.c_str()]
     ;

synthAttribute
@init {std::string longName; }
    :    ID {
      longName = ParserHelpers::qryShort;
      longName += "_";
      longName += (char*)$ID.text->chars;
      // check delayed
      // to check accumulate all atttributes of a query in a list
    } -> ATT[$ID, longName.c_str()]
    | q=QUOTED_ID {
        std::string val = (const char *) $q.text->chars;
        val = val.substr(1, val.size() - 2);

        longName = ParserHelpers::qryShort;
        longName += "_";
        longName += val;
    } -> ATT[$q, longName.c_str()]
  ;


/** Grammar from  http://www.antlr.org/grammar/1153358328744/C.g */

conditional_expression
    : logical_or_expression (QMARK^ expression COLON! expression)?
    ;

logical_or_expression
    //: logical_and_expression ('||'^ logical_and_expression)*
    : (logical_and_expression -> logical_and_expression) (
        o=LOR e=logical_and_expression -> ^(OPERATOR[$o, $o.text->chars] $logical_or_expression $e)
        )*
    ;

logical_and_expression
    //: inclusive_or_expression ('&&'^ inclusive_or_expression)*
    : (inclusive_or_expression -> inclusive_or_expression) (
        o=LAND e=inclusive_or_expression -> ^(OPERATOR[$o, $o.text->chars] $logical_and_expression $e)
        )*
    ;

inclusive_or_expression
    //: exclusive_or_expression ('|'^ exclusive_or_expression)*
    : (exclusive_or_expression -> exclusive_or_expression) (
        o=BOR e=exclusive_or_expression -> ^(OPERATOR[$o, $o.text->chars] $inclusive_or_expression $e)
        )*
    ;

exclusive_or_expression
    //: and_expression ('^'^ and_expression)*
    : (and_expression -> and_expression) (
        o=XOR e=and_expression -> ^(OPERATOR[$o, $o.text->chars] $exclusive_or_expression $e)
        )*
    ;

and_expression
    //: basic_bool_expression ('&'^ basic_bool_expression)*
    : (basic_bool_expression -> basic_bool_expression) (
        o=BAND e=basic_bool_expression -> ^(OPERATOR[$o, $o.text->chars] $and_expression $e)
        )*
    ;

basic_bool_expression
    : equality_expression
    | BOOL_T
    ;

match_expression
    : MATCH_DP LPAREN expr=expression COMMA patt=STRING RPAREN
    -> ^(MATCH_DP $patt $expr)
  ;

case_expression
    : src=CASE_DP base=case_base_expression LCB tests=case_tests deflt=case_default RCB
    -> ^(CASE_EXPR[$src,"CASE"] $base $tests $deflt)
    ;

fragment case_base_expression
    : /* nothing */
    | e=expression -> ^(CASE_BASE $e)
    ;

fragment case_tests
    : case_test+
    ;

fragment case_test
    : WHEN t=expression THEN v=expression -> ^(CASE_TEST $t $v)
    ;

fragment case_default
    : ELSE e=expression -> ^(CASE_DEFAULT $e)
    ;

equality_expression
    //: relational_expression (('=='|'!=')^ relational_expression)*
    : (relational_expression -> relational_expression) (
        (o=ISEQUAL|o=NEQUAL) e=relational_expression -> ^(OPERATOR[$o, $o.text->chars] $equality_expression $e)
        )*
    ;

relational_expression
    //: shift_expression (('<'|'>'|'<='|'>=')^ shift_expression)*
    : (shift_expression -> shift_expression) (
        (o=LS|o=GT|o=LE|o=GE) e=shift_expression -> ^(OPERATOR[$o, $o.text->chars] $relational_expression $e)
        )*
    ;

shift_expression
    //: additive_expression (('<<'|'>>')^ additive_expression)*
    : (additive_expression -> additive_expression) (
        (o=SLEFT|o=SRIGHT) e=additive_expression -> ^(OPERATOR[$o, $o.text->chars] $shift_expression $e)
        )*
    ;

 additive_expression
    //: (multiplicative_expression) ('+'^ multiplicative_expression | '-'^ multiplicative_expression)*
    : (multiplicative_expression -> multiplicative_expression) (
        (o=PLUS|o=MINUS) e=multiplicative_expression -> ^(OPERATOR[$o, $o.text->chars] $additive_expression $e)
        )*
    ;

multiplicative_expression
    //: (unary_expression) ('*'^ unary_expression | '/'^ unary_expression | '%'^ unary_expression)*
    : (unary_expression -> unary_expression) (
        (o=TIMES|o=DIVIDE|o=MOD|o=INTDIV) e=unary_expression -> ^(OPERATOR[$o, $o.text->chars] $multiplicative_expression $e)
        )*
    ;


unary_expression
    :    primary_expression
    |    (o=PLUS|o=MINUS|o=LNOT|o=NOT)  e=unary_expression -> ^(UOPERATOR[$o, $o.text->chars] $e)
    ;

primary_expression
    : attribute
    | synthAttribute
    | constant
    | funct
    | method
    | case_expression
    | match_expression
    | LPAREN! expression RPAREN!
    ;

funct
    :    name=identifier ob=LPAREN expressionList RPAREN -> ^(FUNCTION[$ob, "FUNCTION"] $name expressionList)
    |    src=UDF COLON name=identifier tmp=templateSpec LPAREN e=expressionList RPAREN  ->^(FUNCTION[$src, "FUNCTION"] $name $tmp $e)
    ;

method
    :   ob=LSQ obj=expression ARROW name=identName args=methodParamList RSQ -> ^(METHOD[$ob, "METHOD"] ^(METHOD_NAME $name) ^(METHOD_OBJECT $obj) $args)
    ;

fragment methodParamList
    :   LPAREN! expressionList RPAREN!
    |   /* nothing */
    ;

expressionList
    : /* nothing*/ ->
    | expression (COMMA! expression)* // -> $a
    ;

identName
@init{
    std::string val;
}
    : ID
    | q=QUOTED_ID {
        val = (const char *) $q.text->chars;
        // Remove quotes
        val = val.substr(1, val.size() - 2);
    } -> ID[$q, val.c_str()]
    ;

exprNameSep
    : EQUAL
    | DARROW
    ;

exprName
    : /* nothing */
    | n=identName exprNameSep -> ^(EXPR_NAME $n)
    ;

namedExpression
    : n=exprName v=expression -> ^(NAMED_EXPR $n ^(EXPR_VALUE $v))
    ;

namedExpressionListContents
    : namedExpression (COMMA! namedExpression)*
    ;

namedExpressionList
    : a=namedExpressionListContents -> ^(NAMED_EXPR_LIST $a)
    ;

constant
    :   INT
    |   STRING
    |   FLOAT
    |   NULL_T
    ;

expression
    :    conditional_expression
    ;

fragment jsonValue
    : jsonObject
    | jsonArray
    | jsonSimple
    ;

fragment jsonObject
    : RCB contents=jsonObjectContents LCB -> ^(JSON_OBJECT $contents)
    ;

fragment jsonObjectContents
    : /* nothing */
    | jsonObjectElement (COMMA! jsonObjectElement)*
    ;

fragment jsonObjectElement
    : name=STRING COLON value=jsonValue -> ^(JSON_ELEM $name $value)
    ;

fragment jsonArray
    : LSB contents=jsonArrayContents RSB -> ^(JSON_ARRAY $contents)
    ;

fragment jsonArrayContents
    : /* nothing */
    | jsonValue (COMMA! jsonValue)*
    ;

fragment jsonSimple
    : INT
    | STRING
    | FLOAT
    | NULL_T
    ;
