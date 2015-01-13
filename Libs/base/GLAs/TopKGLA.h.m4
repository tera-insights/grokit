dnl #
dnl #  Copyright 2012 Alin Dobra and Christopher Jermaine
dnl #
dnl #  Licensed under the Apache License, Version 2.0 (the "License");
dnl #  you may not use this file except in compliance with the License.
dnl #  You may obtain a copy of the License at
dnl #
dnl #      http://www.apache.org/licenses/LICENSE-2.0
dnl #
dnl #  Unless required by applicable law or agreed to in writing, software
dnl #  distributed under the License is distributed on an "AS IS" BASIS,
dnl #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl #  See the License for the specific language governing permissions and
dnl #  limitations under the License.
dnl #
dnl # Macros that have to be defined
dnl # TOPK_NAME: name of the resulting GLA
dnl # TOPK_TUPLE: list of elements of the tuple (var, type)...

GLA_TEMPLATE_DESC(</TopK/>)

m4_define(</TopK/>, </dnl
m4_redefine(</TOPK_NAME/>, </$1/>)dnl
m4_redefine(</TOPK_TUPLE/>, </$2/>)dnl
dnl
m4_redefine(</MY_INPUT/>, </(_rank, FLOAT), />m4_defn(</TOPK_TUPLE/>))dnl
m4_redefine(</MY_OUTPUT/>, </(_rank, FLOAT), />m4_defn(</TOPK_TUPLE/>))dnl
m4_redefine(</MY_REZTYPE/>, </multi/>)dnl
m4_redefine(</MY_INIT/>, </(_limit, BIGINT)/>)dnl
#include "DataTypes.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <iomanip>
#include <iostream>

using namespace std;

/* Information for meta GLAs
 * GLA_DESC
 *
 * NAME(TOPK_NAME)
 * INPUTS(MY_INPUT)
 * OUTPUTS(MY_OUTPUT)
 * CONSTRUCTOR(MY_INIT)
 * RESULT_TYPE(MY_REZtYPE)
 *
 * END_DESC
 */

struct TOPK_NAME<//>_Tuple {
    FLOAT topKScore;
m4_foreach(</_A_/>,</TOPK_TUPLE/>,</dnl
    TYPE(_A_) VAR(_A_);
/>)dnl

    TOPK_NAME<//>_Tuple() {
        topKScore = 0;
    }

    // Use the initialization list to construct the members to ensure that
    // deep copies are made
    TOPK_NAME<//>_Tuple(FLOAT _rank, TYPED_REF_ARGS(TOPK_TUPLE)) :
        topKScore( _rank )
m4_foreach(</_A_/>,</TOPK_TUPLE/>,</dnl
        </, />VAR(_A_) </(/> VAR(_A_) </)/>
/>)dnl
    { }

    TOPK_NAME<//>_Tuple& operator=(const TOPK_NAME<//>_Tuple& _other) {
        topKScore = _other.topKScore;
m4_foreach(</_A_/>,</TOPK_TUPLE/>,</dnl
      VAR(_A_) = _other.VAR(_A_);
/>)dnl
        return *this;
    }
};

// Auxiliary function to compare tuples
inline bool GreaterThenTopK(TOPK_NAME<//>_Tuple& t1, TOPK_NAME<//>_Tuple& t2) {
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
class TOPK_NAME{
private:
    // types
    typedef vector<TOPK_NAME<//>_Tuple> TupleVector;

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
    void AddTupleInternal(TOPK_NAME<//>_Tuple& t);

public:
    // constructor & destructor
    TOPK_NAME<//>(int k) { count=0; K=k; pos = -1; worst = -1.0e+30; }
    ~TOPK_NAME<//>() {}

    // function to add an intem
    void AddItem(FLOAT _rank, TYPED_ARGS(TOPK_TUPLE));

    // take the state from ohter and incorporate it into this object
    // this is a + operator on TOPK_NAME
    void AddState(TOPK_NAME& other);

    // finalize the state and prepare for result extraction
    void Finalize() {
       Sort(); pos = 0;
       cout << "Total number of tuples in Topk=" << count << endl;
    }

    // iterator through the content in order (can be destructive)
    bool GetNextResult(FLOAT& _rank, TYPED_REF_ARGS(TOPK_TUPLE)) {
        if (pos == tuples.size())
            return false;
        else {
            TOPK_NAME<//>_Tuple& tuple = tuples[pos++];
m4_foreach(</_A_/>,</TOPK_TUPLE/>,</dnl
            VAR(_A_) = tuple.VAR(_A_);
/>)dnl
            return true;
        }
    }

};

void TOPK_NAME::AddItem(FLOAT _rank, TYPED_ARGS(TOPK_TUPLE)) {
    count++;
    if (_rank<worst) // fast path
                 return;

    TOPK_NAME<//>_Tuple tuple(_rank, ARGS(TOPK_TUPLE));

    AddTupleInternal(tuple);
}

void TOPK_NAME::AddTupleInternal(TOPK_NAME<//>_Tuple& tuple){
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

void TOPK_NAME::AddState(TOPK_NAME& other) {
    count+=other.count;
    // go over all the contents of other and insert it into ourselves
    for(int i = 0; i < other.tuples.size(); i++) {
      if (other.tuples[i].topKScore >= worst)
          AddTupleInternal(other.tuples[i]);
    }
}

/>)dnl

dnl # Synonym for compatibility reasons
GLA_TEMPLATE_DESC(</TopKGLA/>)
m4_define(</TopKGLA/>, m4_defn(</TopK/>))
