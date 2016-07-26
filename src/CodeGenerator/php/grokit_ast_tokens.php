<?php
namespace grokit {

// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

/*
 * This file contains all of the definitions for the token types and members.
 */

// Hacky enum
final class NodeKey {
    // Node specific information
    const NODE_TYPE     = 'node_type';
    const NODE_DATA     = 'node_data';
    const NODE_SOURCE   = 'node_source';

    // Source information
    const FILENAME      = 'file';
    const LINE          = 'line';
    const COL           = 'column';

    // General attributes for expressions, GLAs, etc.
    const NAME          = 'name';
    const TYPE          = 'type';
    const ARGS          = 'args';
    const TARGS         = 't_args';
    const CARGS         = 'const_args';
    const SARGS         = 'state_args';
    const VALUE         = 'value';
    const SOURCE        = 'source';
    const COLUMNS       = 'columns';
    const TUPLES        = 'tuples';
    const QUERIES       = 'queries';
    const SEP           = 'separator';
    const SLOT          = 'slot';
    const STATE         = 'state';
    const ALIAS         = 'alias';

    // Hold-overs from old parser
    const C_EXPR        = 'c_expr';
    const CST_STR       = 'cst_str';
    const C_DEFS        = 'c_defs';
    const OLD_HEADER    = 'old_header';
    const OLD_TYPE      = 'old_type';

    // Case statement
    const BASE          = 'base';
    const CASES         = 'cases';
    const DEFAULT_CASE  = 'default';
    // Individual cases
    const TEST          = 'test';
    const EXPR          = 'expression';

    // Match statement
    const PATTERN       = 'pattern';

    // Overall program node
    const JOB_ID        = 'job_id';
    const HEADER        = 'header';
    const WAYPOINTS     = 'waypoints';
    const EDGES         = 'edges';
    const ATTRIBUTES    = 'attributes';

    // Waypoints
    const ATT_MAP       = 'att_map';
    const PAYLOAD       = 'payload';

    // Selection
    const FILTERS       = 'filters';
    const SYNTH         = 'synth';
    const ATT           = 'attribute';

    // GT
    const PASSTHROUGH   = 'passthrough';

    // Join
    const LHS           = 'LHS';
    const RHS           = 'RHS';
    const COLS_IN       = 'columns_in';
    const COLS_OUT      = 'columns_out';

    // Private constructor so it cannot be instantiated
    private function __construct() { }
}

// Possible values for NODE_TYPE
final class NodeType {

    // Types
    const DT            = 'datatype';
    const STATE         = 'state';
    const FUNCTOR       = 'functor';
    const REFERENCE     = 'reference';
    const TYPEOF        = 'typeof';

    // Waypoints
    const PRINT_WP      = 'print_wp';
    const TL_WP         = 'textloader_wp';
    const GLA_WP        = 'gla_wp';
    const GF_WP         = 'gf_wp';
    const GT_WP         = 'gt_wp';
    const GIST_WP       = 'gist_wp';
    const GI_WP         = 'gi_wp';
    const SEL_WP        = 'selection_wp';
    const JOIN_WP       = 'join_wp';
    const SCAN_WP       = 'scanner_wp';
    const CLEANER_WP    = 'cleaner_wp';
    const CACHE_WP      = 'cache_wp';
    const CLUSTER_WP    = 'cluster_wp';
    const COMPACT_WP    = 'compact_wp';

    // Waypoint payloads
    const GLA           = 'gla';
    const GT            = 'gt';
    const GF            = 'gf';
    const GI            = 'gi';
    const GIST          = 'gist';
    const JOIN          = 'join';
    const SELECTION     = 'selection';
    const SYNTH         = 'synth';

    // Waypoint result
    const RESULT        = 'result';

    // Expressions
    const LITERAL       = 'literal';
    const NUL           = 'null';
    const ATTRIBUTE     = 'att';
    const FUNC          = 'function';
    const METHOD        = 'method';
    const OPERATOR      = 'operator';
    const CASE_NODE     = 'case';
    const MATCH         = 'match';
    const IDENTIFIER    = 'identifier';

    // Template args
    const NAMED_ARGS    = 'named_arguments';

    // Named expression list
    const NAMED_EXPR_LIST = 'named_expression_list';

    // JSON (used for configuration)
    const JSON_INLINE   = 'json_inline';
    const JSON_FILE     = 'json_file';

    // Headers
    const DEFINE_NODE   = 'define';
    const IMPORT        = 'import';

    // Private constructor so it cannot be instantiated
    private function __construct() { }
}

}
?>
