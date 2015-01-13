<?
/* Copyright 2013 Tera Insights. All rights reserved */

/* In all the functions in this file, $ast is an abstract sintax tree for the current waypoint */

function queryName($qry) {
    if( strlen($qry) <= 8 )
        return $qry;
    else
        return substr($qry, 0, 8);
}

/***************************** Attribute functions *****************************/
// form column name for attribute
function attCol($att){ return "col_".$att; }
// form attribute slot for attribute
function attSlot($att){ return lookupAttribute($att)->slot(); }
// form attribute type; this should be a valid C++ type (code injected to make sure this is the case)
function attType($att){ return lookupAttribute($att)->type(); }
// define iterator of attribute
function attIteratorType($att){ return lookupAttribute($att)->type()->iterator(); }
// form queries in which attribute is active
function attQrys($att){ return $att."_Qrys"; }
// form the name of the datastructure that contains attrubute data
function attData($att){ return $att."_Column"; }
// function to form storage name
function attStorage($att){ return "storage_".$att; }
// test weathe the type of an attribute is simple or complex
function isFixedAtt($att){ return lookupAttribute($att)->type()->isFixedSize(); }
// serialization and deserialization functions
function attSerializedSize($att, $obj){
    if (attType($att)->isFixedSize()) return "sizeof(".$att.")";
    else return "" . $obj . '.GetSize()';
}
function attOptimizedSerialize($att, $obj, $buffer){
    if (attType($att)->isFixedSize()) return "(void*)&(".$obj.")";
    else return "" . $obj . ".Serialize(" . $buffer . ")";
}
function attOptimizedDeserialize($att, $obj, $buffer, $offset){
    if (attType($att)->isFixedSize()) return "".$obj." = *((".attType($att)." *) (".$buffer." + ".$offset."))";
    else return "".$obj.".Deserialize(".$buffer." + ".$offset.")";
}




/****************************** Code generation functions **********************/

// Functin to declare query ids. Used everywhere
function cgDeclareQueryIDs($queries){
?>
    // get access to query manager
    QueryManager& qm=QueryManager::GetQueryManager();
    // set up the QueryIDs of the queries involved
<? foreach( $queries as $query => $val){ ?>
    QueryID <?=queryName($query)?>=qm.GetQueryID("<?=queryName($query)?>");
<? } ?>

<?
}

// Function to create columns for a set of attributes.
// An optional postfix will be added to the attribute names
function cgConstructColumns($atts, $postfix = "") {
    foreach( $atts as $attr ) {
        $att = lookupAttribute(strval($attr));
        $attName = $att . $postfix;
?>
    MMappedStorage <?=$attName?>_Column_store;
    Column::Destroyer <?=$attName?>_destroyer;
<?
        if( $att->type()->destroy() ) {
?>
    <?=$attName?>_destroyer = [] (Column& c) {
        <?=$att->type()->iterator()?> iter(c);
        while( !iter.AtUnwrittenByte() ) {
            <?=$att->type()?> & val = const_cast< <?=$att->type()?>& >( iter.GetCurrent() );
            val.Destroy();
            iter.Advance();
        }
        iter.Done(c);
    };
<?
        } // if attribute must be destroyed
?>
    Column <?=$attName?>_Column_Ocol(<?=$attName?>_Column_store, <?=$attName?>_destroyer);
    <?=$att->type()->iterator()?> <?=$attName?>_Column_Out(<?=$attName?>_Column_Ocol);
    <?=$att->type()?> <?=$attName?>; // Container for value to be written

<?
    } // foreach attribute
}

// Function to define columns that are needed.
// Needs attribute map. Assumens $attributes is set globaly
function cgAccessColumns($att_map, $chunk, $wpName){ ?>
    // Declaring and extracting all the columns that are needed
<? foreach( $att_map as $att => $qry){ ?>
    QueryIDSet <?=attQrys($att)?>(<?=$qry?>, true);
    // extracting <?=$att?>:
    Column <?=attCol($att)?>;
    if (<?=attQrys($att)?>.Overlaps(queriesToRun)){
            <?=$chunk?>.SwapColumn(<?=attCol($att)?>, <?=attSlot($att)?>);
            if (! <?=attCol($att)?>.IsValid()){
                FATAL("Error: Column <?=$att?> not found in <?=$wpName?>\n");
            }
        }
    <?=attIteratorType($att)?> <?=attData($att)?> (<?=attCol($att)?>/*, 8192*/);

<? }

}

// Function to put back columns
function cgPutbackColumns($att_map, $chunk, $wpName){ ?>
<? foreach( $att_map as $att => $qry){ ?>
    // putting back column of <?=$att?>:
    if (<?=$att?>_Qrys.Overlaps(queriesToRun)){
        <?=$att?>_Column.Done(<?=attCol($att)?>);
        <?=$chunk?>.SwapColumn(<?=attCol($att)?>, <?=attSlot($att)?>);
    }
<? }

}


// Function to extract attributes from columns into local variables
function cgAccessAttributes($att_map) { ?>
        // extract values of attributes from streams
<?foreach($att_map as $att => $qry){?>
        const <?=attType($att)?>& <?=$att?> = <?=attData($att)?>.GetCurrent();
<?}?>

<?
}

// Function to advance columns corresponding to attributes
function cgAdvanceAttributes($att_map, $indentLevel = 1) {
    $indent = str_repeat('    ', $indentLevel);
    echo $indent . '// Advance attributes' . PHP_EOL;
    foreach( $att_map as $att => $qry ) {
        echo $indent . attData($att) . '.Advance();' . PHP_EOL;
    }
}

// Function to advance a set of columns corresponding to attributes.
// The name of the column is the attribute name concatenated with the suffix.
// The indent level sets how many 4-space indent levels are prepended to each
// line.
function cgAdvanceAttributesList( $att_list, $suffix, $indentLevel = 1 ) {
    $indent = str_repeat('    ', $indentLevel);
    echo $indent . '// Advance attributes' . PHP_EOL;
    foreach( $att_list as $att ) {
        echo $indent . $att . $suffix . '.Advance();' . PHP_EOL;
    }
}

// Function to insert values into a set of columns and advance the columns.
// The value to be inserted is assumed to be the name of the attribute unless
// the value suffix is given.
// The name of the column is the attribute name concatenated with the suffix.
// The indent level sets how many 4-space indent levels are prepended to each
// line.
function cgInsertAttributesList( $att_list, $colSuffix, $indentLevel = 1, $valSuffix = '' ) {
    $indent = str_repeat('    ', $indentLevel);
    echo $indent . '// Advance attributes' . PHP_EOL;
    foreach( $att_list as $att ) {
        echo $indent . $att . $colSuffix . '.Insert(' . $att . $valSuffix . ');' . PHP_EOL;
        echo $indent . $att . $colSuffix . '.Advance();' . PHP_EOL;
    }
}

// Function to setup constants before the tuple processing loop
function cgConstantInit($queries){
    foreach($queries as $query => $val){ ?>
    // constants for query <?=queryName($query)?>:
<?  foreach($val["expressions"] as $exp){
    foreach($exp->constants() as $ct){ ?>
    <?=$ct?>
<?  }
}
    }
}

// Function to set up constants for a list of expressions
// A custom level of indentation can be given, otherwise the default of 1
// indent level is used.
// 1 level of indentation is equal to 4 spaces.
function cgDeclareConstants( $exprList, $indentLevel = 1 ) {
    foreach( $exprList as $expr ) {
        foreach( $expr->constants() as $const ) {
            for( $i = 0; $i < $indentLevel; $i++ ) {
                echo '    '; // 4 spaces
            }

            echo $const . PHP_EOL;
        } // foreach constant
    } //foreach expression
}

// Function to set up preprocessing statements for a list of expressions
// A custom level of indentation can be given, otherwise the default of 1
// indent level is used.
// 1 level of indentation is equal to 4 spaces.
function cgDeclarePreprocessing( $exprList, $indentLevel = 1 ) {
    foreach( $exprList as $expr ) {
        foreach( $expr->preprocess() as $preproc ) {
            for( $i = 0; $i < $indentLevel; $i++ ) {
                echo '    '; // 4 spaces
            }

            echo $preproc . PHP_EOL;
        } // foreach preprocessing statement
    } //foreach expression
}


// Preprocessing for eaqch expression so that it can be evaluated
// place in the Overlaps section just before expression evaluation
function cgPreprocess($val){
    foreach($val["expressions"] as $exp){
        foreach($exp->preprocess() as $ct){ ?>
       <?=$ct?>
<?  }
    }
}


// function to extract column fragments

function cgExtractColumnFragment($att, $chunk, $start, $end, $wpName){ ?>
            // extracting <?=$att?>:
            Column <?=attCol($att)?>;

            if (<?=attQrys($att)?>.Overlaps(queriesToRun)){
                <?=$chunk?>.SwapColumn(<?=attCol($att)?>, <?=attSlot($att)?>);
                if (! <?=attCol($att)?>.IsValid()){
                    FATAL("Error: Column <?=$att?> not found in <?=$wpName?>\n");
                }
            }
            <?=attIteratorType($att)?> <?=attData($att)?>(<?=attCol($att)?>/*, 8192*/, <?=$start?>, <?=$end?>);
<?
}

?>

