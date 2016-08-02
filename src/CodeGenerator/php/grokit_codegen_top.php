<?

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file serves as the overall interface to the code generation facilities.
 *
 * It also contains the definitions for parsing the program-level AST nodes.
 */

namespace grokit {

    class StateRegistry {
        private static $states = [];

        public static function addState($wp, $query, $type) {
            if( !array_key_exists($wp, self::$states) )
                self::$states[$wp] = [];

            self::$states[$wp][$query] = $type;
        }

        public static function getState($wp, $query) {
            grokit_assert(array_key_exists($wp, self::$states), 'No states available for waypoint ' . $wp);
            grokit_assert(array_key_exists($query, self::$states[$wp]), 'No state available for query ' . $query . ' in waypoint ' . $wp);
            return self::$states[$wp][$query];
        }
    }

    /********** Helper Methods **********/

    function _startFile( $file ) {
        //fwrite(STDOUT, 'Starting file ' . $file . PHP_EOL);

        // Start output buffering
        ob_start();

        // Tell the library manager to save the previous state and make
        // a copy for this file.
        LibraryManager::Push();
    }

    function _endFile( $file, $headers ) {
        //fwrite(STDOUT, 'Ending file ' . $file . PHP_EOL);

        // Stop output buffering and get the contents of the buffer
        $libBuff = LibraryManager::GetBuffer();
        $contents = ob_get_clean();

        // Tell the library manager we are done with our file
        LibraryManager::Pop();

        $filename = $file;

        file_put_contents($filename, $headers);
        file_put_contents($filename, $libBuff, FILE_APPEND);
        file_put_contents($filename, $contents, FILE_APPEND);

        // Nicely format the generated file
        shell_exec('astyle --style=kr ' . $filename);
    }

    /*
     *  This function is used to take a list of attributes and a matching list
     *  of types and ensure that they types of the attributes are the same as
     *  the types in the list. If an attribute has no type, it will be given
     *  the corresponding type from the list.
     */
    function correlateAttributes( array $attrs, array $types ) {
        grokit_logic_assert( \count($attrs) == \count($types),
            'correlateAttributes called with unbalanced lists (' . \count($attrs) . ' vs ' . \count($types) . ')');

        reset($types);

        foreach( $attrs as &$curAttr ) {
            $curType = current($types);

            $attName = $curAttr->name();
            $attType = $curAttr->type();

            if( $attType === null ) {
                AttributeManager::setAttributeType($attName, $curType);

                // Refresh the attribute to make sure it has the updated type
                $curAttr = AttributeManager::lookupAttribute($attName);
            }

            next($types);
        }
    }

    /********** Top Level AST Parsing Methods **********/

    function parseRequiredState( $ast, $query ) {
        assert_ast_type($ast, NodeType::STATE);

        $data = ast_node_data( $ast );
        $sourceWP = ast_get($data, NodeKey::NAME);

        $type = StateRegistry::getState($sourceWP, $query);
        $type = $type->lookup();

        return new StateInfo( $sourceWP, $type );
    }

    function parseStateList( $ast, $query ) {
        $list = [];

        grokit_logic_assert( is_array($ast),
            'Got non-list input to ' . __FUNCTION__ . ' of type ' . gettype($ast) );

        foreach( $ast as $val ) {
            $list[] = parseRequiredState($val, $query);
        }

        return $list;
    }

    function parseAttributeMap( $map, &$info ) {
        grokit_logic_assert( is_array($map) || is_object($map),
            'Passed non-list value of type ' . gettype($map) . ' to ' . __FUNCTION__ );

        // Transform it into an array if it isn't already
        $ret = [];
        foreach( $map as $attName => $queries ) {
            $ret[$attName] = $queries;
            $att = lookupAttribute($attName);

            // Get information about attribute
            $info->absorbAttr($att);
        }

        return $ret;
    }

    function parseQueryToAttributeSet( $map ) {
        $ret = [];
        foreach( $map as $query => $set ) {
            $attrs = [];
            foreach( $set as $attr ) {
                $attrs[] = lookupAttribute($attr);
            }
            $ret[$query] = $attrs;
        }

        return $ret;
    }

    function parseProgramHeaders( $ast ) {
        $headers = ast_get($ast, NodeKey::HEADER);

        ob_start();

        // Generate standard header with date of generation and copyright notice
        echo '// This file was automatically generated on ' . date('l jS \of F Y h:i:s A') . PHP_EOL;
        echo '// Copyright 2013 Tera Insights, LLC. All Rights Reserved' . PHP_EOL;

        // Echo out the standard headers.
        echo <<<'EOT'
// Including GrokIt headers
#include <climits>
#include <fstream>

#include "HashFunctions.h"
#include "WorkDescription.h"
#include "ExecEngineData.h"
#include "Chunk.h"
#include "DataTypes.h"
#include "MMappedStorage.h"
#include "ColumnIterator.h"
#include "ColumnIterator.cc"
#include "BString.h"
#include "BStringIterator.h"
#include "Constants.h"
#include "QueryManager.h"
#include <string.h>
#include "Logging.h" // for profiling facility
#include "Profiling.h"
#include "WPFExitCodes.h"
#include "HashFunctions.h"
#include "Null.h"

EOT
        ;

        // Implicit using base
        $base_dir = getenv('GROKIT_INSTALLED_LIBRARY_PATH');
        include_once($base_dir . DIRECTORY_SEPARATOR . 'base.php');

        foreach( $headers as $h ) {
            $type = ast_node_type($h);

            switch( $type ) {
            case NodeType::IMPORT:
                parseImport( $h );
                break;
            case NodeType::DT:
                parseDataType($h);
                break;
            case NodeType::GLA:
                $res = parseGLA($h);
                $res->apply([], []);
                break;
            case NodeType::GT:
                $res = parseGT($h);
                $res->apply([], []);
                break;
            case NodeType::GF:
                $res = parseGF($h);
                $res->apply([]);
                break;
            case NodeType::GIST:
                $res = parseGIST($h);
                $res->apply([]);
                break;
            case NodeType::GI:
                $res = parseGI($h);
                $res->apply([]);
                break;
            }
        }

        $ret = ob_get_clean();

        return $ret;
    }

    function parseImport( $ast ) {
        assert_ast_type( $ast, NodeType::IMPORT );

        $lib = parseIdentifier(ast_node_data($ast));
        $source = ast_node_source($ast);

        $base_dir = getenv('GROKIT_INSTALLED_LIBRARY_PATH');
        $parts = LibraryManager::SplitNamespace($lib);
        $file = $base_dir . DIRECTORY_SEPARATOR . implode( DIRECTORY_SEPARATOR, $parts) . '.php';

        $result = include_once $file;

        grokit_assert( $result !== false,
            'Failed to include library ' . $lib . ', no library file ' . $file .
            ' found ' . $source );
    }

    function parseFullAttribute( $ast ) {
        $name = ast_get($ast, NodeKey::NAME);
        $type = ast_get($ast, NodeKey::TYPE);
        $slot = ast_get($ast, NodeKey::SLOT);

        // Give entire type AST to attribute manager, it will be parsed
        // only when needed.
        addAttribute( $name, $type, $slot );
    }

    function parseProgramAttributes( $ast ) {
        $attrs = ast_get($ast, NodeKey::ATTRIBUTES);

        foreach( $attrs as $attr ) {
            parseFullAttribute( $attr );
        }
    }

    function parseProgramGraph( $ast ) {
        // TODO: Write this
    }

    function parseProgramWaypoints( $ast, $header ) {
        $wps = ast_get($ast, NodeKey::WAYPOINTS);
        $edges = ast_get($ast, NodeKey::EDGES);

        $waypoints = [];
        $res = new GenerationInfo;

        foreach( $wps as $wpast ) {
            // Get the waypoint's name from the node
            $wpname = ast_get( $wpast, NodeKey::NAME );
            $type = ast_get($wpast, NodeKey::TYPE);

            //fwrite( STDERR, "====== Waypoint $wpname - $type ======\n");

            switch( $type ) {
            case NodeType::PRINT_WP:
                $gRes = parsePrintWP( $wpast, $wpname, $header);
                $waypoints[$wpname] = "print";
                break;
            case NodeType::GI_WP:
                $gRes = parseGIWP( $wpast, $wpname, $header);
                $waypoints[$wpname] = "gi";
                break;
            case NodeType::SEL_WP:
                $gRes = parseSelectionWP( $wpast, $wpname, $header);
                $waypoints[$wpname] = "selection";
                break;
            case NodeType::JOIN_WP:
                $gRes = parseJoinWP( $wpast, $wpname, $header);
                $waypoints[$wpname] = "join";
                break;
            case NodeType::GLA_WP:
                $gRes = parseGLAWP( $wpast, $wpname, $header);
                $waypoints[$wpname] = "gla";
                break;
            case NodeType::GT_WP:
                $gRes = parseGTWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = 'gt';
                break;
            case NodeType::GIST_WP:
                $gRes = parseGISTWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = 'gist';
                break;
            case NodeType::SCAN_WP:
                $gRes = parseScanWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = "scan";
                break;
            case NodeType::CLEANER_WP:
                $gRes = parseCleanerWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = "cleaner";
                break;
            case NodeType::CACHE_WP:
                $gRes = parseCacheWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = "cache";
                break;
            case NodeType::COMPACT_WP:
                $gRes = parseCompactWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = "compact";
                break;
            case NodeType::CLUSTER_WP:
                $gRes = parseClusterWP( $wpast, $wpname, $header );
                $waypoints[$wpname] = "cluster";
                break;
            default:
                grokit_logic_error( 'Encountered unexpected node of type ' . $type .
                    ' when a waypoint was expected');
            }

            $res->absorb($gRes);
        }

        return ["waypoints" => $waypoints, "edges" => $edges, "generated" => $res ];
    }

    function parseScanWP( $ast, $wpname, $header ) {
        // Nothing to do for scanner
        return new GenerationInfo;
    }

    function parseGLAWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        /***************   PROCESS AST   ***************/

        $attMap = ast_get($ast, NodeKey::ATT_MAP);
        $payload = ast_get($ast, NodeKey::PAYLOAD );

        $queries = [];

        // Info to return
        $res = new GenerationInfo;

        foreach( $payload as $query => $qInfo ) {

            $glaSpec = parseGLA( ast_get($qInfo, NodeKey::TYPE) );
            $exprs = parseNamedExpressionList( ast_get($qInfo, NodeKey::ARGS) );
            $output = parseAttributeList( ast_get($qInfo, NodeKey::VALUE) );
            $cargs = parseJsonAst( ast_get($qInfo, NodeKey::CARGS) );
            $sargs = parseStateList( ast_get($qInfo, NodeKey::SARGS), $query );
            $retState = ast_get($qInfo, NodeKey::STATE);

            $reqStates = [];
            foreach( $sargs as $val ) {
                $reqStates[$val->name()] = $val->type();
            }

            $gla = $glaSpec->apply($exprs, extractTypes($output), $reqStates);
            //fwrite(STDERR, "GLA outputs: " . print_r($gla->output()) . PHP_EOL);
            correlateAttributes($output, $gla->output());

            $info = [ 'gla' => $gla, 'expressions' => $exprs, 'output' => $output,
                'cargs' => $cargs, 'states' => $sargs, 'retState' => $retState ];

            $queries[$query] = $info;


            $res->addJob( $query, $name );
            $res->absorbInfoList($exprs);
            $res->absorbAttrList($output);
            $res->absorbStateList($sargs);

            $res->absorbInfo($gla);

            StateRegistry::addState($name, $query, $gla);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file at the moment
        $filename = $name . '.cc';
        $res->addFile($filename, $name );
        _startFile( $filename );
        GLAGenerate( $name, $queries, $attMap );
        _endFile( $filename, $myHeaders);

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parseGTWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        /***************   PROCESS AST   ***************/

        $attMap = ast_get($ast, NodeKey::ATT_MAP);
        $payload = ast_get($ast, NodeKey::PAYLOAD );
        $passthrough = parseQueryToAttributeSet(ast_get($ast, NodeKey::PASSTHROUGH ));

        $queries = [];

        // Info to return
        $res = new GenerationInfo;

        foreach( $payload as $query => $qInfo ) {
            $gtSpec = parseGT( ast_get($qInfo, NodeKey::TYPE) );
            $exprs = parseNamedExpressionList( ast_get($qInfo, NodeKey::ARGS) );
            $output = parseAttributeList( ast_get($qInfo, NodeKey::VALUE) );
            $cargs = parseJsonAst( ast_get($qInfo, NodeKey::CARGS) );
            $sargs = parseStateList( ast_get($qInfo, NodeKey::SARGS), $query );

            // Set of attributes to be copied to the output for each tuple
            $pass = $passthrough[$query];

            $reqStates = [];
            foreach( $sargs as $val ) {
                $reqStates[$val->name()] = $val->type();
            }

            $gt = $gtSpec->apply($exprs, extractTypes($output), $reqStates);
            correlateAttributes($output, $gt->output());

            $info = [
                'gt' => $gt,
                'expressions' => $exprs,
                'output' => $output,
                'cargs' => $cargs,
                'states' => $sargs,
                'pass' => $pass,
            ];

            $queries[$query] = $info;

            $res->addJob( $query, $name );
            $res->absorbInfoList($exprs);
            $res->absorbAttrList($pass);
            $res->absorbAttrList($output);
            $res->absorbStateList($sargs);
            $res->absorbInfo($gt);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file at the moment
        $filename = $name . '.cc';
        $res->addFile($filename, $name );
        _startFile( $filename );
        GTGenerate( $name, $queries, $attMap );
        _endFile( $filename, $myHeaders);

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parseGISTWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        /***************   PROCESS AST   ***************/

        $attMap = ast_get($ast, NodeKey::ATT_MAP);
        $payload = ast_get($ast, NodeKey::PAYLOAD );

        $queries = [];

        // Info to return
        $res = new GenerationInfo;

        foreach( $payload as $query => $qInfo ) {

            $gistSpec = parseGIST( ast_get($qInfo, NodeKey::TYPE) );
            $output = parseAttributeList( ast_get($qInfo, NodeKey::VALUE) );
            $cargs = parseJsonAst( ast_get($qInfo, NodeKey::CARGS) );
            $sargs = parseStateList( ast_get($qInfo, NodeKey::SARGS), $query );
            $retState = ast_get($qInfo, NodeKey::STATE);

            $reqStates = [];
            foreach( $sargs as $val ) {
                $reqStates[$val->name()] = $val->type();
            }

            $gist = $gistSpec->apply(extractTypes($output), $reqStates);
            //fwrite(STDERR, "GIST outputs: " . print_r($gist->output()) . PHP_EOL);
            correlateAttributes($output, $gist->output());

            $info = [ 'gist' => $gist, 'output' => $output,
                'cargs' => $cargs, 'states' => $sargs, 'retState' => $retState ];

            $queries[$query] = $info;


            $res->addJob( $query, $name );
            $res->absorbAttrList($output);
            $res->absorbStateList($sargs);

            $res->absorbInfo($gist);

            StateRegistry::addState($name, $query, $gist);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file at the moment
        $filename = $name . '.cc';
        $res->addFile($filename, $name );
        _startFile( $filename );
        GISTGenerate( $name, $queries, $attMap );
        _endFile( $filename, $myHeaders);

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parseJoinWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/
        // There probably needs to be some parsing done here to make sure that
        // types are defined before the WP code is generated.

        $jDesc = new \stdClass;

        $jDesc->attribute_queries_LHS = parseAttributeMap(ast_get($ast, "attribute_queries_LHS"), $res);
        $jDesc->attribute_queries_RHS = parseAttributeMap(ast_get($ast, "attribute_queries_RHS"), $res);
        $jDesc->attribute_queries_LHS_copy = parseAttributeMap(ast_get($ast, "attribute_queries_LHS_copy"), $res);
        $jDesc->attribute_queries_RHS_copy = parseAttributeMap(ast_get($ast, "attribute_queries_RHS_copy"), $res);
        $jDesc->LHS_hash = ast_get($ast, "LHS_hash");
        $jDesc->LHS_keys = ast_get($ast, "LHS_keys");
        $jDesc->exists_target = ast_get($ast, "exists_target");
        $jDesc->not_exists_target = ast_get($ast, "not_exists_target");
        $jDesc->left_target = ast_get($ast, "left_target");
        $jDesc->queries_attribute_comparison = ast_get($ast, "queries_attribute_comparison");

        $jDesc->hash_RHS_attr = ast_get($ast, "hash_RHS_attr");
        $jDesc->query_classes_hash = ast_get($ast, "query_classes_hash");

        $jobs = ast_get($ast, NodeKey::QUERIES);
        foreach( $jobs as $job ) {
            $res->addJob($job, $name);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Join LHS
        $filename = $name . '_LHS.cc';
        $res->addFile($filename, $name);
        _startFile($filename);
        JoinLHS($name, $jDesc);
        _endFile($filename, $myHeaders);

        // Join RHS
        $filename = $name . '_RHS.cc';
        $res->addFile($filename, $name);
        _startFile($filename);
        JoinRHS($name, $jDesc);
        _endFile($filename, $myHeaders);

        // Join LHS Hash
        $filename = $name . '_LHS_Hash.cc';
        $res->addFile($filename, $name);
        _startFile($filename);
        JoinLHSHash($name, $jDesc);
        _endFile($filename, $myHeaders);

        // Join Merge
        $filename = $name . "_Merge.cc";
        $res->addFile($filename, $name);
        _startFile($filename);
        JoinMerge($name, $jDesc);
        _endFile($filename, $myHeaders);

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parsePrintWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $queriesAST = ast_get($ast, NodeKey::PAYLOAD);
        $attMap = ast_get($ast, NodeKey::ATT_MAP);

        $queries = [];

        parseAttributeMap($attMap, $res);

        foreach( $queriesAST as $query => $qInfo ) {
            $exprAST = ast_get($qInfo, NodeKey::EXPR);
            $exprs = parseExpressionList($exprAST);

            $sep = ast_get($qInfo, NodeKey::SEP);
            $type = ast_get($qInfo, NodeKey::TYPE);

            $info = [ 'expressions' => $exprs, 'separator' => $sep, 'type' => $type ];
            $queries[$query] = $info;

            $res->addJob($query, $name);
            $res->absorbInfoList($exprs);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file at the moment
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename );
        PrintGenerate( $name, $queries, $attMap );
        _endFile( $filename, $myHeaders );

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parseSelectionWP( $ast, $name, $header ) {
        // Push LibraryManager so we can undo this waypoint's definitions.
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $attMap = parseAttributeMap(ast_get($ast, NodeKey::ATT_MAP), $res);
        $qFilters = ast_get($ast, NodeKey::FILTERS);
        $qSynth = ast_get($ast, NodeKey::SYNTH);

        $queries = [];

        foreach( $qFilters as $query => $qInfo ) {
            $filterAST = ast_get($qInfo, NodeKey::ARGS);
            $gfAST = ast_get($qInfo, NodeKey::TYPE);

            if( $gfAST !== null )
                $filter = parseNamedExpressionList($filterAST);
            else
                $filter = parseExpressionList($filterAST);

            $gf = null;
            if( $gfAST !== null ) {
                $gf = parseGF($gfAST);
                $gf = $gf->apply($filter);
            }

            if( ast_has( $qInfo, NodeKey::CARGS) ) {
                $cargs = parseLiteralList( ast_get($qInfo, NodeKey::CARGS) );
            }
            else {
                $cargs = [];
            }

            $sargs = ast_has($qInfo, NodeKey::SARGS) ? parseStateList( ast_get($qInfo, NodeKey::SARGS), $query ) : [];

            $synths = Array();
            $synthAST = ast_get($ast, NodeKey::SYNTH);
            if( ast_has( $synthAST, $query ) ) {
                $curSynths = ast_get( $synthAST, $query );

                foreach( $curSynths as $curSynthAST ) {
                    $expr = parseExpression(ast_get($curSynthAST, NodeKey::EXPR));
                    $att = parseAttribute(ast_get($curSynthAST, NodeKey::ATT));

                    if( $att->type() == null ) {
                        AttributeManager::setAttributeType($att->name(), $expr->type());
                        $att = AttributeManager::lookupAttribute($att->name());
                    }
                    else if( canConvert( $expr, $att->type() ) ) {
                        $expr = convertExpression( $expr, $att->type() );
                    }
                    else {
                        grokit_error('Unable to convert expression for synthesized attribute ' . $att->name() . ' from type ' . $expr->type() .
                            ' to type ' . $att->type() . ' ' . $expr->source() );
                    }

                    $synths[$att->name()] = $expr;
                }
            }

            $info = [ 'filters' => $filter, 'synths' => $synths, 'gf' => $gf, 'cargs' => $cargs, 'states' => $sargs ];
            $queries[$query] = $info;

            $res->addJob($query, $name);
            $res->absorbInfoList($filter);
            $res->absorbInfoList($synths);
            $res->absorbInfoList($cargs);
            $res->absorbStateList($sargs);
            if( $gf !== null ) $res->absorbInfo($gf);
        }

        /*************** END PROCESS AST ***************/

        // Get this waypoint's headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file at the moment
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename );
        SelectionGenerate( $name, $queries, $attMap );
        _endFile( $filename, $myHeaders );

        // Pop LibraryManager again to get rid of this waypoint's declarations
        LibraryManager::Pop();

        return $res;
    }

    function parseGIWP( $ast, $name, $header ) {
        // Push  LibraryManager so we can get the waypoint-specific headers
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $payload = ast_get($ast, NodeKey::PAYLOAD);

        $spec = parseGI( ast_get($payload, NodeKey::TYPE) );
        $outputs = parseAttributeList(ast_get($payload, NodeKey::ARGS));
        $constArgs = parseLiteralList( ast_get($payload, NodeKey::CARGS) );
        $nTuples = ast_get($payload, NodeKey::TUPLES);
        $queries = ast_get($ast, NodeKey::QUERIES);

        foreach( $queries as $query ) {
            $res->addJob($query, $name);
        }

        // The spec is (possibly) an incomplete GI type, to finish it we need to tell it to
        // perform a lookup with its outputs known.
        $type = $spec->apply( extractTypes( $outputs ) );
        correlateAttributes($outputs, $type->output());

        $attMap = parseAttributeMap(ast_get($ast, NodeKey::ATT_MAP), $res);

        $res->absorbAttrList($outputs);
        $res->absorbInfoList($constArgs);
        $res->absorbInfo($type);

        /*************** END PROCESS AST ***************/

        // Get our headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file right now
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename);
        GIGenerate( $name, $type, $outputs, $constArgs, $nTuples );
        _endFile( $filename , $myHeaders );

        LibraryManager::Pop();

        return $res;
    }

    function parseCleanerWP( $ast, $name, $header ) {
        // Push  LibraryManager so we can get the waypoint-specific headers
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $queryNames = ast_get($ast, "query_names");
        foreach( $queryNames as $query ) {
            $res->addJob($query, $name);
        }

        $queries = ast_get($ast, "queries");
        $lhsNames = ast_get($ast, "LHS");
        $rhsNames = ast_get($ast, "RHS");

        $LHS = [];
        $RHS = [];

        foreach( $lhsNames as $att ) {
            $LHS[] = lookupAttribute($att);
        }
        foreach( $rhsNames as $att ) {
            $RHS[] = lookupAttribute($att);
        }

        $res->AbsorbAttrList($LHS);
        $res->AbsorbAttrList($RHS);

        /*************** END PROCESS AST ***************/

        // Get our headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file right now
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename );
        CleanerGenerate( $name, $LHS, $RHS, $queries, $queryNames );
        _endFile( $filename , $myHeaders );

        LibraryManager::Pop();

        return $res;
    }

    function parseCacheWP( $ast, $name, $header ) {
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;
        // Get our headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file right now
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename );
        CacheGenerate( $name );
        _endFile( $filename , $myHeaders );

        LibraryManager::Pop();

        return $res;
    }

    function parseCompactWP( $ast, $name, $header ) {
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $attMap = parseAttributeMap(ast_get($ast, NodeKey::ATT_MAP), $res);

        /*************** END PROCESS AST ***************/

        // Get our headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        // Only one file right now
        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile( $filename );
        CompactGenerate( $name, $attMap );
        _endFile( $filename , $myHeaders );

        LibraryManager::Pop();

        return $res;
    }

    function parseClusterWP( $ast, $name, $header ) {
        ob_start();
        LibraryManager::Push();

        $res = new GenerationInfo;

        /***************   PROCESS AST   ***************/

        $attr = lookupAttribute(ast_get($ast, NodeKey::PAYLOAD));
        $attrType = $attr->type();

        grokit_assert($attrType->is('clusterable'),
            'Attempting to cluster on unclusterable attribute ' .
            $attr->name());

        /*************** END PROCESS AST ***************/

        // Get our headers
        $myHeaders = $header . PHP_EOL . ob_get_clean();

        $filename = $name . '.cc';
        $res->addFile($filename, $name);
        _startFile($filename);
        ClusterGenerate($name, $attr);
        _endFile($filename, $myHeaders);

        LibraryManager::Pop();

        return $res;
    }

    function parseProgram( $ast ) {
        parseProgramAttributes ($ast );
        $headers = parseProgramHeaders( $ast );
        $res = parseProgramWaypoints( $ast, $headers );
        return $res;
    }

    function parseJSON( $json ) {
        $ast = json_decode( $json );
        grokit_assert( $ast !== null, 'Unable to decode JSON.');

        return parseProgram( $ast );
    }
}

namespace grokit\internal {
    function error_handler($code, $message, $file, $line) {
        if( ($code & E_STRICT) != 0 ) return TRUE;
        $strict = ($code & E_STRICT) != 0 ? 'true' : 'false';
        fwrite(STDERR, "Error: code($code) message($message) file($file) line($line) strict($strict)" . PHP_EOL);
        fflush(STDERR);
        throw new \ErrorException($message, $code, $code, $file, $line);
    }

    function concise_exception_trace(array $trace) {
        $ret = [];

        foreach( $trace as $n => $v ) {
            $ret[$n] = [ ];

            if( array_key_exists('function', $v) ) {
                $ret[$n]['function'] = $v['function'];
            }

            if( array_key_exists('line', $v) ) {
                $ret[$n]['line'] = $v['line'];
            }

            if( array_key_exists('file', $v) ) {
                $ret[$n]['file'] = $v['file'];
            }

            if( array_key_exists('type', $v) ) {
                $ret[$n]['type'] = $v['type'];
            }

            if( array_key_exists('object', $v) ) {
                $ret[$n]['object'] = $v['object'];
            }

            if( array_key_exists('class', $v) ) {
                $ret[$n]['class'] = $v['class'];
            }

            $args = [];
            foreach( $v['args'] as $argN => $argV ) {
                $type = gettype($argV);
                $str = $type;
                switch($type) {
                case 'string':
                    $strLen = \strlen($argV);
                    // Display at most 20 characters of the string
                    $shortStr = substr($argV, 0, 20);
                    if( \strlen($shortStr) < $strLen )
                        $shortStr .= '...';
                    $str .= '(' . \strlen($argV) . ',' . $shortStr . ')';
                    break;
                case 'array':
                    $str .= '(' . \count($argV) . ')';
                    break;
                case 'integer':
                case 'double':
                    $str .= '(' . $argV . ')';
                    break;
                }

                $args[$argN] = $str;
            }

            $ret[$n]['args'] = $args;
        }

        return $ret;
    }

    function exception_to_array(\Exception $e) {
        //fwrite(STDERR, "Exception!" . PHP_EOL);
        $ret = [
            '__type__'  => 'error',
            'kind'      => 'php',
            'message'   => $e->getMessage(),
            'code'      => $e->getCode(),
            'file'      => $e->getFile(),
            'line'      => $e->getLine(),
            'trace'     => concise_exception_trace($e->getTrace()),
        ];

        $prev = $e->getPrevious();
        if( $prev !== NULL ) {
            $ret['previous'] = exception_to_array($prev);
        }

        return $ret;
    }

    function exception_handler($e) {
        fwrite(STDERR, 'Exception: code(' . $e->getCode() . ')'.
            ' file(' . $e->getFile() . ')' .
            ' line(' . $e->getLine() . ')' .
            ' message(' . $e->getMessage() . ')' . PHP_EOL);

        $fName = './grokit_php_error.json';

        try {
            $info = exception_to_array($e);
            $json = json_encode($info);

            $file = fopen($fName, 'w');
            fwrite($file, $json);
            fwrite($file, PHP_EOL);
            fclose($file);
        } catch( Exception $ne ) {
            $reason = $ne->getMessage();
            fwrite(STDERR, "Failed to write error to file {$fName} for reason: {$reason}" . PHP_EOL);
        }

        exit(2);
    }
}

namespace {
    //fwrite(STDERR, "Setting error handler" . PHP_EOL);
    set_error_handler("\\grokit\\internal\\error_handler");
    //fwrite(STDERR, "Setting exception handler" . PHP_EOL);
    set_exception_handler("\\grokit\\internal\\exception_handler");
}

?>
