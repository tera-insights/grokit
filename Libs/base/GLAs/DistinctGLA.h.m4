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
dnl # DistinctGLA Template
dnl #
dnl # Author: Christopher Dudley
dnl # Based on GroupByGLA.h.m4 by Alin Dobra
dnl #
dnl # This GLA takes in tuples and outputs only those that are distinct,
dnl # i.e. that tuple is different in at least one attribute than all other
dnl # tuples.
dnl
dnl # the following macros should be defined when including this file
dnl # GLA_NAME: name of the produced class. This must coincide withe the produced .h file
dnl # DST_ATTS: list of distinct attrbutes
dnl # All lists are of the form </(arg1, type1),.../>

GLA_TEMPLATE_DESC(</Distinct/>)
dnl # Usage
dnl # $1 = GLA_NAME   name of the class generated
dnl # $2 = DST_ATTS   list of distinct attributes
m4_define(</Distinct/>,</dnl
m4_ifndef(</USE_FRAGMENTS/>, </dnl
<//>m4_define(</USE_FRAGMENTS/>, </NUM_EXEC_ENGINE_THREADS/>)dnl
/>)dnl
m4_ifdef(</GLA_NAME/>,</m4_undef(</GLA_NAME/>)/>)dnl
m4_ifdef(</DST_ATTS/>,</m4_undef(</GBY_ATTS/>)/>)dnl
m4_define(</GLA_NAME/>, </$1/>)dnl
m4_define(</DST_ATTS/>,m4_quote($2))dnl
m4_redefine(</MY_REZTYPE/>, m4_ifdef(</USE_FRAGMENTS/>, </fragment/>, </multi/>))dnl
dnl
/* Information for meta GLAs
 * GLA_DESC
 *
 * NAME(GLA_NAME)
 * INPUTS(DST_ATTS)
 * OUTPUTS(DST_ATTS)
 * RESULT_TYPE(MY_REZTYPE)
 *
 * END_DESC
 */

#include <iomanip>
#include <iostream>
#include "HashFunctions.h"
#include <inttypes.h>

#define INIT_SIZE 100000
#define USE_MCT

using namespace std;

#ifdef USE_TR1
#include <tr1/unordered_set>
using namespace std::tr1;
#define DEFINED_SET std::tr1::unordered_set
#endif // USE_TR1

#ifdef USE_MCT
#include <mct/hash-set.hpp>
using namespace mct;
#define DEFINED_SET mct::closed_hash_set
#endif // USE_MCT

struct Key_<//>GLA_NAME {
dnl # member variables
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
    TYPE(_A_) VAR(_A_);
/>)dnl



dnl # constructor
  Key_<//>GLA_NAME (TYPED_REF_ARGS(DST_ATTS)) :
m4_undefine_full(</_TMP_/>)dnl
m4_foreach(</_A_/>, </DST_ATTS/>, </dnl
        m4_ifndef(</_TMP_/>, </m4_define(</_TMP_/>)/>, </, />)dnl
VAR(_A_)</(/> VAR(_A_) </)/>
/>)dnl
    { }

    bool operator==(const Key_<//>GLA_NAME& o) const {
        return (true dnl
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
 && (TYPE(_A_)) VAR(_A_) == (TYPE(_A_)) o.VAR(_A_)<//>dnl
/>) );
    }

    size_t hash_value() {
        uint64_t hash= H_b;
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
        hash = CongruentHash(Hash(VAR(_A_)), hash);
/>)dnl
        return (size_t) hash;
    }

};

struct HashKey_<//>GLA_NAME {
    size_t operator()(const Key_<//>GLA_NAME& o) const {
        Key_<//>GLA_NAME& newObject = const_cast<Key_<//>GLA_NAME&>(o);
        return newObject.hash_value();
    }
};

class GLA_NAME {

      uint64_t tuplesSeen;
      uint64_t countDistinct;

      typedef DEFINED_SET<Key_<//>GLA_NAME, HashKey_<//>GLA_NAME> SetType;

      SetType distinctSet;
m4_ifdef(</USE_FRAGMENTS/>, </dnl
      vector<SetType::iterator> theIterators;
/>, </dnl
      SetType::iterator theIterator;
/>)dnl

public:
    GLA_NAME</():distinctSet(INIT_SIZE), countDistinct(0), tuplesSeen(0)/> {}
    ~GLA_NAME</()/> {}

  void AddItem(TYPED_ARGS(DST_ATTS)) {
    Key_<//>GLA_NAME key(ARGS(DST_ATTS));

        SetType::iterator it = distinctSet.find(key);
        if (it != distinctSet.end()) { // group exists
            // Do nothing
        }
        else {
            // Insert the data
            distinctSet.insert(key);
            ++countDistinct;
        }
        ++tuplesSeen;
    }

    void AddState(GLA_NAME& other) {
             uint64_t prevCount = countDistinct;
        uint64_t prevSeen = tuplesSeen;
        // scan other hash and insert or update content in this one
        for (SetType::iterator it = other.distinctSet.begin(); it != other.distinctSet.end();
            ++it) {
            const Key_<//>GLA_NAME& okey = *it;

            SetType::iterator itt = distinctSet.find(okey);
            if (itt != distinctSet.end()) { // found the group
                // Do nothing
            } else {
                // add the other group to this hash
                distinctSet.insert(okey);
                ++countDistinct;
            }
        }
        tuplesSeen += other.tuplesSeen;

        //cerr << "Merge done. prevCount: " << prevCount << " prevSeen: " << prevSeen << endl;
        //cerr << "countDistinct: " << countDistinct << " tuplesSeen: " << tuplesSeen << endl;
    }

m4_ifdef(</USE_FRAGMENTS/>,</dnl # use fragment interface
    struct GLA_NAME<//>_Iterator {
        SetType::iterator it; // current value
        SetType::iterator end; // last value in the fragment

        GLA_NAME<//>_Iterator(SetType::iterator& _it, SetType::iterator& _end):
            it(_it), end(_end) {}
    };

    int GetNumFragments(void) {
         SetType::size_type size = distinctSet.size();
m4_ifval(USE_FRAGMENTS,</dnl
         SetType::size_type sizeFrag = size / USE_FRAGMENTS;
/>, </dnl
         SetType::size_type sizeFrag = 100000;
/>)dnl
         theIterators.clear();

         // setup the fragment boundaries
         // scan via iterator and count
         int frag = 0;
         SetType::size_type pos = 0;
         SetType::iterator it = distinctSet.begin();
         theIterators.push_back( it );
         // special case when size < num_fragments
         if (sizeFrag == 0) {
             theIterators.push_back( distinctSet.end() );
             return 1; // one fragment
         }
         while( it != distinctSet.end() ) {
             while( it != distinctSet.end() && pos < (frag + 1) * sizeFrag ) {
                 ++it;
                 pos++;
             }
             theIterators.push_back( it );
             frag++;
         }

         return frag;
    }

    GLA_NAME<//>_Iterator* Finalize(int fragment) {
        GLA_NAME<//>_Iterator* rez = new GLA_NAME<//>_Iterator(theIterators[fragment], theIterators[fragment+1] );
        return rez;
    }

    bool GetNextResult(GLA_NAME<//>_Iterator* it, TYPED_REF_ARGS(DST_ATTS)) {
        SetType::iterator& theIterator = it->it;
        SetType::iterator& endIt = it->end;

        if (theIterator == endIt) {
            return false;
        }
        else {
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
            VAR(_A_) = theIterator->VAR(_A_);
/>)dnl

            ++theIterator;
            return true;
        }
    }
/>, </dnl # use multi interface
    void Finalize() {
        theIterator = distinctSet.begin();
        //cerr << "Distinct count: " << countDistinct << " tuples seen: " << tuplesSeen << endl;
    }

    bool GetNextResult(TYPED_REF_ARGS(DST_ATTS)) {
        if (theIterator == distinctSet.end()) {
            return false;
        }
        else {
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
            VAR(_A_) = theIterator->VAR(_A_);
           />)dnl
            ++theIterator;

            return true;
        }
    }
/>)

    static uint64_t Map(TYPED_REF_ARGS(DST_ATTS)) {
           uint64_t h = H_b;
m4_foreach(</_A_/>,</DST_ATTS/>,</dnl
        h = CongruentHash(Hash(VAR(_A_)), h);
/>)dnl
        return h;
    }

};

m4_ifdef(</USE_FRAGMENTS/>,</dnl # use fragment interface
typedef GLA_NAME::GLA_NAME<//>_Iterator GLA_NAME<//>_Iterator;
/>)dnl

/>)dnl # end of the DistinctTemplate

dnl # Synonym for compatibility reasons
GLA_TEMPLATE_DESC(</DistinctGLA/>)
m4_define(</DistinctGLA/>, m4_defn(</Distinct/>))
