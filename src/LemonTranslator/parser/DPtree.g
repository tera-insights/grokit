// this is the tree grammar that postprocesses DataPath grammar
tree grammar DPtree;

options {
  language = C;
  tokenVocab = BaseLexer;
  ASTLabelType    = pANTLR3_BASE_TREE;
}

@header {
#ifndef _empty
    #define _empty NULL
#endif

#ifdef _DPtree_H

 #include <iostream>
 #include <fstream>
 #include <map>
 #include <vector>
 #include <set>
 #include <utility>
 #include <string>
 #include <ctime>
 #include <boost/algorithm/string.hpp>
 #include <glob.h>
 #include "SymbolicWaypointConfig.h"
 #include "LemonTranslator.h"
 #include "AttributeManager.h"
 #include "QueryManager.h"
 #include "ExprListInfo.h"
 #include "Catalog.h"
 #include "ExternalCommands.h"
 #include "Errors.h"
 #include "ContainerTypes.h"
 #include "JsonAST.h"
 #include "ParserHelpers.h"

/* Debugging */
#undef PREPORTERROR
#define PREPORTERROR assert(1==2)

#ifndef TXT
#define TXT(x) ((const char*)(x->getText(x))->chars)
#endif
#ifndef TXTN
#define TXTN(x) ((NormalizeQuotes((const char*)(x->getText(x))->chars)).c_str())
#endif
#ifndef TXTS
#define TXTS(x) ((StripQuotes((const char*)(x->getText(x))->chars)).c_str())
#endif
#ifndef STR
#define STR(X) ( std::string(TXT(X)) )
#endif
#ifndef STRN
#define STRN(X) ( std::string(TXTN(X)) )
#endif
#ifndef STRS
#define STRS(X) ( std::string(TXTS(X)) )
#endif

#ifndef LINENO
#define LINENO(x) ((x)->getLine(x))
#endif
#ifndef COLNO
#define COLNO(x) ((x)->getCharPositionInLine(x))
#endif

#ifndef NS_SEP
#define NS_SEP "::"
#endif

#endif // _DPtree_H
}

@members {
static LemonTranslator* lT = NULL; // this is a pointe to the lemon translator used
 //set by parse to be used by all
static WayPointID wp; // this is the currect waypoint. If null, illegal to have a waypoint
 // related statement
static WaypointType wpType = InvalidWaypoint; //type of above waypoint
static QueryID curQuery; // current query. Empty means illegal ...

static bool isNew = false; // is this a new anything.

 // attribute manager that does the translation from attributes to SlotID
 // when it starts it will define the attributes of all relations
static AttributeManager& am = AttributeManager::GetAttributeManager();

 // get it ready for adding queries
static QueryManager& qm = QueryManager::GetQueryManager();

// the catalog
static Catalog& catalog = Catalog::GetCatalog();

// Keeps track of GLAs that return states and their types
static std::map<QueryID, std::map<WayPointID, Json::Value> > glaStateTypeJson;

extern int tempCounter; // id for temporary variables}

// The name of the file being parsed, for error reporting.
static const char * sourceFileName;

/*
 * Converts a slot container into a list of the types of the attributes represented
 * by the slots.
 */
static void SlotContainerToTypes( SlotContainer& cont, std::vector<std::string>& result ) {
    FOREACH_TWL(slot, cont) {
        std::string att_name = am.GetAttributeName(slot);
        std::string att_type = am.GetAttributeType(att_name);
        result.push_back(att_type);
    } END_FOREACH;
}

static std::string MetaQuoted( std::string str ) {
    str = std::string("</") + str + std::string("/>");
    return str;
}

// Extracts source information from a token
static Json::Value Json_SourceInfo( ANTLR3_BASE_TREE * token ) {
    return Json_SourceInfo( sourceFileName, LINENO(token), COLNO(token) );
}

} // End of members clause

parse[LemonTranslator* trans, const char * filename]
@init {
    lT = trans;
    sourceFileName = filename;

    lT->SetJobID(ParserHelpers::qry);
}
@after {
}
   : complexStatement+ ;

identifier returns [std::string name, Json::Value json]
    : ^(IDENT v=ID { $name = STR($v); } (b=ID { $name = $name + NS_SEP + STR($b); } )* ) {
        $json = Json_Identifier( Json_SourceInfo($v), $name );
    }
    ;

tmplType returns[Json::Value json]
@init {
    std::string prefix;
    qm.GetQueryShortName(curQuery, prefix);
    std::string attribute;
    std::string name;
}
    : id=identifier {
        $json = $id.json;
    }
    | ^( to=TYPEOF_ ^(ATT a=ID {attribute = STR($a);} (b=ID {prefix = STR($b);})? ))
    {
        std::string longName = prefix + "_" + attribute;
        $json = Json_TypeOf( Json_SourceInfo($to), Json_Attribute( Json_SourceInfo($a), longName ) );
    }
    | e=explicitDataType { $json = $e.json;  }
    | gla=explicitGlaDef { $json = $gla.json; }
    | gf=explicitGfDef { $json = $gf.json; }
    | gt=explicitGtDef { $json = $gt.json; }
    | gist=explicitGistDef { $json = $gist.json; }
    | gi=explicitGiDef { $json = $gi.json; }
    | al=aliasReference { $json = $al.json; }
    ;

dType returns[std::string type, Json::Value json]
@init {
    std::string prefix;
    qm.GetQueryShortName(curQuery, prefix);
    std::string attribute;
    std::string name;
}
    : id=identifier {
        $type = $id.name;
        $json = $id.json;
    }
    | ^( to=TYPEOF_ ^(ATT a=ID {attribute = STR($a);} (b=ID {prefix = STR($b);})? ))
    {
        std::string longName = prefix + "_" + attribute;
        $type = "TYPEOF";
        $json = Json_TypeOf( Json_SourceInfo($to), Json_Attribute( Json_SourceInfo($a), longName ) );
    }
    | e=explicitDataType { $type = $e.type, $json = $e.json;  }
    ;

explicitDataType returns [std::string type, Json::Value json]
@init{
    std::string kind = "TYPE";
    std::string name;
}
    : ^(TYPE_
            n=identifier {name = $n.name;}
            t=templateSpec
            a=alias
        ) {
            $type = name;
            $json = Json_DataType( ($n.json)[J_NODE_SOURCE], $n.name, $t.json, $a.json );
        }
    ;

aliasReference returns [Json::Value json]
    : ^(REFERENCE_ i=ID) {
        $json = Json_Reference( Json_SourceInfo($i), STR($i) );
    }
    ;

alias returns[Json::Value json]
    : /* nothing */
    | ^(ALIAS_ id=ID) { $json = STR($id); }
    ;

templateSpec returns [Json::Value json]
@init{ std::string args; Json::Value sInfo;
}
@after{
    if( !($json.isArray()) ) {
        $json = Json::Value(Json::arrayValue);
    }
}
    : /* nothing, simple type */ {
        $json = Json::Value(Json::arrayValue);
    }
    | ^(ob=TEMPLATE_DEF { sInfo = Json_SourceInfo($ob);}
        cont=templateParamListContents[sInfo]) {
        $json = $cont.json;
    }
    ;

templateParamList returns [Json::Value json]
@init { Json::Value sInfo;
}
    : ^(ob=LIST { sInfo = Json_SourceInfo($ob); }
        tl=templateParamListContents[sInfo])
        {
            $json = Json_NamedArgs(sInfo, $tl.json);
        }
    ;

templateParamListContents[Json::Value& sInfo] returns [Json::Value json]
@init{
    $json = Json::Value(Json::arrayValue);
    int nArgs = 0;
}
    :
        (t=templateParam[nArgs, $json] {nArgs += 1;})*
    ;

templateParam[int nArgs, Json::Value& list]
    : ^(TARG name=templateParamName[nArgs] val=templateParamValue) {
        Json::Value tVal(Json::objectValue);
        tVal[J_NAME] = $name.name;
        tVal[J_VAL] = $val.json;
        list.append(tVal);
    }
    ;

templateParamName[int nArgs] returns [std::string name]
// Default name is the string of the current argument index
    : /* nothing */ { $name = std::to_string(nArgs); }
    | ^(TARG_NAME
        ( i=STRING
            { $name = STRS($i); }
        | o=ID
            { $name = STR($o); }
        | a=attribute
            {
                SlotID slot = $a.slot;
                $name = am.GetAttributeName(slot);
            }
        )
    )
    ;

templateParamValue returns [Json::Value json]
    : ^(TARG_VALUE cont=templateParamValueContents) {
        $json = $cont.json;
    }
    ;

templateParamValueContents returns [Json::Value json]
    : lit=literalValue {
        $json = $lit.json;
    }
    | t=tmplType {
        $json = $t.json;
    }
    | a=attribute {
        $json = $a.json;
    }
    | f=functor { $json = $f.json; }
    | l=templateParamList { $json = $l.json; }
    ;

functorName returns [std::string name, Json::Value sourceInfo]
    : id=ID { $name = STR($id); $sourceInfo = Json_SourceInfo($id); }
    | s=STRING { $name = STRS($s); $sourceInfo = Json_SourceInfo($s); }
    ;

functor returns [Json::Value json]
    : ^(FUNCTOR fName=functorName targs=templateParamList )
    {
        $json = Json_Functor( $fName.sourceInfo, $fName.name, $targs.json );
    }
    ;

complexStatement
  : ^(NEWSTATEMENT statement[true])
  | ^(OLDSTATEMENT statement[false])
  | ^(DELWAYPOINT ID) /* nothing for now, add */
  | ^(DELQUERY ID) { QueryID q=qm.GetQueryID(TXT($ID)); lT->DeleteQuery(q); }
  | aliasDefinition
  | relationCR
  | relationDeleteContent
  | relationDelete
  | FLUSHTOKEN
        {
            catalog.SaveCatalog();

            // Sleep for a few seconds to allow the Schema message to be sent
            timespec req = { 5, 0 };
            timespec rem;
            while( nanosleep(&req, &rem) != 0 ) {
                req = rem;
            }
        }
  | importStatement
  | runStmt
  | QUITTOKEN { exit(0); }
  ;

importStatement
    : ^(im=IMPORT_ id=identifier)
    {
        lT->AddHeader( Json_Import( Json_SourceInfo($im), $id.json ) );
    }
    ;

aliasDefinition
@init{ Json::Value def; }
@after{ lT->AddHeader( def ); }
    : e=explicitDataType { def = $e.json;  }
    | gla=explicitGlaDef { def = $gla.json; }
    | gf=explicitGfDef { def = $gf.json; }
    | gt=explicitGtDef { def = $gt.json; }
    | gist=explicitGistDef { def = $gist.json; }
    | gi=explicitGiDef { def = $gi.json; }
    | al=aliasReference { def = $al.json; }
    ;

reqStateList returns [Json::Value json]
@init { $json = Json::Value(Json::arrayValue); }
    : ^(STATE_LIST (t=dType { $json.append($t.json); })+ )
    | /* nothing */
    ;

runStmt
@init{QueryIDSet qrys;}
@after{lT->Run(qrys);}
  : ^(RUN__ (ID{
        QueryID lq = qm.GetQueryID(TXT($ID));
        // check lq to ensur it is valid
        qrys.Union(lq);
        })+)
  ;

relationCR
@init { Schema newSch; int index = 0; }
@after { catalog.AddSchema(newSch);/* register the relatin with catalog */ }
  : ^(CRRELATION x=ID {newSch.SetRelationName(TXT($x));/* set relation name */}
      ( ^(TPATT n=ID t=dType)
        {
            Attribute att;
            att.SetName(TXT($n));

            std::string type = $t.type;
            Json::Value jType = $t.json;

            att.SetType(type);
            att.SetJType(jType);
            att.SetIndex(++index);
            newSch.AddAttribute(att);/* add attribte n with type t */ }
      )+ )
  ;

relationDeleteContent
  : ^( DELETE_CONTENT n=ID ) {
    std::string relName = STR($n);
    DeleteRelationTask task(relName);
    lT->AddTask(task);
  }
  ;

relationDelete
    : ^( DELETE_RELATION n=ID ) {
        std::string relName = STR($n);
        DeleteRelationTask task(relName);
        lT->AddTask(task);

        catalog.RemoveSchema(relName);
    }
    ;

lstArgsFc returns [Json::Value json]
@init { $json = Json::Value(Json::arrayValue); }
  :( dType { $json.append($dType.json); } )*
  ;

lstArgsGLA returns [Json::Value json]
@init { $json = Json::Value(Json::arrayValue); }
    : ( f=dType { $json.append($f.json); } )+
    ;

statement[bool isNew]
    : scanner
    | waypoint[isNew]
    | query
    | writer
    | cluster
    ;

writer
@init {
    SlotToSlotMap storeMap;
    SlotContainer attribs;
    WayPointID scanner;
    std::string relName;
}
    : ^(WRITER__ a=ID b=ID {
        // set wp to current scanner
        wp = WayPointID::GetIdByName((const char*)$a.text->chars);
        // set the query
        relName = STR($a);
        am.GetAttributesSlots(relName, attribs); // put attributes in attribs

        Json::Value sourceInfo = Json_SourceInfo($WRITER__);

        scanner = WayPointID::GetIdByName(relName.c_str());
        curQuery = qm.GetQueryID((const char*) $b.text->chars);
      } (DELETE_CONTENT {
        DeleteRelationTask task(relName);
        lT->AddTask(task);
      })?
      storeMapping[storeMap, attribs] {
        lT->AddScannerWP(scanner, relName, attribs);
        lT->AddWriter(scanner, curQuery, storeMap);
        // now wp is set for connList
      }
      connList)
    ;

storeMapping[SlotToSlotMap & map, SlotContainer & attribs]
    : /* nothing */ {
        // Just make an identity map from attribs
        FOREACH_TWL(slot, attribs) {
            SlotID slotCopy1 = slot;
            SlotID slotCopy2 = slot;
            map.Insert(slotCopy1, slotCopy2);
        } END_FOREACH;
    }
    | ^(STORE_MAP
        (^(STORE_MAP_ENTRY dest=attribute src=attribute {
            map.Insert($dest.slot, $src.slot);
        }))+
    )
    ;

scanner
    @init { std::string sName; // scanner name, possible alias for relation
            std::string rName; // relation name
            WayPointID scanner;
          }
    :    ^(SCANNER__ a=ID { rName=(char*)$a.text->chars; sName=rName;}
                (b=ID {sName=(char*)$b.text->chars;})?
            {
            SlotContainer attribs;
            am.AliasAttributesSlots(rName, sName, attribs);
            //am.GetAttributesSlots(rName, attribs); // put attributes in attribs
            //WayPointID scanner = WayPointID::GetIdByName(sName.c_str());

            scanner = WayPointID(sName);
            lT->AddScannerWP(scanner, rName, attribs);
            // Tell the scanner to report it can read all columns
            lT->AddScanner(scanner, curQuery);
        }
            scannerFilters[scanner]
        )
    ;

fragment scannerFilters[WayPointID& wpID]
    : /* nothing */
    | ^(LOAD_FILTER scannerFilter[wpID]+)
    ;

fragment scannerFilter[WayPointID& wpID]
    : ^(LOAD_FILTER_RANGE min=scannerFilterMin max=scannerFilterMax) {
        lT->AddScannerRange(wpID, curQuery, $min.value, $max.value);
    }
    ;

fragment scannerFilterMin returns [int64_t value]
    : i=INT {
        std::string eVal = STR($i);
        int64_t val = stoll( eVal );
        $value = val;
    }
    | NULL_T {
        $value = std::numeric_limits<int64_t>::min();
    }
    ;

fragment scannerFilterMax returns [int64_t value]
    : i=INT {
        std::string eVal = STR($i);
        int64_t val = stoll( eVal );
        $value = val;
    }
    | NULL_T {
        $value = std::numeric_limits<int64_t>::max();
    }
    ;

waypoint[bool isNew]
    @after { wp = WayPointID(); }
    :    ^(WAYPOINT__ ID {
            if (isNew){
                /*waypointIncludes.clear();*/
                WayPointID nWp((const char*)$ID.text->chars);
                wp = nWp;
            } else {
                wp = WayPointID::GetIdByName((const char*)$ID.text->chars);
            }
        }  wpDefinition? bypassRule* wpbodyStatement* )
    ;

query
  :    ^(QUERRY__ (ID {
            curQuery = qm.GetQueryID((const char*) $ID.text->chars);
            /*qm.AddNewQuery(std::string((const char*) $ID.text->chars), curQuery);*/
        })+  qBodyStatement* )
  ;


qBodyStatement
    :    ^(WAYPOINT__ ID {
            wp = WayPointID::GetIdByName((char*)$ID.text->chars);
            //wpType=typeMap[wp];
        }  bodyStatement)
    ;

wpbodyStatement
  : ^(QUERRY__ ID {curQuery = qm.GetQueryID((const char*) $ID.text->chars);} bodyStatement)
  ;

bodyStatement
  :    rules*
  ;

rules :
    filterRule
  | synthRule
  | printRule
  | joinRule
  | glaRule
  | gtRule
  | gfRule
  | gistRule
  ;

filterRule
    @init {SlotContainer atts; /* the set of attributes */
      std::string cstStr; /* the constants used in the expression */
      std::string defs; /* definitions needed by expressions */
        }
   : ^(FILTER expr[atts]) {
        std::vector<WayPointID> sources;

        Json::Value jVal(Json::objectValue);
        jVal[J_ARGS] = $expr.json;
        jVal[J_TYPE] = Json::Value(Json::nullValue);
        jVal[J_CARGS] = Json::Value(Json::arrayValue);
        jVal[J_SARGS] = Json::Value(Json::arrayValue);

        lT->AddFilter(wp, curQuery, atts, sources, jVal);
    }
  ;

synthRule
@init {
    SlotContainer atts; /* the set of attributes */
    SlotContainer outAtts;
    Json::Value genAtt(Json::arrayValue);
}
    : ^(SYNTHESIZE__ a=attLOT[outAtts, genAtt] v=expression[atts]) {
        Json::Value jVal(Json::objectValue);
        jVal[J_ATT] = genAtt[0u];
        jVal[J_EXPR] = $v.json;

        outAtts.MoveToStart();
        SlotID sID = outAtts.Current();

        lT->AddSynthesized(wp, curQuery, sID, atts, jVal);
    }
  ;

printRule
    @init {SlotContainer atts; /* the set of attributes */
      std::vector< std::vector< std::string > > header;
      std::string file;
      std::string separator = ",";
      std::string type;
      Json::Value headerColumns(Json::arrayValue);
    }
    : ^(PRINT expr[atts] printType[type] printAtts[headerColumns] printFile[file] printSep[separator] printLimit)
    {
        if( headerColumns.size() != 0 ) {
            FATALIF(headerColumns.size() != $expr.json.size(),
                "Print got \%lu header columns but \%lu expressions",
                headerColumns.size(), $expr.json.size() );
        }

        Json::Value val(Json::objectValue);
        val[J_EXPR] = $expr.json;
        val[J_FILE] = file;
        val[J_SEP] = separator;
        val[J_HEADER] = headerColumns;
        val[J_TYPE] = type;
        val[J_LIMIT] = $printLimit.limit;

        lT->AddPrint(wp, curQuery, atts, val);
    }
  ;

printType[std::string & type]
    : ^(PRINT_TYPE s=STRING) { type = STRS($s); }
    ;

printAtts[Json::Value& json]
    @init{
        Json::Value column = Json::Value(Json::arrayValue);
    }
    : PRINT_NO_HEADER
    | ^( PRINT_HEADER (
        ^(CLIST
            (a=ID
                {
                    column.append(STR($a));
                }
            | s=STRING {
                column.append(STRS($s));
            }
            )+
            {
                json.append(column);
                column = Json::Value(Json::arrayValue);
            }
        ) )+
      )
    ;

ctAttList[Json::Value& json]
@init { json = Json::Value(Json::arrayValue); }
    : /*no arg, nothing in result*/
    | a=literalValue {json.append($a.json); }
        (b=literalValue  {json.append($b.json);} )*
    ;

printFile[std::string& s]
    : /* nothing */
    | ^(LIST a=STRING ) {s=STRS(a);}
    ;

printSep[std::string& s]
    : /* nothing */
    | ^(SEPARATOR sep=STRING)
    {
        s = STRS($sep);
    }
    ;

printLimit returns [int limit]
    : 
    {
        $limit = -1; // this is unlimited
    }
    | ^(LIMIT lim=INT)
    {
        std::string eVal = STR($lim);
        $limit = stol( eVal );
    };    

/* accumulate arguments form GLA definitions and form m4 code to call it */
glaDef returns [Json::Value json]
    : e=explicitGlaDef { $json = $e.json; }
    | al=aliasReference { $json = $al.json;  }
  ;

explicitGlaDef returns [Json::Value json]
    : ^(GLA_DEF id=identifier
        ta=templateSpec
        a=alias
    )
    {
        std::string name = $id.name;
        Json::Value sInfo = ($id.json)[J_NODE_SOURCE];
        $json = Json_GLA( sInfo, name, $ta.json, $a.json );
    }
    ;

gtDef returns [Json::Value json]
    : e=explicitGtDef { $json = $e.json; }
    | al=aliasReference { $json = $al.json;  }
    ;

explicitGtDef returns [Json::Value json]
  : ^(GT_DEF id=identifier
        ta=templateSpec
        a=alias
    )
    {
        std::string name = $id.name;
        Json::Value sInfo = ($id.json)[J_NODE_SOURCE];
        $json = Json_GT( sInfo, name, $ta.json, $a.json );
    }
  ;

gfDef returns [Json::Value json]
    : e=explicitGfDef { $json = $e.json; }
    | al=aliasReference { $json = $al.json;  }
    ;

explicitGfDef returns [Json::Value json]
    : ^(GF_DEF id=identifier
        ta=templateSpec
        a=alias
    )
    {
        std::string name = $id.name;
        Json::Value sInfo = ($id.json)[J_NODE_SOURCE];
        $json = Json_GF( sInfo, name, $ta.json, $a.json );
    }
    ;

gistDef returns [Json::Value json]
    : e=explicitGistDef { $json = $e.json; }
    | al=aliasReference { $json = $al.json;  }
    ;

explicitGistDef returns [Json::Value json]
    : ^(GIST_DEF id=identifier
        ta=templateSpec
        a=alias
    )
    {
        std::string name = $id.name;
        Json::Value sInfo = ($id.json)[J_NODE_SOURCE];
        $json = Json_GIST( sInfo, name, $ta.json, $a.json );
    }
    ;

giDef returns [Json::Value json]
    : e=explicitGiDef { $json = $e.json; }
    | al=aliasReference { $json = $al.json;  }
    ;

explicitGiDef returns [Json::Value json]
    : ^(GI_DEF id=identifier
        ta=templateSpec
        a=alias
    )
    {
        std::string name = $id.name;
        Json::Value sInfo = ($id.json)[J_NODE_SOURCE];
        $json = Json_GI( sInfo, name, $ta.json, $a.json );
    }
    ;

stateArgs[std::vector<WayPointID>& stateSources, Json::Value& json]
@init{ json = Json::Value(Json::arrayValue); }
    : /* nothing */
    | (TERMCONN s=ID {
            std::string sourceName = STR($s);
            WayPointID sourceID = WayPointID::GetIdByName(sourceName.c_str());
            if( glaStateTypeJson[curQuery].find(sourceID) != glaStateTypeJson[curQuery].end() ) {
                lT->AddTerminatingEdge(sourceID, wp);
                lT->ReturnAsState(sourceID, curQuery);

                stateSources.push_back(sourceID);

                json.append(Json_State( Json_SourceInfo($s), sourceID.getName(), glaStateTypeJson[curQuery][sourceID] ));

                glaStateTypeJson[curQuery].erase(sourceID);

            }
            else {
                FATAL("State required from waypoint \%s, but that waypoint not registered as returning a state.", sourceName.c_str());
            }
        }
    )+
    ;

glaRez[SlotContainer& outAtts, Json::Value& json] returns [bool retState]
@init{
    $retState = false;
    json = Json::Value(Json::arrayValue);
}
    : attLOT[outAtts, json]*
    | STATE__ {
        $retState = true;
    }
    ;

cacheOption[bool& cache]
    : DO_CACHE { cache = true; }
    | NO_CACHE { cache = false; }
    ;

glaRule
    @init {
            SlotContainer atts; /* the set of attributes */
            SlotContainer outAtts; /**output attributes */
            std::string cstStr; /* the constants used in the expression */
            std::string sExpr; // the entire expression representing the arguments
            ExprListInfo lInfo;
            std::string ctArgs="("; /* constructor arguments*/
            std::string defs; /* the definitions needed by the expressions */
            std::vector<WayPointID> reqStateSources;

            // JSON version of stuff
            Json::Value jCtArgs(Json::objectValue);
            Json::Value jRez(Json::arrayValue);
            Json::Value jStates(Json::arrayValue);
        }
    : ^(GLA__
            objectParameters[jCtArgs]
            stateArgs[reqStateSources, jStates]
            res=glaRez[outAtts, jRez]
            nexp=namedExpressionList[atts]
            glaDef
        )
        {

            glaStateTypeJson[curQuery][wp] = $glaDef.json;

           Json::Value jVal(Json::objectValue);
           jVal[J_TYPE] = $glaDef.json;
           jVal[J_ARGS] = $nexp.json;
           jVal[J_VAL] = jRez;
           jVal[J_CARGS] = jCtArgs;
           jVal[J_STATE] = $res.retState;
           jVal[J_SARGS] = jStates;

           lT->AddGLA(wp,curQuery, outAtts, atts, reqStateSources, jVal );
    }
  ;

gtRule
    @init {
            SlotContainer atts; /* the set of attributes */
            SlotContainer outAtts; /**output attributes */
            std::string cstStr; /* the constants used in the expression */
            std::string sExpr; // the entire expression representing the arguments
            ExprListInfo lInfo;
            std::string ctArgs="("; /* constructor arguments*/
            std::string defs; /* the definitions needed by the expressions */
            std::vector<WayPointID> reqStateSources;

            // JSON version of stuff
            Json::Value jCtArgs(Json::arrayValue);
            Json::Value jRez(Json::arrayValue);
            Json::Value jStates(Json::arrayValue);
        }
    : ^(GT__
            objectParameters[jCtArgs]
            stateArgs[reqStateSources, jStates]
            res=attLOT[outAtts, jRez]*
            nexp=namedExpressionList[atts]
            gtDef
        )
        {

            Json::Value jVal(Json::objectValue);
            jVal[J_TYPE] = $gtDef.json;
            jVal[J_ARGS] = $nexp.json;
            jVal[J_VAL] = jRez;
            jVal[J_SARGS] = jStates;
            jVal[J_CARGS] = jCtArgs;

            // Old stuff
            jVal[J_C_EXPR] = sExpr;
            jVal[J_CST_STR] = cstStr;
            jVal[J_C_DEFS] = defs;

            lT->AddGT(wp,curQuery, outAtts, atts, reqStateSources, jVal);
    }
  ;

gfRule
    @init {
            SlotContainer atts; /* the set of attributes */
            std::vector<WayPointID> reqStateSources;
        }
    : gf=gfExpression[atts, reqStateSources] {
           lT->AddFilter(wp, curQuery, atts, reqStateSources, $gf.json);
    }
  ;

gistRule
    @init {
            SlotContainer outAtts; /**output attributes */
            std::string defs; /* the definitions needed by the expressions */
            std::vector<WayPointID> reqStateSources;

            // JSON version of stuff
            Json::Value jCtArgs;
            Json::Value jRez;
            Json::Value jStates;
        }
    : ^(GIST__
            objectParameters[jCtArgs]
            stateArgs[reqStateSources, jStates]
            res=glaRez[outAtts, jRez]
            gistDef
        )
        {
            glaStateTypeJson[curQuery][wp] = $gistDef.json;

            Json::Value jVal(Json::objectValue);
            jVal[J_TYPE] = $gistDef.json;
            jVal[J_VAL] = jRez;
            jVal[J_CARGS] = jCtArgs;
            jVal[J_STATE] = $res.retState;
            jVal[J_SARGS] = jStates;

           // Old stuff
           jVal[J_C_DEFS] = defs;

           lT->AddGIST(wp,curQuery, outAtts, reqStateSources, jVal );
       }
  ;

attLWT [SlotContainer& outAtts, Json::Value& json]
    : ^(ATTWT att=ID type=dType) {
            std::string name = STR($att);
            std::string t = $type.type;

            SlotID glaID = am.AddSynthesizedAttribute(curQuery, name, t, $type.json);

            // Only care about the name of the attribute, we will be able to look up
            // the type and slot later
            json.append( Json_Attribute( Json_SourceInfo($att), am.GetAttributeName(glaID) ) );
            outAtts.Append(glaID);
        }
    ;

attLOT [SlotContainer& outAtts, Json::Value& json]
    : attLWT[outAtts, json]
    | ^(ATT_REL rel=ID att=ID) {
        std::string relation = STR($rel);
        std::string attribute = STR($att);
        SlotID id = am.GetAttributeSlot(relation, attribute);

        FATALIF( !id.IsValid(), "Attribute \%s.\%s does not exist", relation.c_str(), attribute.c_str());

        json.append( Json_Attribute( Json_SourceInfo($rel), am.GetAttributeName(id) ) );
        outAtts.Append(id);
    }
    | ^(ATT_SYNTH att=ID) {
        std::string name = STR($att);
        std::string prefix;
        qm.GetQueryShortName(curQuery, prefix);

        SlotID id = am.GetAttributeSlot(prefix, name);

        if( !id.IsValid() ) {
            // Need to add a new attribute.
            std::string type = "auto";
            Json::Value jType(Json::nullValue);
            id = am.AddSynthesizedAttribute( curQuery, name, type, jType );
        }

        json.append( Json_Attribute( Json_SourceInfo($att), am.GetAttributeName(id) ) );
        outAtts.Append(id);
    }
    ;

bypassRule :
    ^(BYPASS ID) {
      curQuery = qm.GetQueryID((const char*) $ID.text->chars);
      lT->AddBypass(wp,curQuery);
    }
  ;

joinType returns [LemonTranslator::JoinType type]
: /* none, normal */ {$type = LemonTranslator::Join_EQ;}
| JOIN_IN { $type = LemonTranslator::Join_IN; }
| JOIN_NOTIN { $type = LemonTranslator::Join_NOTIN; }
;

joinRule
    @init
    {
        SlotContainer atts; /* the set of attributes */
        Json::Value jAttrs(Json::arrayValue);
    }
    : ^(JOIN joinType attributeList[atts, jAttrs])
    {
        std::string defs;

        Json::Value jVal(Json::objectValue);
        jVal[J_ARGS] = jAttrs;
        jVal[J_TYPE] = $joinType.type;
        jVal[J_C_DEFS] = defs;

        lT->AddJoin(wp, curQuery, atts, $joinType.type, jVal);
    }
  ;

attribute returns [SlotID slot, Json::Value json]
    :     att=ATT {
            std::string attName = STR($att);
            $slot = am.GetAttributeSlot(attName);
            FATALIF( !$slot.IsValid(), "Attribute \%s does not exist", attName.c_str());

            $json = Json_Attribute(Json_SourceInfo($att), am.GetAttributeName($slot));
        }
     ;

selectWP
    :    ^(SELECT__ {
            lT->AddSelectionWP(wp);
        }  connList )
    ;

joinWP
@init
    {
        SlotContainer atts;

        Json::Value jAttrs(Json::arrayValue);
    }
    : ^(JOIN attributeList[atts, jAttrs]
    {
        std::string defs;

        Json::Value jVal(Json::objectValue);
        jVal[J_ARGS] = jAttrs;
        jVal[J_C_DEFS] = defs;

        lT->AddJoinWP(wp, atts, jVal);
    } connList)
    ;

attributeList[SlotContainer& atts, Json::Value& json]
: ^(ATTS (a=attribute {$atts.Append($a.slot); json.append($a.json);})+ )
;

cacheWP :
    ^(CACHE_WP a=ID) {
        WayPointID source = WayPointID::GetIdByName(TXT($a));
        lT->AddCacheWP(wp);
        lT->AddEdge(source, wp);
        lT->AddCaching(wp, curQuery);
    }
    ;

glaWP
    : ^(GLA__ {
        lT->AddGLAWP(wp);
    }
      connList )
  ;

gtWP
    : ^(GT__ {
        lT->AddGTWP(wp);
    }
      connList )
  ;

gistWP
    : GIST_WP {
        lT->AddGISTWP(wp);
        lT->AddEdgeFromBottom(wp);
    }
    ;

printWP
    :    ^(PRINT {
      lT->AddPrintWP(wp);
      }
      connList )
  ;

cluster
@init{
    std::string cAttName;
}
    : ^(CLUSTER_
            rel=ID
            ^(QUERRY__ qid=ID)
            clusterAtt[cAttName]
        {
            SlotID clusterAtt;
            QueryID query = qm.GetQueryID(TXT($qid));

            // Create the scanner
            std::string relName = STR($rel);
            SlotContainer attribs;
            am.GetAttributesSlots(relName, attribs);

            WayPointID scanner(relName);
            lT->AddScannerWP(scanner, relName, attribs);
            lT->AddScanner(scanner, query);

            Schema schema;
            catalog.GetSchema(relName, schema);

            if( !cAttName.empty() ) {
                // Ensure clusterAttr is in attribs
                bool valid = schema.SetClusterAttribute(cAttName);

                FATALIF(!valid,
                    "Specified clustering attribute does not exist in relation");

                catalog.AddSchema(schema);
            } else {
                Attribute schemaAtt;
                bool clustered = schema.GetClusterAttribute(schemaAtt);

                FATALIF(!clustered,
                    "No clustering attribute specified and relation not already clustered");

                cAttName = schemaAtt.GetName();

                lT->AddScannerRange(scanner, query, 1, 0);
            }
            clusterAtt = am.GetAttributeSlot(relName, cAttName);

            WayPointID cluster("cluster");

            lT->AddClusterWP(cluster, relName, clusterAtt, query);
            lT->AddTerminatingEdge(scanner, cluster);
        }
    );

fragment clusterAtt[std::string& attr]
    :   /* nothing */
    | ^(CLUSTER_ON a=ID) {
        attr = STR($a);
    }
    ;

connList
    : wayPointCN+
    ;

wayPointCN
    :    ID {
          WayPointID nWP = WayPointID::GetIdByName((char*)$ID.text->chars);
          lT->AddEdge(nWP, wp);
        }
    | TERMCONN ID{
          WayPointID nWP = WayPointID::GetIdByName((char*)$ID.text->chars);
          lT->AddTerminatingEdge(nWP, wp);
        }
    ;

textloaderWP
@init {
    SlotContainer attribs;
    char sep;
    std::string tablePattern;
    int count = 0;
    std::string defs;
    size_t tuplesPerChunk;

    Json::Value jAttrs(Json::arrayValue);
    Json::Value jFiles(Json::arrayValue);
}
@after {
    Json::Value jVal(Json::objectValue);
    jVal[J_ARGS] = jAttrs;
    jVal[J_FILE] = jFiles;
    jVal[J_SEP] = std::string(1, sep);
    jVal[J_TUPLES] = Json::UInt64(tuplesPerChunk);
    jVal[J_C_DEFS] = defs;

    lT->AddTextLoaderWP(wp, attribs, jVal );
    lT->AddScanner(wp, curQuery); // DO WE NEED THIS?
}
  :
    ^(TEXTLOADER__
            textloaderAttributes[attribs, jAttrs]
            ^(SEPARATOR a=STRING) {sep = char(*(StripQuotes(TXT($a)).c_str()));}
            textloaderChunkSize[tuplesPerChunk]
            giFileList[jFiles]
   )
;

giWP
@init{
    SlotContainer attribs;
    std::string giType;
    std::string defs;
    size_t tuplesPerChunk;

    Json::Value jAttrs(Json::arrayValue);
    Json::Value jFiles(Json::arrayValue);
    Json::Value jCtArgs(Json::arrayValue);
    Json::Value jType;
}
@after {
    Json::Value jVal(Json::objectValue);
    jVal[J_TYPE] = jType;
    jVal[J_ARGS] = jAttrs;
    jVal[J_CARGS] = jCtArgs;
    jVal[J_FILE] = jFiles;
    jVal[J_TUPLES] = Json::UInt64( tuplesPerChunk );
    jVal[J_C_DEFS] = defs;
    jVal[J_OLD_TYPE] = giType;

    lT->AddGIWP(wp, attribs, jVal );
    lT->AddScanner(wp, curQuery); // DO WE NEED THIS?
    lT->AddEdgeFromBottom(wp);
}
    : ^(GI__
        textloaderAttributes[attribs,jAttrs]
        giFileList[jFiles]
        handleGIDef[jType]
        ctAttList[jCtArgs]
        textloaderChunkSize[tuplesPerChunk]
        )
    ;

handleGIDef[Json::Value& json]
    : giDef {
        $json = $giDef.json;
    }
    ;

giFileList[Json::Value& json]
    : ^(FILESPEC_LIST (giFileSpec[json])+ )
    ;

giFileSpec[Json::Value& json]
    : simpleFileSpec[json]
    | globFileSpec[json]
    ;

simpleFileSpec[Json::Value& jFiles]
@init{ std::string pat; int i = 0;}
    : ^(FILESPEC_SIMPLE pattern=STRING { pat = STRS($pattern); } (num=INT {i = atoi(TXT($num)); })? ) {
        if( i == 0 ) {
            /*std::cerr << "Adding file " << pat << " to GI file list" << std::endl;*/
            jFiles.append(pat);
        }
        else {
            for( int j = 1; j <= i; ++j ) {
                char buffer[1024];
                sprintf(buffer, pat.c_str(), j);
                std::string nStr(buffer);
                jFiles.append(nStr);
            }
        }
    }
    ;

globFileSpec[Json::Value& jFiles]
    : ^(FILESPEC_GLOB pattern=STRING) {
        const char * pat = TXTS($pattern);
        fprintf(stderr, "GLOB Pattern: \%s\n", pat);
        glob_t globRes;
        int ret = glob(pat, GLOB_ERR | GLOB_BRACE, NULL, &globRes );
        FATALIF(ret != 0, "Failed to match GLOB pattern \%s\n", pat);
        for( int i = 0; i < globRes.gl_pathc; ++i ) {
            fprintf(stderr, "GLOB Match: \%s\n", globRes.gl_pathv[i]);
            std::string nStr(globRes.gl_pathv[i]);
            jFiles.append(nStr);
        }
        globfree(&globRes);
    }
    ;

textloaderChunkSize[size_t& tuplesPerChunk]
    : /* nothing */ { tuplesPerChunk = 0; } // default
    | ^(TUPLE_COUNT n=INT)
        {
            tuplesPerChunk = atoi(TXT($n));
        }
    ;

textloaderAttributes[SlotContainer& attribs, Json::Value& json]
    :   ^(ATTFROM ID) {
            am.GetAttributesSlots(TXT($ID), attribs);

            FOREACH_TWL(sID, attribs) {
                std::string name = am.GetAttributeName( sID );
                json.append(Json_Attribute(Json_SourceInfo($ID), name));
            } END_FOREACH;
        }
    |   ^(ATTS
            (^(ATTWT name=ID ty=dType) {
                std::string type = $ty.type;

                SlotID nSlot = am.AddSynthesizedAttribute( curQuery, TXT($name), type, $ty.json);
                json.append(Json_Attribute(Json_SourceInfo($name), am.GetAttributeName(nSlot)));
                attribs.Append(nSlot);
            }
            | ^(ATT_SYNTH name=ID) {
                std::string n = STR($name);
                std::string prefix;
                qm.GetQueryShortName(curQuery, prefix);

                SlotID id = am.GetAttributeSlot(prefix, n);
                if( !id.IsValid() ) {
                    // Need to add a new attribute.
                    std::string type = "auto";
                    Json::Value jType(Json::nullValue);
                    id = am.AddSynthesizedAttribute(curQuery, n, type, jType);
                }

                json.append( Json_Attribute( Json_SourceInfo($name), am.GetAttributeName(id)) );
                attribs.Append(id);
            } )+
        )
    ;

wpDefinition
  : selectWP
    | joinWP
    | printWP
    | textloaderWP
    | glaWP
    | gtWP
    | gistWP
    | giWP
    | cacheWP
  ;

/*
selExpr[SlotContainer& atts, std::vector<WayPointID> & reqStateSources] returns [Json::Value json]
@init{ $json = Json::Value(Json::arrayValue); }
    : ( a=selExpression[atts, reqStateSources] { $json.append($a.json); } )+
    ;

selExpression[SlotContainer& atts, std::vector<WayPointID> & reqStateSources] returns [Json::Value json]
    : b=selExpressionBase[atts, reqStateSources] {$json = $b.json; }
    | ^(uo=FIL_UNARY $e=selExpression[atts, reqStateSources]) {
        std::string op = STR($uo);
        $json = Json_FilterUnary( Json_SourceInfo($uo), op, $e.json );
    }
    | ^(bo=FIL_BINARY left=selExpression[atts, reqStateSources] right=selExpression[atts, reqStateSources]) {
        std::string op = STR($bo);
        $json = Json_FilterBinary( Json_SourceInfo($bo), op, $left.json, $right.json );
    }
    ;

selExpressionBase[SlotContainer& atts, std::vector<WayPointID> & reqStateSources]
    returns [Json::Value json]
    : sExpr=expression[atts] { $json = $sExpr.json; }
    | gfExpression[atts, reqStateSources] { $json = $gfExpression.json; }
    ;

*/
gfExpression[SlotContainer& atts, std::vector<WayPointID> & reqStateSources] returns [Json::Value json]
@init {
    // JSON version of stuff
    Json::Value jCtArgs;
    Json::Value jStates;
}
    : ^(GF__
            ctAttList[jCtArgs]
            stateArgs[reqStateSources, jStates]
            nexp=namedExpressionList[atts]
            gfDef
        )
        {
           Json::Value jVal(Json::objectValue);
           jVal[J_TYPE] = $gfDef.json;
           jVal[J_ARGS] = $nexp.json;
           jVal[J_CARGS] = jCtArgs;
           jVal[J_SARGS] = jStates;

            $json = jVal;
        }
    ;

// Lists of named expressions
namedExpressionList[SlotContainer& atts] returns [Json::Value json]
@init{ Json::Value myJson(Json::arrayValue); }
@after{ $json = Json_NamedExpressionList( Json_SourceInfo( sourceFileName, 0, 0 ), myJson); }
    : /* nothing */
    | ^(NAMED_EXPR_LIST cont=namedExpressionListContents[atts]) { myJson = $cont.json; }
    ;

namedExpressionListContents[SlotContainer& atts] returns [Json::Value json]
@init{
    $json = Json::Value(Json::arrayValue);
    int nArgs = 0;
}
    : (b=namedExpression[atts, nArgs] { $json.append($b.json); ++nArgs; } )+
    ;

namedExpression[SlotContainer& atts, int nArgs] returns [Json::Value json]
    : ^(NAMED_EXPR n=expressionName[nArgs] ^(EXPR_VALUE a=expression[atts]))
    {
        $json = Json::Value(Json::objectValue);
        $json[J_NAME] = $n.name;
        $json[J_VAL] = $a.json;
    }
    ;

expressionName[int num] returns [std::string name]
    : /* nothing */ { $name = std::to_string(num); }
    | ^(EXPR_NAME i=ID) { $name = STR($i); }
    ;

expr[SlotContainer& atts] returns [Json::Value json]
@init { $json = Json::Value(Json::arrayValue); }
    :
    (a=expression[atts] { $json.append($a.json); } )+
    ;

// There could be more than one consts, hence we must have cstArray or something alike
expression[SlotContainer& atts] returns [Json::Value json]
@init {
    Json::Value jInfo(Json::arrayValue);
}
  :
    ^(QMARK a=expression[atts] b=expression[atts] c=expression[atts] ) {
        $json = Json_CaseSimple( Json_SourceInfo($QMARK), $a.json, $b.json, $c.json );
    }
  |
    ^(OPERATOR a=expression[atts] { jInfo.append($a.json); }
      b=expression[atts] { jInfo.append($b.json); } ) // binary
    {
        $json = Json_Operator( Json_SourceInfo($OPERATOR), STR($OPERATOR), jInfo );
    }
  | ^(UOPERATOR a=expression[atts] { jInfo.append($a.json); }) // unary
    {
      $json = Json_Operator( Json_SourceInfo($UOPERATOR), STR($UOPERATOR), jInfo );
    }

  | ^(src=FUNCTION i=identifier
        t=templateSpec
        (a=expression[atts] { jInfo.append($a.json);  } )*
    ) // Function
    {
         $json = Json_Function( Json_SourceInfo($src), $i.json, jInfo, $t.json );
    }
  | ^(src=METHOD
        ^(METHOD_NAME name=ID)
        ^(METHOD_OBJECT obj=expression[atts])
        (a=expression[atts] { jInfo.append($a.json); })*
    ) {
        $json = Json_Method( Json_SourceInfo($src), STR($name), $obj.json, jInfo );
    }
  | ^(MATCH_DP patt=STRING a=expression[atts] ) // pattern matcher
    {
        std::string pattern = STRN($patt);

        $json = Json_Match( Json_SourceInfo($MATCH_DP), pattern, $a.json );
   }
  | ^(src=CASE_EXPR b=case_expr_base[atts] t=case_expr_tests[atts] d=case_expr_default[atts]) {
    $json = Json_Case(Json_SourceInfo($src), $t.json, $d.json, $b.json);
    }
  | att=ATT  // Attribute
{
        std::string longName = (char*)$att.text->chars;
        SlotID slot = am.GetAttributeSlot(longName.c_str());
        FATALIF( !slot.IsValid(), "Attribute \%s does not exist, how did this happen?", longName.c_str());
        atts.Append(slot);

        $json = Json_Attribute( Json_SourceInfo($att), longName );
}
| litVal=literalValue
{
    $json = $litVal.json;
}
;

fragment case_expr_base[SlotContainer& atts] returns [Json::Value json]
    : /* nothing */
    | ^(CASE_BASE e=expression[atts]) {
        $json = $e.json;
    }
    ;

fragment case_expr_tests[SlotContainer& atts] returns [Json::Value json]
@init{ $json = Json::Value(Json::arrayValue); }
    : ( t=case_expr_test[atts] { $json.append($t.json); } )+
    ;

fragment case_expr_test[SlotContainer& atts] returns [Json::Value json]
    : ^(CASE_TEST t=expression[atts] v=expression[atts]) {
        $json = Json_CaseTest( $t.json, $v.json );
    }
    ;

fragment case_expr_default[SlotContainer& atts] returns [Json::Value json]
    : ^(CASE_DEFAULT v=expression[atts]) {
        $json = $v.json;
    }
    ;

literalValue returns [Json::Value json]
    : INT
    {
        std::string expr = STR($INT);
        std::string eType = "INT";
        if( expr[expr.size()-1] == 'L' ) // Literal long int
            eType = "BIGINT";

        $json = Json_Literal( Json_SourceInfo($INT), expr, eType );
    }
    | BOOL_T
    {
        std::string expr = STR($BOOL_T);
        boost::to_lower(expr);
        std::string eType = "bool";
        $json = Json_Literal( Json_SourceInfo($BOOL_T), expr, eType );
    }
    | STRING
    {
        std::string expr = STRN($STRING);
        std::string eType = "STRING_LITERAL";
        $json = Json_Literal( Json_SourceInfo($STRING), expr, eType );
    }
    | FLOAT
    {
        std::string expr = STR($FLOAT);
        std::string eType = "DOUBLE";
        if( expr[expr.size()-1] == 'f' ) // Literal float
            eType = "FLOAT";

        $json = Json_Literal( Json_SourceInfo($FLOAT), expr, eType );
    }
    | n=NULL_T { $json = Json_Null( Json_SourceInfo($n) ); }
    ;

objectParameters[Json::Value & putHere]
@init {
    Json::Value srcInfo;
}
    : /* nothing */ {
        Json::Value tmp(Json::objectValue);
        putHere = Json_JsonInline( Json_SourceInfo( sourceFileName, 0, 0 ), tmp );
    }
    | ^(open=PARAMETERS__ { srcInfo = Json_SourceInfo($open); }  objectParametersContent[putHere, srcInfo])
    ;

objectParametersContent[Json::Value & putHere, Json::Value sourceInfo]
@init{
    Json::Value tmp;
}
    : ^(JSON_INLINE jsonObject[tmp]) {
        putHere = Json_JsonInline( sourceInfo, tmp );
    }
    | ^(JSON_FILE fname=STRING) {
        std::string filename = STRS($fname);
        putHere = Json_JsonFile( sourceInfo, filename );
    }
    ;

jsonValue[Json::Value & putHere]
    : jsonObject[putHere]
    | jsonArray[putHere]
    | jsonSimple[putHere]
    ;

jsonObject[Json::Value & putHere]
@init{
    putHere = Json::Value(Json::objectValue);
}
    : ^(JSON_OBJECT (jsonObjectElem[putHere])* )
    ;

jsonObjectElem[Json::Value & putHere]
@init{
    Json::Value tmp;
}
    : ^(JSON_ELEM name=STRING value=jsonValue[tmp]) {
        std::string eName = STRS($name);
        putHere[eName] = tmp;
    }
    ;

jsonArray[Json::Value & putHere]
@init {
    putHere = Json::Value(Json::arrayValue);
}
    : ^(JSON_ARRAY (jsonArrayElem[putHere])* )
    ;

jsonArrayElem[Json::Value & putHere]
@init {
    Json::Value tmp;
}
    : jsonValue[tmp] {
        putHere.append(tmp);
    }
    ;

jsonSimple[Json::Value & putHere]
    : i=INT {
        std::string eVal = STR($i);
        long val = stol( eVal );
        putHere = (Json::Int64) val;
    }
    | b=BOOL {
        std::string eVal = STR($b);
        boost::to_lower(eVal);

        if( eVal == "true" ) {
            putHere = true;
        } else {
            putHere = false;
        }
    }
    | f=FLOAT {
        std::string eVal = STR($f);
        double val = stod( eVal );
        putHere = val;
    }
    | s=STRING {
        std::string eVal = STRS($s);
        putHere = eVal;
    }
    | n=NULL_T {
        putHere = Json::Value(Json::nullValue);
    }
    ;
