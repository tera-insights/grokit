<?php
namespace grokit {

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * Functions to parse expressions.
 */

function parseNamedArgValue( $ast ) {
    assert_ast_type( $ast, [NodeType::LITERAL,
        NodeType::IDENTIFIER,
        NodeType::NAMED_ARGS,
        NodeType::TYPEOF,
        NodeType::DT,
        NodeType::GLA,
        NodeType::GF,
        NodeType::GT,
        NodeType::GIST,
        NodeType::GI,
        NodeType::NUL,
        NodeType::FUNCTOR,
        NodeType::ATTRIBUTE,
        ] );

    $source = ast_node_source( $ast );

    switch( ast_node_type($ast) ) {
    case NodeType::LITERAL:
        return parseLiteralTemplate( $ast );
        break;
    case NodeType::IDENTIFIER:
        return new Identifier(parseIdentifier($ast));
        break;
    case NodeType::DT:
    case NodeType::TYPEOF:
        return parseType($ast);
        break;
    case NodeType::ATTRIBUTE:
        return parseAttribute($ast);
        break;
    case NodeType::NAMED_ARGS:
        return parseNamedArgList($ast);
        break;
    case NodeType::FUNCTOR:
        return parseFunctor($ast);
        break;
    case NodeType::GLA:
        return parseGLA($ast);
        break;
    case NodeType::GF:
        return parseGF($ast);
        break;
    case NodeType::GT:
        return parseGT($ast);
        break;
    case NodeType::GIST:
        return parseGIST($ast);
        break;
    case NodeType::GI:
        return parseGI($ast);
        break;
    case NodeType::NUL:
        return null;
        break;
    }
}

function parseNamedArg( $ast, &$list ) {
    $name = ast_get($ast, NodeKey::NAME);
    $value = parseNamedArgValue(ast_get($ast, NodeKey::VALUE));

    $list[$name] = $value;
}

function parseNamedArgListContents( array &$arg_list ) {
    $list = [];

    foreach( $arg_list as $val ) {
        parseNamedArg( $val, $list );
    }

    return $list;
}


function parseNamedArgList( $ast ) {
    assert_ast_type( $ast, NodeType::NAMED_ARGS );

    $arg_list = ast_node_data( $ast );
    $list = parseNamedArgListContents( $arg_list );

    return $list;
    //return new TemplateArg( ast_node_source($ast), TemplateArg::NAMED_LIST, $list );
}

/*
 * Node can be null. Otherwise, expect it to be a named argument list
 */
function parseTemplateArgs( $ast ) {
    // Null if no template args
    if( $ast === null )
        return null;

    return parseNamedArgListContents( $ast );
}

function parseFunctor( $ast ) {
    assert_ast_type( $ast, NodeType::FUNCTOR );

    $data = ast_node_data( $ast );

    $name = ast_get( $data, NodeKey::NAME );
    $args = parseNamedArgList( ast_get( $data, NodeKey::ARGS ) );

    return new Functor( ast_node_source($ast), $name, $args );
}

function parseTypeOf( $ast ) {
    assert_ast_type( $ast, NodeType::TYPEOF );

    $data = ast_node_data($ast);

    assert_ast_type( $data, NodeType::ATTRIBUTE );

    $attr_info = parseAttribute( $data );
    return $attr_info->type();
}

function parseIdentifier( $ast ) {
    assert_ast_type( $ast, NodeType::IDENTIFIER );
    return ast_node_data( $ast );
}

function parseDataType( $ast ) {
    assert_ast_type( $ast, NodeType::DT );
    $data = ast_node_data($ast);

    $name = ast_get($data, NodeKey::NAME);
    $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
    $alias = ast_get($data, NodeKey::ALIAS);
    $alias = $alias === null ? null : parseIdentifier($alias);

    //fwrite( STDERR, "parseDT: name($name) t_args(" . squashToString($t_args) . ")\n");

    return lookupType($name, $t_args, $alias);
}

function parseReference( $ast ) {
    assert_ast_type( $ast, NodeType::REFERENCE );

    $name = ast_node_data($ast);

    $info = null;
    try {
        $info = lookupAlias( $name );
    }
    catch( Exception $e ) {
        grokit_error( 'Failed to lookup reference ' . $name . ' from ' . $source,
            $e);
    }

    return $info;
}

function parseType( $ast ) {
    assert_ast_type( $ast, [NodeType::DT, NodeType::IDENTIFIER, NodeType::TYPEOF, NodeType::REFERENCE] );
    $ast_type = ast_node_type($ast);
    $source = ast_node_source( $ast );

    switch( $ast_type ) {
    case NodeType::TYPEOF:
        return parseTypeOf( $ast );
        break;
    case NodeType::IDENTIFIER:
        $name = parseIdentifier( $ast );
        return lookupType( $name, [] );
        break;
    case NodeType::DT:
        $info = parseDataType( $ast );

        grokit_assert( $info->isDataType(),
            'Performed lookup expecting DataType, got ' . $info->kind() . ' instead ' . $source );

        return $info;
        break;
    case NodeType::REFERENCE:
        $info = parseReference( $ast );

        grokit_assert( $info->isDataType(),
            'Performed alias lookup expecting DataType, got ' . $info->kind() . ' instead ' . $source );

        return $info;
        break;
    }
}

function parseLiteralTemplate( $ast ) {
    assert_ast_type( $ast, NodeType::LITERAL );

    $data = ast_node_data($ast);
    $value = ast_get($data, NodeKey::VALUE);
    $type_node = ast_get($data, NodeKey::TYPE);
    $source = ast_node_source($ast);

    switch( $type_node ) {
    case 'STRING_LITERAL':
        $value = \trim($value, '"');
        //$type = TemplateArg::STRING;
        break;
    case 'INT':
    case 'BIGINT':
        $value = \intval($value);
        //$type = TemplateArg::INT;
        break;
    case 'DOUBLE':
    case 'FLOAT':
        $value = \floatval($value);
        //$type = TemplateArg::FLOAT;
        break;
    case 'bool':
        $value = $value == 'true' ? true : false;
        //$type = TemplateArg::BOOL;
        break;
    default:
        grokit_logic_error( 'Unknown literal type ' . $type_node . ' from ' . $source );
    }

    return $value;
    //return new TemplateArg( $source, $type, $value );
}

function parseLiteral( $ast ) {
    assert_ast_type( $ast, NodeType::LITERAL );

    $data = ast_node_data( $ast );
    $value = ast_get($data, NodeKey::VALUE);
    $type_node = ast_get($data, NodeKey::TYPE);

    $type = lookupType( 'base::' . $type_node );

    $source = ast_node_source($ast);

    $info = new ExpressionInfo($source, $type, $value, true);

    return $info;
}

function parseLiteralList( $ast ) {
    grokit_logic_assert( is_array($ast),
        'Got value of type ' . gettype($ast) . ' expected array');

    $list = [];
    foreach( $ast as $lit ) {
        $list[] = parseLiteral($lit);
    }

    return $list;
}

// This function is used by the waypoint on the top-level GI in order to
// instantiate it.
function parseGI( $ast ) {
    assert_ast_type( $ast, [ NodeType::GI, NodeType::REFERENCE ] );

    $type = ast_node_type($ast);
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    switch( $type ) {
    case NodeType::GI:
        $name = ast_get($data, NodeKey::NAME);
        $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
        $alias = ast_has($data, NodeKey::ALIAS) ? null : parseIdentifier(ast_get($data, NodeKey::ALIAS));

        return new GI_Spec( $source, $name, $t_args, $alias);
        break;
    case NodeType::REFERENCE:
        $info = parseReference($ast);
        grokit_assert( $info->isGI(),
            'Tried to look up reference as a GI, got a ' . $info->kind() .
            'instead ' . $source );

        return $info;
        break;
    }
}

// This function is used by the waypoint on the top-level GI in order to
// instantiate it.
function parseGF( $ast ) {
    assert_ast_type( $ast, [ NodeType::GF, NodeType::REFERENCE ] );

    $type = ast_node_type($ast);
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    switch( $type ) {
    case NodeType::GF:
        $name = ast_get($data, NodeKey::NAME);
        $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
        $alias = ast_has($data, NodeKey::ALIAS) ? null : parseIdentifier(ast_get($data, NodeKey::ALIAS));

        return new GF_Spec( $source, $name, $t_args, $alias );
        break;
    case NodeType::REFERENCE:
        $info = parseReference($ast);
        grokit_assert( $info->isGF(),
            'Tried to look up reference as a GF, got a ' . $info->kind() .
            'instead ' . $source );

        return $info;
        break;
    }
}

function parseGLA( $ast ) {
    assert_ast_type( $ast, [ NodeType::GLA, NodeType::REFERENCE ] );

    $type = ast_node_type($ast);
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    switch( $type ) {
    case NodeType::GLA:
        $name = ast_get($data, NodeKey::NAME);
        $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
        $alias = ast_has($data, NodeKey::ALIAS) ? null : parseIdentifier(ast_get($data, NodeKey::ALIAS));

        return new GLA_Spec( $source, $name, $t_args, $alias);
        break;
    case NodeType::REFERENCE:
        $info = parseReference($ast);
        grokit_assert( $info->isGLA(),
            'Tried to look up reference as a GLA, got a ' . $info->kind() .
            'instead ' . $source );

        return $info;
        break;
    }
}

function parseGIST( $ast ) {
    assert_ast_type( $ast, [ NodeType::GIST, NodeType::REFERENCE ] );

    $type = ast_node_type($ast);
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    switch( $type ) {
    case NodeType::GIST:
        $name = ast_get($data, NodeKey::NAME);
        $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
        $alias = ast_has($data, NodeKey::ALIAS) ? null : parseIdentifier(ast_get($data, NodeKey::ALIAS));

        return new GIST_Spec( $source, $name, $t_args, $alias);
        break;
    case NodeType::REFERENCE:
        $info = parseReference($ast);
        grokit_assert( $info->isGIST(),
            'Tried to look up reference as a GIST, got a ' . $info->kind() .
            'instead ' . $source );

        return $info;
        break;
    }
}

function parseGT( $ast ) {
    assert_ast_type( $ast, [ NodeType::GT, NodeType::REFERENCE ] );

    $type = ast_node_type($ast);
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    switch( $type ) {
    case NodeType::GT:
        $name = ast_get($data, NodeKey::NAME);
        $t_args = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
        $alias = ast_has($data, NodeKey::ALIAS) ? null : parseIdentifier(ast_get($data, NodeKey::ALIAS));

        return new GT_Spec( $source, $name, $t_args, $alias);
        break;
    case NodeType::REFERENCE:
        $info = parseReference($ast);
        grokit_assert( $info->isGT(),
            'Tried to look up reference as a GT, got a ' . $info->kind() .
            'instead ' . $source );

        return $info;
        break;
    }
}

function parseAttribute( $ast ) {
    assert_ast_type( $ast, NodeType::ATTRIBUTE );

    $data = ast_node_data( $ast );
    $name = ast_get($data, NodeKey::NAME);

    // Look up attribute information in Attribute Manager
    return lookupAttribute($name);
}

function parseAttributeList( $ast ) {
    grokit_logic_assert( is_array($ast),
        'Got value of type ' . gettype($ast) . ' expected array');

    $list = [];

    foreach( $ast as $attr ) {
        $attrTmp = parseAttribute($attr);
        $list[$attrTmp->name()] = $attrTmp;
    }

    return $list;
}

function parseAttributeExpr( $ast ) {
    $attr_info = parseAttribute( $ast );

    $name = $attr_info->name();
    $type = $attr_info->type();
    $source = ast_node_source($ast);
    $expr = $name;

/*
    if( $type->is('__state__') ) {
        // Dealing with a state wrapper, so we need to transform it to the inner type.
        $innerType = $type->get('type');
        $expr = '(*(' . $name . '.GetObject()))';
        $type = $innerType;
    }
*/
    return new ExpressionInfo($source, $type, $expr, false);
}

function parseAttributeListExpr( $ast ) {
    grokit_logic_assert( is_array($ast),
        'Got value of type ' . gettype($ast) . ' expected array');

    $list = [];

    foreach( $ast as $attr ) {
        $list[] = parseAttributeExpr($attr);
    }

    return $list;
}

function parseFunction( $ast ) {
    assert_ast_type( $ast, NodeType::FUNC );

    $data = ast_node_data($ast);
    $name = parseIdentifier(ast_get($data, NodeKey::NAME));
    $args = parseExpressionList(ast_get($data, NodeKey::ARGS));
    $targs = parseTemplateArgs(ast_get($data, NodeKey::TARGS));
    $targs = is_null($targs) ? [] : $targs;
    $source = ast_node_source($ast);

    $arg_types = array_map(function($item) { return $item->type(); }, $args );
    try{
        $func = lookupFunction( $name, $arg_types, $targs );
    }
    catch( Exception $e ) {
        grokit_error( 'Failed to find function ' . $name . ' called from ' . $source, $e );
    }

    $info = $func->apply($args, $source);

    // If the function is deterministic and all of the expressions were constant,
    // just turn this expression into a constant.
    if( $info->is_const() ) {
        $info->makeConstant();
    }

    return $info;
}

function parseMethod( $ast ) {
    assert_ast_type( $ast, NodeType::METHOD );
    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    $name = ast_get($data, NodeKey::NAME);
    $obj = parseExpression(ast_get($data, NodeKey::VALUE));
    $args = parseExpressionList(ast_get($data, NodeKey::ARGS));

    $arg_types = extractTypes($args);
    $obj_type = $obj->type();

    try {
        $mInfo = $obj_type->lookupMethod( $name, $arg_types );
    }
    catch( Exception $e ) {
        grokit_error( 'Failed to find method ' . $obj_type . '->' . $name . ' with args (' .
            implode(', ', $arg_types) . ') called from ' . $source, $e );
    }

    //fwrite(STDERR, "Calling apply for method {$obj}->{$name} with args: " . print_r($args, true) . PHP_EOL);
    $info = $mInfo->apply($obj, $args, $source);
    if( $info->is_const() ) {
        $info->makeConstant();
    }

    return $info;
}

function parseOperator( $ast ) {
    assert_ast_type( $ast, NodeType::OPERATOR );

    $source = ast_node_source( $ast );
    $data = ast_node_data($ast);
    $name = ast_get( $data, NodeKey::NAME );
    $args = parseExpressionList( ast_get( $data, NodeKey::ARGS ) );

    // Special case for integer divison
    if( $name == "/" && \count($args) == 2 && ($args[0]->type()->is('integral') || $args[1]->type()->is('integral'))) {
        // We have division, and at least 1 argument is an integer. Force both
        // to be doubles.
        $dblType = lookupType('BASE::DOUBLE');
        $args[0] = convertExpression( $args[0], $dblType, $args[0]->source());
        $args[1] = convertExpression( $args[1], $dblType, $args[1]->source());
    }

    $arg_types = array_map(function($item) { return $item->type(); }, $args );
    try{
        $func = lookupOperator( $name, $arg_types );
    }
    catch( Exception $e ) {
        grokit_error( 'Failed to find operator ' . $name . ' called from ' . $source, $e );
    }

    $info = $func->apply($args, $source);

    // If the function is deterministic and all of the expressions were constant,
    // just turn this expression into a constant.
    if( $info->is_const() ) {
        $info->makeConstant();
    }

    return $info;
}

function parseMatch( $ast ) {
    assert_ast_type( $ast, NodeType::MATCH );
    $expectedType = lookupType('base::STRING_LITERAL');
    $data = ast_node_data($ast);
    $pattern = ast_get($data, NodeKey::PATTERN);
    $expr = parseExpression(ast_get($data, NodeKey::EXPR));
    $type = lookupType('bool');
    $source = ast_node_source($ast);

    grokit_assert( $expr->type() == $expectedType,
        'Match expected expression of type STRING_LITERAL, got ' . $expr->type()
        . ' from ' . $expr->source()
    );

    $libman = & LibraryManager::GetLibraryManager();

    // Set up a constant for the pattern matcher.
    $regex_name = generate_name('_regex');

/*
    // STL
    $const = 'const std::regex ' . $regex_name . '(' . $pattern . ', std::regex_constants::ECMAScript | std::regex_constants::optimize );';
    $value = 'std::regex_match(' . $expr->value() . ', ' . $regex_name . ')';

    $libman->addHeader('<regex>');
*/

    // Oniguruma
    $const = 'const PatternMatcherOnig ' . $regex_name . '(' . $pattern . ');';
    $value = $regex_name . '.Match(' . $expr->value() . ')';

    $libman->addHeader('"PatternMatcherOnig.h"');

    // Create the new expression, taking any constants required by the inner
    // expression and then adding the one we just created.
    $ret = new ExpressionInfo($source, $type, $value, false);
    $ret->absorbMeta($expr);
    $ret->addConstant($const);

    return $ret;
}

function parseCaseBase( &$source, &$base, &$cases, &$default ) {
    $base_name = generate_name("case_base");
    $base_value = $base->type() . " " . $base_name . " = "
        . $base->value() . ";";

    $retType = null;
    $retSource = null;

    $prep = [];
    $value_name = generate_name("case_value");

    $info = new ExpressionInfo( $source, null, $value_name, true);
    $info->absorbMeta($base);

    grokit_logic_assert( count($cases) > 0,
        'No cases found for case statement with base at ' . $source );

    // Handle cases
    foreach( $cases as $case ) {
        $test = parseExpression(ast_get($case, NodeKey::TEST));
        $expr = parseExpression(ast_get($case, NodeKey::EXPR));
        $first = false;

        // TODO: Test if == operator exists between test and base

        // If the return type is not set, set it.
        // Otherwise, make sure that the expression's return type is compatible
        // with the already set return type.
        if( $retType === null ) {
            $retType = $expr->type();
            $retSource = $expr->source();
            $first = true;
            $info->setType($retType);
        }
        else if( canConvert( $expr->type(), $retType ) ) {
            // They aren't the same but we can perform an implicit conversion
            // to make them the same, so do so.
            $expr = convertExpression( $expr, $retType, $retSource );
        }
        else {
            // Incompatible types.
            grokit_error( 'Case return type ' . $expr->type() . ' of expression at '
                . $expr->source() . ' incompatible with previous return type '
                . $retType . ' defined by expression at ' . $retSource  );
        }

        $info->absorbMeta($test);
        $info->absorbMeta($expr);

        $myPrep = '';
        if( ! $first ) {
            $myPrep .= 'else ';
        }
        $myPrep .= "if( {$base_name} == {$test->value()} ) $value_name = {$expr->value()};";

        $prep[] = $myPrep;
    }

    // Handle default
    if( $default !== null ) {
        if( canConvert( $default->type(), $retType ) ) {
            $default = convertExpression( $default, $retType, $retSource );
        }
        else {
            // Incompatible types.
            grokit_error( 'Case return type ' . $default->type() . ' of default at '
                . $default->source() . ' incompatible with previous return type '
                . $retType . ' defined by expression at ' . $retSource );
        }

        $info->absorbMeta($default);

        $prep[] = "else $value_name = {$default->value()};";
    }

    // Prepend the declaration of the variable that holds the return value of the case.
    array_unshift( $prep, "{$retType} {$value_name};" );

    // If base expression is constant, add to constants, otherwise to preprocessing.
    if( $base->is_const() ) {
        $info->addConstant( $base_value );
    }
    else {
        $info->addPreprocess( $base_value );
    }

    $info->addPreprocesses( $prep );

    // If everything was constant, transform the expression into a constant
    if( $info->is_const() ) {
        $info->makeConstant();
    }

    $testRetType = lookupType('bool');
}

function parseCaseNoBase( &$source, &$cases, &$default ) {
    // The return type of the tests must be boolean
    $testRetType = lookupType('bool');

    // We don't know the return type yet, it will be defined by the cases.
    $retType = null;
    $retSource = null;

    // Generate a name for the return value of the case.
    $value_name = generate_name("case_value");

    $prep = [];

    $info = new ExpressionInfo( $source, null, $value_name, true);

    grokit_logic_assert( count($cases) > 0,
        'No cases found for case statement at ' . $source );

    // Handle cases
    foreach( $cases as $case ) {
        $test = parseExpression(ast_get($case, NodeKey::TEST));
        $expr = parseExpression(ast_get($case, NodeKey::EXPR));
        $first = false;

        // Test if the return type of the test is compatible with boolean
        if( canConvert( $test->type(), $testRetType ) ) {
            $test = convertExpression( $test, $testRetType, $retSource );
        }
        else {
            // Incompatible types
            grokit_error( 'Case test expression has return type ' . $test->type()
                . ' which is incompatible with boolean ' . $test->source() );
        }

        // If the return type is not set, set it and continue.
        // Otherwise, make sure the expression's return type is compatible with
        // the already set return type.
        if( $retType === null ) {
            $retType = $expr->type();
            $retSource = $expr->source();
            $first = true;
            $info->setType($retType);
        }
        else if( canConvert( $expr->type(), $retType ) ) {
            // The types are compatible or the same, so make them the same.
            $expr = convertExpression( $expr, $retType, $retSource );
        }
        else {
            // Incompatible types
            grokit_error( 'Case return type ' . $expr->type() . ' of expression at '
                . $expr->source() . ' incompatible with previous return type '
                . $retType . ' defined by expression at ' . $retSource  );
        }

        // Absorb the metadata from the test and expression into our info
        $info->absorbMeta($test);
        $info->absorbMeta($expr);

        $myPrep = '';
        if( ! $first ) {
            $myPrep .= 'else ';
        }
        $myPrep .= "if( {$test->value()} ) {$value_name} = {$expr->value()};";

        $prep[] = $myPrep;
    }

    // Handle default
    if( $default !== null ) {
        if( canConvert( $default->type(), $retType ) ) {
            $default = convertExpression( $default, $retType, $retSource );
        }
        else {
            // Incompatible types.
            grokit_error( 'Case return type ' . $default->type() . ' of default at '
                . $default->source() . ' incompatible with previous return type '
                . $retType . ' defined by expression at ' . $retSource );
        }

        $info->absorbMeta($default);

        $prep[] = "else $value_name = {$default->value()};";
    }

    // Prepend the declaration of the return variable
    array_unshift( $prep, "{$retType} {$value_name};" );

    // Add all of our stuff as preprocesses
    $info->addPreprocesses($prep);

    if( $info->is_const() ) {
        $info->makeConstant();
    }

    return $info;
}

function parseCase( $ast ) {
    assert_ast_type( $ast, NodeType::CASE_NODE );

    $data = ast_node_data($ast);
    $source = ast_node_source($ast);

    $base_ast = ast_get($data, NodeKey::BASE);
    $cases = ast_get($data, NodeKey::CASES);
    $default_ast = ast_get($data, NodeKey::DEFAULT_CASE);

    // TODO: Allow case to handle no default if we add in null values.
    grokit_assert( $default_ast !== null,
        'Case statements with no default currently unsupported '
        . ast_node_source($ast));

    grokit_logic_assert( is_array($cases),
        'Cases attribute of CASE statement was not an array! '
        . ast_node_source($ast));

    $default = parseExpression($default_ast);

    if( $base_ast === null ) {
        return parseCaseNoBase( $source, $cases, $default );
    }
    else {
        $base = parseExpression($base_ast);
        return parseCaseBase( $source, $base, $cases, $default );
    }
}

function parseNull( $ast ) {
    assert_ast_type( $ast, Nodetype::NUL );

    $nType = lookupType('BASE::NULL');
    $nVal = 'GrokitNull::Value';

    $source = ast_node_source($ast);

    $info = new ExpressionInfo($source, $nType, $nVal, true);

    return $info;
}

// This is basically a dispatch function for the various types of expressions
function parseExpression( $ast ) {
    $type = ast_node_type($ast);

    switch($type) {
    case NodeType::LITERAL:
        $ret = parseLiteral($ast);
        break;
    case NodeType::ATTRIBUTE:
        $ret = parseAttributeExpr($ast);
        break;
    case NodeType::FUNC:
        $ret = parseFunction($ast);
        break;
    case Nodetype::METHOD:
        $ret = parseMethod($ast);
        break;
    case NodeType::OPERATOR:
        $ret = parseOperator($ast);
        break;
    case NodeType::CASE_NODE:
        $ret = parseCase($ast);
        break;
    case NodeType::MATCH:
        $ret = parseMatch($ast);
        break;
    case NodeType::NUL:
        $ret = parseNull($ast);
        break;
    default:
        $source = ast_node_source($ast);
        grokit_logic_error('Attempted to evaluate node of type ' . $type .
            ' as an expression ' . $source);
    }

    return $ret;
}

function parseExpressionList( $ast ) {
    $list = [];

    grokit_logic_assert( is_array($ast),
        'Attempted to parse value of type ' . gettype($ast) . ' as an array.');

    foreach( $ast as $expr ) {
        $list[] = parseExpression($expr);
    }

    return $list;
}

function parseNamedExpression( $ast, &$list ) {
    $name = ast_get($ast, NodeKey::NAME);
    $value = parseExpression(ast_get($ast, NodeKey::VALUE));

    if( is_numeric($name) ) {
        $name = '_expr' . $name . '_';
    }

    $list[$name] = $value;
}

function parseNamedExpressionList( $ast ) {
    assert_ast_type( $ast, NodeType::NAMED_EXPR_LIST );

    $list = [];
    $data = ast_node_data($ast);

    foreach( $data as $nexpr ) {
        parseNamedExpression( $nexpr, $list );
    }

    return $list;
}

// Helper function to extract types from a list of expressions
function extractTypes( array &$expList ) {
    $list = [];

    foreach( $expList as $key => $expr ) {
        $list[$key] = $expr->type();
    }

    return $list;
}

function parseJsonInline( $ast ) {
    assert_ast_type( $ast, NodeType::JSON_INLINE );

    $value = ast_node_data($ast);
    $source = ast_node_source($ast);

    return new JsonLiteral($source, $value);
}

function parseJsonFile($ast) {
    assert_ast_type( $ast, NodeType::JSON_FILE );

    $value = ast_node_data($ast);
    $source = ast_node_source($ast);

    return new JsonFile($source, $value);
}

function parseJsonAst( $ast ) {
    assert_ast_type( $ast, [NodeType::JSON_INLINE, NodeType::JSON_FILE] );

    $type = ast_node_type($ast);

    switch($type) {
    case NodeType::JSON_INLINE:
        return parseJsonInline($ast);
        break;
    case NodeType::JSON_FILE:
        return parseJsonFile($ast);
        break;
    }
}

}
?>
