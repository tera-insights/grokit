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
dnl # TOPK_RANK: list of elements in the rank (var, type)...
dnl # TOPK_TUPLE: list of elements of the tuple (var, type)...

dnl # Define some useful macros first

m4_redefine(</m4_fix_order/>, </dnl
<//>m4_if(m4_to_upper(</$1/>), </ASC/>, </ASC/>, </m4_if(m4_to_upper(</$1/>), </DESC/>, </DESC/>, </ASC/>)/>)<//>dnl
/>)

m4_redefine(</ORDER/>,</m4_third($1)/>)

GLA_TEMPLATE_DESC(</OrderBy/>)

m4_define(</OrderBy/>,</dnl
m4_redefine(</TOPK_NAME/>, </$1/>)dnl
m4_redefine(</TOPK_TUPLE/>, </$3/>)dnl
dnl
m4_undefine_full(</TOPK_RANK/>)dnl
dnl
m4_redefine(</__TEMP__/>,</$2/>)dnl
m4_foreach(</_A_/>,</__TEMP__/>,</dnl
<//>m4_redefine(</_TARG_/>, m4_quote(</(/>VAR(_A_)</, />TYPE(_A_)</, />m4_fix_order(ORDER(_A_))</)/>))
<//>m4_append(</TOPK_RANK/>, _TARG_, </</, />/>)
/>)dnl
dnl
m4_define(</TOPK_RANK/>, m4_quote(TOPK_RANK))dnl
dnl
m4_redefine(</MY_INPUT/>, m4_quote(GLUE_LISTS(</$2/>, m4_quote(TOPK_TUPLE))))dnl
m4_redefine(</MY_OUTPUT/>, m4_defn(</MY_INPUT/>))dnl
m4_redefine(</MY_REZTYPE/>, </multi/>)dnl
dnl
m4_redefine(</MY_INIT/>, </(_limit, BIGINT)/>)dnl
dnl
#include "DataTypes.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <iomanip>
#include <iostream>
#include <inttypes.h>

using namespace std;

/** Information for Meta-GLAs
 *  GLA_DESC
 *
 *  NAME(TOPK_NAME)
 *  INPUTS(MY_INPUT)
 *  OUTPUTS(MY_OUTPUT)
 *  CONSTRUCTOR(MY_INIT)
 *  RESULT_TYPE(MY_REZTYPE)
 *
 *  END_DESC
*/

struct TOPK_NAME<//>_Tuple {
m4_foreach(</_A_/>,</MY_INPUT/>,</dnl
    TYPE(_A_) VAR(_A_);
/>)dnl

    TOPK_NAME<//>_Tuple() {
m4_foreach(</_A_/>,</TOPK_RANK/>,</dnl
        VAR(_A_) = TYPE(_A_)<//>();
/>)dnl
    }

    // This constructor uses the initialization list to construct each
    // element using its copy constructor to ensure a deep copy
    TOPK_NAME<//>_Tuple(TYPED_CONST_REF_ARGS(MY_INPUT)) :
m4_undefine_full(</_TMP_/>)dnl
m4_foreach(</_A_/>, </MY_INPUT/>, </dnl
        m4_ifndef(</_TMP_/>, </m4_define(</_TMP_/>)/>, </, />)dnl
VAR(_A_)</(/> VAR(_A_) </)/>
/>)dnl
    { }

    TOPK_NAME<//>_Tuple& operator=(const TOPK_NAME<//>_Tuple& _other) {
m4_foreach(</_A_/>,</MY_INPUT/>,</dnl
        VAR(_A_) = _other.VAR(_A_);
/>)dnl
        return *this;
    }
};

dnl # Possible alternative to below function:
dnl # Have 2 bitmaps, one for ranks in t1 that are greater than in t2,
dnl # and another for ranks in t2 that are greater than in t1.
dnl # (MSB = most significant ranking variable)
dnl # then return bitmap1 > bitmap2
dnl # This avoids any branches, but requires that all ranking variables be
dnl # checked every time.
dnl # This would be faster for simple types, but could be bad if a rank is a
dnl # complex type that requires a lot of time to compare (like strings)
// Auxiliary function to compare tuples
inline bool GreaterThanTopK( const TOPK_NAME<//>_Tuple& t1, const TOPK_NAME<//>_Tuple& t2) {
m4_foreach(</_A_/>,</TOPK_RANK/>,</dnl
    if( (TYPE(_A_)) t1.VAR(_A_) m4_if(ORDER(_A_), </ASC/>, <, >) (TYPE(_A_)) t2.VAR(_A_) )
        return true;
    else if( (TYPE(_A_)) t1.VAR(_A_) m4_if(ORDER(_A_), </ASC/>, >, <) (TYPE(_A_)) t2.VAR(_A_) )
        return false;

/>)dnl
    return false;
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

 * Assumptions: The input tuple has as a member one or more "rank" values that
 * support the >, <, and >= operators.
**/
class TOPK_NAME{
private:
    // types
    typedef vector<TOPK_NAME<//>_Tuple> TupleVector;

    long long int count; // number of tuples covered

    // k as in top-k
    int K;

    // worst tuple in the heap
m4_foreach(</_A_/>,</TOPK_RANK/>,</dnl
    TYPE(_A_) VAR(_A_);
/>)<//>dnl

    TupleVector tuples;
    int pos; // position of the output iterator

    // function to force sorting so that GetNext gets the tuples in order
    void Sort() {
        // If tuples doesn't contain at least K elements, it was never made
        // into a heap in the first place, so just sort it normally.
        if( __builtin_expect(tuples.size() < K, 0 ) )
            sort(tuples.begin(), tuples.end(), GreaterThanTopK);
        else
            sort_heap(tuples.begin(), tuples.end(), GreaterThanTopK);
    }

    // internal function
    void AddTupleInternal(TOPK_NAME<//>_Tuple& t);

public:
    // constructor & destructor
    TOPK_NAME<//>(int k) { count=0; K=k; pos = -1; }
    ~TOPK_NAME<//>() {}

    // function to add an intem
    void AddItem(TYPED_CONST_REF_ARGS(MY_INPUT));

    // take the state from other and incorporate it into this object
    // this is a + operator on TOPK_NAME
    void AddState(TOPK_NAME& other);

    // finalize the state and prepare for result extraction
    void Finalize() {
       Sort(); pos = 0;
  }

    // iterator through the content in order (can be destructive)
    bool GetNextResult(TYPED_REF_ARGS(MY_OUTPUT)) {
        if (pos == tuples.size())
            return false;
        else {
            TOPK_NAME<//>_Tuple& tuple = tuples[pos++];
m4_foreach(</_A_/>,</MY_OUTPUT/>,</dnl
            VAR(_A_) = tuple.VAR(_A_);
/>)dnl
            return true;
        }
    }

};

void TOPK_NAME::AddItem(TYPED_CONST_REF_ARGS(MY_INPUT)) {

    TOPK_NAME<//>_Tuple tuple(ARGS(MY_INPUT));

    if( tuples.size() == K && !GreaterThanTopK( tuple, tuples.front() ) )
                 return;

    AddTupleInternal(tuple);
}

void TOPK_NAME::AddTupleInternal(TOPK_NAME<//>_Tuple& tuple){
    ++count;

    if (tuples.size() < K) {
        tuples.push_back(tuple);

        // when we have exactly K elements in the vector, organize it as a heap
        if (tuples.size() == K) {
            make_heap(tuples.begin(), tuples.end(), GreaterThanTopK);
m4_foreach(</_A_/>,</TOPK_RANK/>,</dnl
            this->VAR(_A_) = tuples.front().VAR(_A_);
/>)<//>dnl
        }
    }
    else {
        pop_heap(tuples.begin(), tuples.end(), GreaterThanTopK);

        tuples.pop_back();
        tuples.push_back(tuple) ;
        push_heap(tuples.begin(), tuples.end(), GreaterThanTopK);
m4_foreach(</_A_/>,</TOPK_RANK/>,</dnl
            this->VAR(_A_) = tuples.front().VAR(_A_);
/>)<//>dnl
    }
}

void TOPK_NAME::AddState(TOPK_NAME& other) {
    // go over all the contents of other and insert it into ourselves
    for(int i = 0; i < other.tuples.size(); i++) {

        if( tuples.size() < K || GreaterThanTopK( other.tuples[i], tuples.front() ) )
            AddTupleInternal(other.tuples[i]);
    }
}
/>)

dnl # Synonym for compatibility reasons
GLA_TEMPLATE_DESC(</OrderByGLA/>)
m4_define(</OrderByGLA/>, m4_defn(</OrderBy/>))
