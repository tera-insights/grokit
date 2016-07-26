grammar Piggy;
options {
    language = C;
    output = AST;
}

import BaseLexer, BaseParser;

/* Keywords */

LOAD : 'load' | 'Load' | 'LOAD' ;
READ : 'read' | 'Read' | 'READ' ;
USING : 'using' | 'Using' | 'USING';
FROM : 'from' | 'From' | 'FROM' ;
BY : 'by' | 'By' | 'BY' ;
STORE : 'store' | 'Store' | 'STORE';
AS : 'as' | 'As' | 'AS';
TO : 'to' | 'To' | 'TO';
INTO : 'into' | 'Into' | 'INTO';
IN : 'in' | 'In' | 'IN';
NOT_K : 'not' | 'Not' | 'NOT';
FOREACH : 'foreach' | 'Foreach' | 'FOREACH';
GENERATE : 'generate' | 'Generate' | 'GENERATE';
REQUIRES : 'requires' | 'Requires' | 'REQUIRES';
SELF : 'self' | 'Self' | 'SELF';
CHUNKSIZE : 'chunksize' | 'Chunksize' | 'CHUNKSIZE';
GLOB: 'glob' | 'Glob' | 'GLOB';
HEADER : 'HEADER';
JOBID : 'JOBID';
CACHE : 'CACHE';
COMPACT : 'COMPACT';
OVERWRITE : 'OVERWRITE';
DELETE : 'DELETE';
CONTENT : 'CONTENT';
PARAMETERS : 'PARAMETERS' | 'Parameters' | 'parameters';
INLINE : 'INLINE' | 'Inline' | 'inline';
CLUSTER : 'CLUSTER' | 'Cluster' | 'cluster';
RANGE : 'RANGE' | 'Range' | 'range';

parse[LemonTranslator* trans]
    : jobidStatement s=statements quitstatement?
    -> ^(NEWSTATEMENT ^(QUERRY__ ID[ParserHelpers::qry.c_str()]) ) $s ^(RUN__ ID[ParserHelpers::qry.c_str()]) quitstatement?
;

jobidStatement
    : /* nothing */ {
        // create new query
        ParserHelpers::qry = GenerateTemp("Q\%d");
        ParserHelpers::qryShort = ParserHelpers::qry;
        DP_CheckQuery(true, ParserHelpers::qry);
    }
    | JOBID j=identName SEMICOLON {
        std::string jName = (const char *) $j.tree->getText($j.tree)->chars;
        ParserHelpers::qry = jName;
        DP_CheckQuery(true, ParserHelpers::qry);

        std::string val = ParserHelpers::qry;
        std::string shortname = val.substr(0, 8);
        QueryManager& qm = QueryManager::GetQueryManager();

        QueryID qID = qm.GetQueryID(ParserHelpers::qry);
        qm.AddAlias(shortname, qID);

        ParserHelpers::qryShort = shortname;
    }
    ;

quitstatement
    : QUIT SEMICOLON -> QUITTOKEN
    ;

statements
    :(statement SEMICOLON!)+
    ;

statement
  : wpID=identName EQUAL actionBody -> ^(NEWSTATEMENT ^(WAYPOINT__ $wpID actionBody) )
    /* above always creates a new waypoint.  */
  | LOAD rel=identName (AS al=identName)? cl=loadFilter -> ^(NEWSTATEMENT ^(SCANNER__ $rel $al? $cl) )
  | pr=PRINT wpID=identName USING exp=expressionList pType=printType pHeader=printHeader 
    (INTO file=STRING)? (SEPARATOR sep=STRING)? (LIMIT limit=INT)?
    -> ^(NEWSTATEMENT
            ^(WAYPOINT__ ID[$pr, "print"] 
                ^(PRINT TERMCONN $wpID)
                ^(QUERRY__ ID[$pr,ParserHelpers::qry.c_str()] 
                    ^(PRINT $exp $pType $pHeader ^(LIST $file)? ^(SEPARATOR $sep)? ^(LIMIT $limit)? )
                )
            )
        )
  | st=STORE wpID=identName map=storeMap INTO rel=identName ov=overwriteSpec
    -> ^(NEWSTATEMENT ^(WRITER__ $rel ID[$st, ParserHelpers::qry.c_str()] $ov $map TERMCONN $wpID))
  | CREATE createStatement -> createStatement
  | DELETE deleteStatement -> deleteStatement
  | aliasStatement
  | use=USING id=identifier -> ^(IMPORT_[$use,"IMPORT"] $id)
  | clusterStatement
  ;

fragment loadFilter
    : FILTER lfl=loadFilterList -> ^(LOAD_FILTER $lfl)
    | /* nothing */
    ;

fragment loadFilterList
    : loadFilterElem+
    ;

fragment loadFilterElem
    : RANGE min=loadFilterValue COMMA max=loadFilterValue -> ^(LOAD_FILTER_RANGE $min $max)
    ;

fragment loadFilterValue
    : INT
    | NULL_T
    ;

fragment storeMap
    : /* nothing */
    | AS cont=storeMapContents -> ^(STORE_MAP $cont)
    ;

fragment storeMapContents
    : storeMapEntry (COMMA! storeMapEntry)*
    ;

fragment storeMapEntry
    : dest=attribute EQUAL src=storeMapSource -> ^(STORE_MAP_ENTRY $dest $src)
    ;

fragment storeMapSource
    : attribute
    | synthAttribute
    ;

fragment overwriteSpec
    : /* nothing */
    | OVERWRITE -> DELETE_CONTENT
    ;

// DPtree will take care of missing aliases in the definitions
aliasStatement
    : aliasedExplicitType
    ;

printType
    : /* nothing */ -> ^(PRINT_TYPE STRING["\"csv\""])
    | AS s=STRING -> ^(PRINT_TYPE $s)
    ;

printHeader
    : /* nothing */ -> PRINT_NO_HEADER
    | HEADER cnt=printHeaderContent -> ^(PRINT_HEADER $cnt)
    ;

printHeaderContent
    : colonSepList (COMMA! colonSepList)*
    ;

clusterStatement
    : cl=CLUSTER relname=identName on=clusterOn? -> 
        ^(NEWSTATEMENT
            ^(CLUSTER_
                $relname
                ^(QUERRY__ ID[$cl,ParserHelpers::qry.c_str()])
                $on
            )
        )
    ;

fragment clusterOn
    : BY attr=identName -> ^(CLUSTER_ON $attr)
    ;

createStatement
  : RELATION n=identName LPAREN tpAttList RPAREN -> ^(CRRELATION $n tpAttList)
  ;

deleteStatement
    : CONTENT n=identName -> ^(DELETE_CONTENT $n)
    | RELATION n=identName -> ^(DELETE_RELATION $n)
    ;

tpAttList
  : tpAtt ( COMMA! tpAtt)*
  ;

tpAtt
  : var=identName COLON dtype=type -> ^(TPATT $var $dtype)
  ;

inStmt
    : IN -> JOIN_IN
    | NOT_K IN -> JOIN_NOTIN
    ;

actionBody
    : jo=JOIN r1=identName BY l1=attEListAlt COMMA r2=identName BY l2=attEListAlt
        ->  ^(JOIN ^(ATTS $l1) $r1 TERMCONN $r2) ^(QUERRY__ ID[$jo,ParserHelpers::qry.c_str()] ^(JOIN ^(ATTS $l2)))
    | fi=FILTER a=identName BY exp=expressionList
        -> ^(SELECT__ $a) ^(QUERRY__ ID[$fi,ParserHelpers::qry.c_str()] ^(FILTER $exp))
    | fi=FILTER a=identName BY gf=gfDef ct=constArgs st=stateArgs (USING nexp=namedExpressionList)?
        -> ^(SELECT__ $a) ^(QUERRY__ ID[$fi, ParserHelpers::qry.c_str()] ^(GF__ $ct $st $nexp $gf))
    | fi=FILTER r1=identName USING l1=attEListAlt inStmt r2=identName LPAREN l2=attEListAlt RPAREN
        ->  ^(JOIN ^(ATTS $l1) $r1 TERMCONN $r2) ^(QUERRY__ ID[$fi,ParserHelpers::qry.c_str()] ^(JOIN inStmt ^(ATTS $l2)))
    | gla=glaDef par=parameterClause? (FROM inp=identName) st=stateArgs (USING nexp=namedExpressionList)? (AS rez=glaRez)?
        -> ^(GLA__ $inp) ^(QUERRY__ ID[$gla.start,ParserHelpers::qry.c_str()] ^(GLA__ $par $st $rez $nexp $gla ))
    | gt=gtDef ct=constArgs (FROM? inp=identName) st=stateArgs (USING nexp=namedExpressionList)? (AS res=attListOptTypes)?
        -> ^(GT__ $inp) ^(QUERRY__ ID[$gt.start, ParserHelpers::qry.c_str()] ^(GT__ $ct $st $res $nexp $gt))
    | gist=gistDef ct=constArgs st=stateArgs AS rez=glaRez
        -> ^(GIST_WP) ^(QUERRY__ ID[ParserHelpers::qry.c_str()] ^(GIST__ $ct $st $rez $gist))
    | TEXTLOADER FILE? f=readFileSpecList (SEPARATOR s=STRING)? cs=readChunkSize attrs=readAttributes
        -> ^(TEXTLOADER__ $attrs ^(SEPARATOR $s)? $cs  $f )
    | giReadBody
    | fe=FOREACH a=identName GENERATE generateList
        -> ^(SELECT__ $a) ^(QUERRY__ ID[$fe,ParserHelpers::qry.c_str()] generateList )
    | CACHE in=identName -> ^(CACHE_WP $in)
    | COMPACT in=identName -> ^(COMPACT_WP $in)
    ;

readAttributes
    : ATTRIBUTES FROM c=identName -> ^(ATTFROM $c)
    | ATTRIBUTES attrs=attListOptTypes -> ^(ATTS $attrs)
    ;

readChunkSize
    : /* nothing */
    | CHUNKSIZE n=INT -> ^(TUPLE_COUNT $n)
    ;

readFileSpec
    : FILE f=STRING (COLON b=INT)?
        -> ^(FILESPEC_SIMPLE $f $b)
    | GLOB f=STRING
        -> ^(FILESPEC_GLOB $f)
    ;

readFileSpecListContents
    : readFileSpec+
    ;

readFileSpecList
    : l=readFileSpecListContents -> ^(FILESPEC_LIST $l)
    ;

giReadBody
    : READ files=readFileSpecList USING gi=giDef ct=constArgs cs=readChunkSize attrs=readAttributes
        -> ^(GI__ $attrs $files $gi $ct $cs )
    ;

generateItem
    : a=attOptType EQUAL e=expression -> ^(SYNTHESIZE__ $a $e)
    ;

generateList
    : generateItem (COMMA! generateItem)* ;

glaRez
    : attListOptTypes
    | SELF -> STATE__
    ;

/* constructor arguments */
constArgs
  : /* noting */
  | LPAREN! ctAttList RPAREN!
  ;

fragment constArgsNode
    : /* nothing */
    | LPAREN atts=ctAttList RPAREN -> ^(CT_ARGS $atts)
    ;

stateArgs
    : stateArgsReq
    | /* nothing */
    ;

stateArgsNode
    : /* nothing */
    | st=stateArgsReq -> ^(STATE_LIST $st)
    ;

fragment stateArgsReq
    : REQUIRES! stateArg (COMMA! stateArg)*
    ;

stateArg
    : a=identName -> TERMCONN $a
    ;

attEListAlt
  : (attribute|synthAttribute)
  | LPAREN! attributeEList RPAREN!
  ;

colonSepList
    : a=colonSepListInner -> ^(CLIST colonSepListInner)
    ;

colonSepListInner
    : idOrString (COLON! idOrString)*
    ;

idOrString
    : identName
    | STRING
    ;

fragment parameterClause
    : b=PARAMETERS info=parametersInfo -> ^(PARAMETERS__[$b, "PARAMETERS"] $info)
    ;

fragment parametersInfo
    : parametersInline
    | parametersFile
    ;

fragment parametersInline
    : INLINE obj=jsonObject -> ^(JSON_INLINE $obj)
    ;

fragment parametersFile
    : FILE fname=STRING -> ^(JSON_FILE $fname)
    ;

/*
filterExpression
    : filterOrExpression
    ;

fragment filterOrExpression
    : a=filterAndExpression o=LOR b=filterAndExpression -> ^(FIL_BINARY[$o, $o.text->chars] $a $b)
    ;

fragment filterAndExpression
    : a=filterPrimaryExpression o=LAND b=filterPrimaryExpression -> ^(FIL_BINARY[$o, $o.text->chars] $a $b)
    ;

fragment filterPrimaryExpression
    : gfUnaryExpression
    | LPAREN! filterExpression RPAREN!
    ;

fragment gfUnaryExpression
    : o=NOT e=gfExpression -> ^(FIL_UNARY[$o, $o.text->chars] $e)
    | gfExpression
    ;

// Filter expressions (may contain GFs, which act like functions but may require state)
fragment gfExpression
    : gf=gfDef ct=constArgsNode st=stateArgsNode -> ^(GF_EXPR $gf $ct $st)
    | expression
    ;
*/
