<?
function GLAGenerate_PostFinalize( $wpName, $queries, $attMap ) {
?>
//+{"kind":"WPF", "name":"Post-Finalize", "action":"start"}
extern "C"
int GLAPostFinalizeWorkFunc_<?=$wpName?>
(WorkDescription &workDescription, ExecEngineData &result) {
    
    GLAPostFinalizeWD myWork;
    myWork.swap(workDescription);

    QueryToGLAStateMap& queryGLAStates = myWork.get_glaStates();
    QueryExitContainer& queries = myWork.get_whichQueryExits();

<?	cgDeclareQueryIDs($queries); ?>

	FOREACH_TWL(iter, queries) {
		FATALIF(!queryGLAStates.IsThere(iter.query),
			"GLA PostFinalize got a query with no state");
		GLAState& curState = queryGLAStates.Find(iter.query);

<? 	foreach( $queries as $query => $info ):
		$gla = $info['gla'];
		if( !$gla->post_finalize() ) {
			continue;
		}
?>
		if( iter.query == <?=queryName($query)?> ) {
			GLAPtr localState;
			localState.swap(curState);
			FATALIF( localState.get_glaType() != <?=$gla->cHash()?>,
				"GLA PostFinalize got GLA with incorrect type for query <?=$query?>");
			<?=$gla?> * localGLA = (<?=$gla?> *) localState.get_glaPtr();
			localState.swap(curState);

			localGLA->PostFinalize();
		}
<? 	endforeach; // queries ?>
	} END_FOREACH;

	return WP_POST_FINALIZE;
}
//+{"kind":"WPF", "name":"Post-Finalize", "action":"end"}
<?
} // end function GLAGenerate_PostFinalize
?>