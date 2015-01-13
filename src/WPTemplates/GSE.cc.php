<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

function GSEGenerate( $wpName, $queries ) {
    GSEGenerate_PreProcess($wpName, $queries);
    GSEGenerate_ProcessReadOnly($wpName, $queries);
} // enc function GSEGenerate

function GSEGenerate_PreProcess( $wpName, $queries ) {

?>
//+{"kind":"WPF", "name":"Pre-Processing", "action":"start"}
extern "C"
int GSEPreProcessWorkFunc_<?=$wpName?>
(WorkDescription& workDescription, ExecEngineData& result) {

}
//+{"kind":"WPF", "name":"Pre-Processing", "action":"end"}
<?
} // end function GSEGenerate_PreProcess
?>
