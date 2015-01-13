<?
// Copyright 2013 Tera Insights, LLC. All Rights Reserved


function TopK( array $types, array $inputs, array $outputs ) {
  // Validate template arguments
  // $types is array of types
  // these types are validated, and an array $t_args with var=>type is constructed

  $t_args = array();
  $i=1;
  foreach($types as $type){
    if(!ensure_type($type)) {
        throw new \RuntimeException("TopK GLA called with an invalid type ($type).");
    }
    $t_args["var" . $i++] = $type;
  }

?>



using namespace std;

struct <?=$name?>_Tuple {
    FLOAT topKScore;
<?foreach($t_args as $var=>$type):?>
    <?=$type?> <?=$var?>;
<? endforeach;?>

    <?=$name?>_Tuple() {
        topKScore = 0;
    }

    // Use the initialization list to construct the members to ensure that
    // deep copies are made
    <?=$name?>_Tuple(FLOAT _rank, <?= \grokit\typed_ref_args($t_args)?>) :
        topKScore( _rank )
<?foreach($t_args as $var=>$type):?>
      , <?=$var?> (<?=$var?>)
<? endforeach;?>

      {}

    <?=$name?>_Tuple& operator=(const <?=$name?>_Tuple& _other){
    topKScore = other.topKScore;
<?foreach($t_args as $var=>$type):?>
       <?=$var?> = other.<?=$var?>;
<? endforeach;?>
        return *this;
    }
};

// Auxiliary function to compare tuples
inline bool GreaterThenTopK(<?=$name?>_Tuple& t1, <?=$name?>_Tuple& t2) {
    return (t1.topKScore > t2.topKScore);
}

/** This class implements the computation of Top-k tuples. Input and
    *    output tuple have to be defined to be the same: Tuple.

    * The heap is maintained upside down (smallest value on top) so that
    * we can prune most insertions. If a new tuple does not compete with
    * the smallest value, we do no insertion. This pruning is crucial in
    * order to speed up inserition. If K is small w.r.t the size of the
    * data (and the data is not adversarially sorted), the effort to
    * compute Top-k is very close to O(N) with a small constant. In
    * practical terms, Top-k computation is about as cheap as
    * evaluating a condition or an expression.

    * InTuple: Tuple
    * OutTuple: Tuple

    * Assumptions: the input tuple has as member a numeric value called
    * "topKScore". What we mean by numeric is that is supports
    * conversion to double and has > comparison.
**/
class <?=$name?>{
private:
    // types
    typedef vector<<?=$name?>_Tuple> TupleVector;

    long long int count; // number of tuples covered

    // k as in top-k
    int K;

    // worst tuple in the heap
    double worst;

    TupleVector tuples;
    int pos; // position of the output iterator

    // function to force sorting so that GetNext gets the tuples in order
    void Sort() {sort_heap(tuples.begin(), tuples.end(), GreaterThenTopK);}

    // internal function
    void AddTupleInternal(<?=$name?>_Tuple& t);

public:
    // constructor & destructor
    <?=$name?>(int k) { count=0; K=k; pos = -1; worst = -1.0e+30; }
    ~<?=$name?>() {}

    // function to add an intem
    void AddItem(FLOAT _rank, <?=\grokit\typed_args($t_args)?>);

    // take the state from ohter and incorporate it into this object
    // this is a + operator on TOPK_NAME
    void AddState(<?=$name?>& other);

    // finalize the state and prepare for result extraction
    void Finalize() {
       Sort(); pos = 0;
       cout << "Total number of tuples in Topk=" << count << endl;
    }

    // iterator through the content in order (can be destructive)
    bool GetNextResult(FLOAT& _rank, <?=\grokit\typed_ref_args($t_args)?>) {
        if (pos == tuples.size())
            return false;
        else {
            <?=$name?>_Tuple& tuple = tuples[pos++];

<?foreach($t_args as $var=>$type):?>
	    <?=$type?> = tuple.<?=$var?>;
<? endforeach;?>

            return true;
        }
    }

};

void <?=$name?>::AddItem(FLOAT _rank, <?=\grokit\typed_ref_args($t_args)?>) {
    count++;
    if (_rank<worst) // fast path
                 return;

    <?=$name?>_Tuple tuple(_rank,  <?=\grokit\args($t_args)?>);

    AddTupleInternal(tuple);
}

void <?=$name?>::AddTupleInternal(<?=$name?>_Tuple& tuple){
    if (tuples.size() < K) {
        tuples.push_back(tuple);

        // when we have exactly K elements in the vector, organize it as a heap
        if (tuples.size() == K) {
            make_heap(tuples.begin(), tuples.end(), GreaterThenTopK);
            worst = tuples.front().topKScore;
        }
    }
    else {
        pop_heap(tuples.begin(), tuples.end(), GreaterThenTopK);
        tuples.pop_back();
        tuples.push_back(tuple) ;
        push_heap(tuples.begin(), tuples.end(), GreaterThenTopK);
        worst = tuples.front().topKScore;
    }
}

void <?=$name?>::AddState(<?=$name?>& other) {
    count+=other.count;
    // go over all the contents of other and insert it into ourselves
    for(int i = 0; i < other.tuples.size(); i++) {
      if (other.tuples[i].topKScore >= worst)
          AddTupleInternal(other.tuples[i]);
    }
}

<?

  $args = $types;
  array_unshift($args, "FLOAT");// create array with FLOAT, and types
  return  array (
            'args'          => $args,
            'result'        => $args,
            'result_type'   => 'single',
        );

}
?>
