#include "JsonAST.h"
#include "Errors.h"
#include <boost/algorithm/string.hpp>

Json::Value Json_SourceInfo( std::string filename, int lineNo, int colNo ) {
    Json::Value info(Json::objectValue);
    info[J_FILE] = filename;
    info[J_LINE] = lineNo;
    info[J_COL] = colNo;

    return info;
}

Json::Value Json_Node( Json::Value sourceInfo, std::string type, Json::Value & data ) {
    Json::Value nVal(Json::objectValue);

    nVal[J_NODE_TYPE] = type;
    nVal[J_NODE_DATA] = data;
    nVal[J_NODE_SOURCE] = sourceInfo;

    return nVal;
}

Json::Value Json_DataType( Json::Value sourceInfo,  std::string name, Json::Value t_args,
        Json::Value alias) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = name;
    data[J_TARGS] = t_args;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_DT, data );
}

Json::Value Json_Reference( Json::Value sourceInfo, std::string ref ) {
    Json::Value data = ref;
    return Json_Node( sourceInfo, JN_REFERENCE, data );
}

Json::Value Json_Functor( Json::Value sourceInfo, std::string name, Json::Value args ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = name;
    data[J_ARGS] = args;

    return Json_Node( sourceInfo, JN_FUNCTOR, data );
}

Json::Value Json_Literal( Json::Value sourceInfo, std::string value, std::string type ) {
    Json::Value data(Json::objectValue);
    data[J_VAL] = value;
    data[J_TYPE] = type;

    return Json_Node( sourceInfo, JN_LIT, data );
}

Json::Value Json_Null( Json::Value sourceInfo ) {
    Json::Value data(Json::nullValue);
    return Json_Node( sourceInfo, JN_NULL, data );
}

Json::Value Json_Attribute( Json::Value sourceInfo, std::string name ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = name;

    return Json_Node( sourceInfo, JN_ATT, data );
}

Json::Value Json_CaseTest( Json::Value test, Json::Value expr ) {
    Json::Value data(Json::objectValue);
    data[J_TEST] = test;
    data[J_EXPR] = expr;

    return data;
}

Json::Value Json_Case( Json::Value sourceInfo, Json::Value tests,
    Json::Value defaultExpr, Json::Value baseExpr ) {

    Json::Value data(Json::objectValue);
    data[J_BASE] = baseExpr;
    data[J_CASES] = tests;
    data[J_DEFAULT] = defaultExpr;

    return Json_Node( sourceInfo, JN_CASE, data );
}

Json::Value Json_CaseSimple( Json::Value sourceInfo, Json::Value test, Json::Value trueExpr, Json::Value falseExpr ) {
    Json::Value tests(Json::arrayValue);

    tests.append(Json_CaseTest(test, trueExpr));

    return Json_Case( sourceInfo, tests, falseExpr );
}

Json::Value Json_Match( Json::Value sourceInfo, std::string pattern, Json::Value expr ) {
    Json::Value data(Json::objectValue);
    data[J_PATT] = pattern;
    data[J_EXPR] = expr;

    return Json_Node( sourceInfo, JN_MATCH, data );
}

Json::Value Json_Function( Json::Value sourceInfo, Json::Value name, Json::Value args, Json::Value tArgs ) {

    Json::Value data(Json::objectValue);
    data[J_NAME] = name;
    data[J_ARGS] = args;
    data[J_TARGS] = tArgs;

    return Json_Node( sourceInfo, JN_FUNC, data );
}

Json::Value Json_Method( Json::Value sourceInfo, std::string name, Json::Value obj, Json::Value args ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = name;
    data[J_VAL] = obj;
    data[J_ARGS] = args;

    return Json_Node( sourceInfo, JN_METHOD, data );
}

Json::Value Json_Operator( Json::Value sourceInfo, std::string name, Json::Value args ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = name;
    data[J_ARGS] = args;

    return Json_Node( sourceInfo, JN_OP, data );
}

Json::Value Json_Identifier( Json::Value sourceInfo, std::string value ) {
    Json::Value data;
    data = value;

    return Json_Node( sourceInfo, JN_IDENTIFIER, data );
}

Json::Value Json_TypeOf( Json::Value sourceInfo, Json::Value attName ) {
    Json::Value data = attName;
    return Json_Node( sourceInfo, JN_TYPEOF, data );
}

Json::Value Json_GLA( Json::Value sourceInfo, std::string name, Json::Value tArgs,
        Json::Value alias ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TARGS] = tArgs;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_GLA, data );
}

Json::Value Json_GF( Json::Value sourceInfo, std::string name, Json::Value tArgs,
        Json::Value alias ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TARGS] = tArgs;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_GF, data );
}

Json::Value Json_GT( Json::Value sourceInfo, std::string name, Json::Value tArgs,
        Json::Value alias ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TARGS] = tArgs;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_GT, data );
}

Json::Value Json_GIST( Json::Value sourceInfo, std::string name, Json::Value tArgs,
        Json::Value alias ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TARGS] = tArgs;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_GIST, data );
}

Json::Value Json_GI( Json::Value sourceInfo, std::string name, Json::Value tArgs,
        Json::Value alias ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TARGS] = tArgs;
    data[J_ALIAS] = alias;

    return Json_Node( sourceInfo, JN_GI, data );
}

Json::Value Json_ExprWrap( Json::Value expr, std::string cExpr, std::string cstStr, std::string defs ) {
    Json::Value data(Json::objectValue);

    data[J_EXPR] = expr;
    data[J_C_EXPR] = cExpr;
    data[J_CST_STR] = cstStr;
    data[J_C_DEFS] = defs;

    return data;
}

Json::Value Json_Define( Json::Value sourceInfo, std::string name, Json::Value type ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = name;
    data[J_TYPE] = type;

    return Json_Node( sourceInfo, JN_DEFINE, data );
}

Json::Value Json_Import( Json::Value sourceInfo, Json::Value ident ) {
    return Json_Node( sourceInfo, JN_IMPORT, ident );
}

Json::Value Json_State( Json::Value sourceInfo, std::string source, Json::Value type ) {
    Json::Value data(Json::objectValue);

    data[J_NAME] = source;
    data[J_TYPE] = type;

    return Json_Node( sourceInfo, JN_STATE, data);
}

Json::Value Json_Waypoint( std::string nType, Json::Value payload ) {
    // Waypoints can be defined in multiple queries, so they have no source info.
    return Json_Node(Json::Value(Json::nullValue), nType, payload );
}

Json::Value Json_NamedArgs( Json::Value sourceInfo, Json::Value payload ) {
    if( payload.isNull() ) {
        // I have no idea why the payload is null sometimes. It only seems to
        // happen when an empty array is passed as a template argument, so
        // just make the payload an empty array in this case.
        payload = Json::Value(Json::arrayValue);
    }
    return Json_Node(sourceInfo, JN_NAMED_ARGS, payload);
}

Json::Value Json_NamedExpressionList( Json::Value sourceInfo, Json::Value payload ) {
    return Json_Node(sourceInfo, JN_NAMED_EXPRS, payload );
}

Json::Value Json_FilterUnary( Json::Value sourceInfo, std::string op, Json::Value expr ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = op;
    data[J_ARGS] = expr;

    return Json_Node( sourceInfo, JN_FIL_UNARY, data );
}

Json::Value Json_FilterBinary( Json::Value sourceInfo, std::string op, Json::Value left, Json::Value right ) {
    Json::Value data(Json::objectValue);
    data[J_NAME] = op;

    Json::Value args(Json::arrayValue);
    args.append(left);
    args.append(right);

    data[J_ARGS] = args;

    return Json_Node( sourceInfo, JN_FIL_BINARY, data );
}

Json::Value Json_JsonInline( Json::Value sourceInfo, Json::Value jVal ) {
    return Json_Node( sourceInfo, JN_JSON_INLINE, jVal );
}

Json::Value Json_JsonFile( Json::Value sourceInfo, std::string filename ) {
    Json::Value data = filename;
    return Json_Node( sourceInfo, JN_JSON_FILE, data);
}
