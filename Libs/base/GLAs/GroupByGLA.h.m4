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
dnl # the following macros should be defined when including this file
dnl # GLA_NAME: name of the produced class. This must coincide withe the produced .h file
dnl # INNER_GLA: the name of the inner GLA
dnl # NameOfInnerGLA_INPUT: list of inputs for the inner gla
dnl # NameOfInnerGLA_OUTPUT: list of outputs.
dnl # GBY_ATTS: list of group  by attrbutes
dnl # All lists are of the form </(arg1, type1),.../>

dnl # Example: see GBy_Averrage.h.m4

dnl # Usage
dnl # $1=GLA_NAME name of the class generated
dnl # $2=GBY_ATTS, list of group by attributes
dnl # $3=INNER_GLA, name of the GLA used for each group
dnl # this GLA must be in the file GLA.h and have a description
dnl # specifying the input

GLA_TEMPLATE_DESC(</GroupBy/>)

m4_define(</GroupBy/>,</dnl
m4_ifdef(</INNER_GLA/>,</m4_undefine(</INNER_GLA/>)/>)dnl
m4_ifdef(</GLA_NAME/>,</m4_undefine(</GLA_NAME/>)/>)dnl
m4_ifdef(</GBY_ATTS/>,</m4_undefine(</GBY_ATTS/>)/>)dnl
m4_define(</INNER_GLA/>, </$3/>)dnl
m4_define(</GLA_NAME/>, </$1/>)dnl
m4_define(</GBY_ATTS/>,</$2/>)dnl
dnl
m4_define(</USE_FRAGMENTS/>,</NUM_EXEC_ENGINE_THREADS/>)dnl
dnl
m4_define(</MY_REZTYPE/>, m4_ifdef(</USE_FRAGMENTS/>, </fragment/>, </multi/>))dnl
dnl
m4_ifndef(INNER_GLA</_INPUT/>, </SCAN_GLA_FILE(INNER_GLA)/>)dnl
dnl
m4_ifdef(INNER_GLA</_INIT/>, </dnl
<//>m4_redefine(</MY_INIT/>, m4_defn(INNER_GLA</_INIT/>))<//>dnl
/>, </dnl
<//>m4_redefine(</MY_INIT/>, <//>)<//>dnl
/>)dnl
dnl
m4_redefine(</MY_INPUT/>, m4_quote(GLUE_LISTS(</GBY_ATTS/>, m4_quote(INNER_GLA</_INPUT/>))))dnl

dnl # Special case for inner result type of state
m4_if(m4_defn(INNER_GLA</_REZTYPE/>), state, </dnl
m4_redefine(</MY_OUTPUT/>, m4_quote(GLUE_LISTS(</GBY_ATTS/>, </(state, STATE)/>)))dnl
/>, </dnl
m4_redefine(</MY_OUTPUT/>, m4_quote(GLUE_LISTS(</GBY_ATTS/>, m4_quote(INNER_GLA</_OUTPUT/>))))dnl
/>)dnl

/* Information for meta GLAs
 * GLA_DESC
 *
 * NAME(GLA_NAME)
 * INPUTS(MY_INPUT)
 * OUTPUTS(MY_OUTPUT)
 * CONSTRUCTOR(MY_INIT)
 * RESULT_TYPE(MY_REZTYPE)
 *
 * END_DESC
 */

#include <iomanip>
#include <iostream>
#include "HashFunctions.h"
#include <string.h>

#define INIT_SIZE 100000
#define USE_MCT

using namespace std;

#ifdef USE_TR1
#include <tr1/unordered_map>
using namespace std::tr1;
#define DEFINED_MAP std::tr1::unordered_map
#endif // USE_TR1

#ifdef USE_MCT
#include <mct/hash-map.hpp>
using namespace mct;
#define DEFINED_MAP mct::closed_hash_map
#endif // USE_MCT


struct Key_<//>GLA_NAME {
dnl # member variables
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
    TYPE(_A_) VAR(_A_);
/>)dnl

dnl # constructor
  Key_<//>GLA_NAME (TYPED_CONST_REF_ARGS(GBY_ATTS)) :
m4_undefine_full(</_TMP_/>)dnl
m4_foreach(</_A_/>, </GBY_ATTS/>, </dnl
        m4_ifndef(</_TMP_/>, </m4_define(</_TMP_/>)/>, </, />)dnl
VAR(_A_)</(/> VAR(_A_) </)/>
/>)dnl
    { }

    bool operator==(const Key_<//>GLA_NAME& o) const {
        return (true<//>dnl
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
 && VAR(_A_) == o.VAR(_A_)<//>dnl
/>)dnl
);
    }

    size_t hash_value() const {
        uint64_t hash= H_b;
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
        hash = CongruentHash(Hash(VAR(_A_)), hash);
/>)dnl
        return (size_t) hash;
    }
};

struct HashKey_<//>GLA_NAME {
    size_t operator()(const Key_<//>GLA_NAME& o) const {
        return o.hash_value();
    }
};

class GLA_NAME{
private:
    typedef DEFINED_MAP<Key_<//>GLA_NAME, INNER_GLA, HashKey_<//>GLA_NAME> MapType;

    MapType groupByMap;

m4_ifdef(</USE_FRAGMENTS/>,</dnl
     vector<MapType::iterator> theIterators;  // the iterators, only 2 elements if multi, many if fragment
/>,</dnl
     MapType::iterator theIterator;
/>)dnl

m4_ifval(MY_INIT, </dnl
<//>m4_foreach(</_A_/>, </MY_INIT/>, </dnl
    TYPE(_A_) VAR(_A_);
<//>/>)dnl
/>)dnl

public:
m4_ifval(MY_INIT, </dnl
    GLA_NAME<//>(TYPED_ARGS(MY_INIT)) : groupByMap( INIT_SIZE )
<//>m4_foreach(</_A_/>, </MY_INIT/>, </dnl
        </, />VAR(_A_) </(/> VAR(_A_) </)/>
<//>/>)dnl
    { }
/>, </dnl
    GLA_NAME</():groupByMap(INIT_SIZE)/> {}
/>)dnl
    ~GLA_NAME</()/> {}

    void AddItem(TYPED_CONST_REF_ARGS(MY_INPUT)) {
        // check if _key is already in the map; if yes, add _value; else, add a new
        // entry (_key, _value)
        Key_<//>GLA_NAME key(ARGS(GBY_ATTS));

        MapType::iterator it = groupByMap.find(key);
        if (it == groupByMap.end()) { // group does not exist
            // create an empty GLA and insert
            // better to not add the item here so we do not have
            // to transport a large state
dnl         # Note: I have to do this annoying check to make sure ARGS(MY_INIT)
dnl         # is not empty before putting the parentheses because if you just
dnl         # put a pair of empty parentheses there, instead of calling the
dnl         # default constructor (which is explicity defined!), gcc will just
dnl         # freak out and think you're defining some other bizarre type.
            INNER_GLA gla<//>m4_ifval(ARGS(MY_INIT), </(/>ARGS(MY_INIT)</)/>, <//>);
            // Key_<//>GLA_NAME key(ARGS(GBY_ATTS));
            groupByMap.insert(MapType::value_type(key, gla));
            it = groupByMap.find(key); // reposition
        }
        it->second.AddItem(ARGS(INNER_GLA</_INPUT/>));
    }

    void AddState(GLA_NAME& other) {
        // scan other hash and insert or update content in this one
        for (MapType::iterator it = other.groupByMap.begin(); it != other.groupByMap.end();
                ++it) {
            const Key_<//>GLA_NAME& okey = it->first;
            INNER_GLA& ogla = it->second;

            MapType::iterator itt = groupByMap.find(okey);
            if (itt != groupByMap.end()) { // found the group
                INNER_GLA& gla = itt->second;
                gla.AddState(ogla);
            } else {
                // add the other group to this hash
                groupByMap.insert(MapType::value_type(okey, ogla));
            }
        }
    }


m4_ifdef(</USE_FRAGMENTS/>,</dnl use fragment interface
    struct GLA_NAME<//>_Iterator {
        MapType::iterator it; // current value
        MapType::iterator end; // last value in the fragment

        GLA_NAME<//>_Iterator(MapType::iterator& _it, MapType::iterator& _end):
            it(_it), end(_end){}
    };

    int GetNumFragments(void){
        int size = groupByMap.size();
        int sizeFrag = size/USE_FRAGMENTS;
        // setup the fragment boundaries
        // scan via iterator and count
        int frag=0;
        int pos=0;
        MapType::iterator it = groupByMap.begin();
        theIterators.clear();
        theIterators.push_back( it );
        // special case when size<num_fragments
        if (sizeFrag == 0){
            it = groupByMap.end();
            theIterators.push_back( it );
            return 1; // one fragment
        }

        while(it!=groupByMap.end()){
            while(it!=groupByMap.end() && pos<( frag + 1 )*sizeFrag){
                ++it;
                pos++;
            }
            theIterators.push_back( it );
            frag++;
        }

        return frag;

    }

    GLA_NAME<//>_Iterator* Finalize(int fragment){
        // Call finalize on all inner GLAs in this fragment.
        MapType::iterator iter = theIterators[fragment];
        MapType::iterator iterEnd = theIterators[fragment+1];

        for( ; iter != iterEnd; ++iter ) {
            INNER_GLA & gla = iter->second;
m4_case(reval(</GLA_REZTYPE_/>INNER_GLA), </fragment/>, </dnl
</#/>error Finalizing fragmented inner GLAs not supported.
/>, </multi/>, </dnl
            gla.Finalize();
/>)dnl
        }

        GLA_NAME<//>_Iterator* rez
            = new GLA_NAME<//>_Iterator(theIterators[fragment], theIterators[fragment+1] );
        return rez;
    }

    bool GetNextResult(GLA_NAME<//>_Iterator* it,  TYPED_REF_ARGS(MY_OUTPUT)) {
        MapType::iterator& theIterator = it->it;
        MapType::iterator& endIt = it->end;
/>,</dnl use multi interface

    void Finalize() {
        theIterator = groupByMap.begin();
        for( ; iter != iterEnd; ++iter ) {
            INNER_GLA & gla = iter->second;
m4_if(reval(</GLA_REZTYPE_/>INNER_GLA), </fragment/>, </dnl
</#/>error Finalizing fragmented inner GLAs not supported.
/>, </multi/>, </dnl
            gla.Finalize();
/>)dnl
        }
    }

    bool GetNextResult(TYPED_REF_ARGS(MY_OUTPUT)) {
        MapType::iterator endIt = groupByMap.end();
/>)dnl

        if (theIterator == endIt) {
            return false;
        }
        else {
            FATALIF(theIterator == groupByMap.end(), "WHY??");

m4_case(reval(INNER_GLA</_REZTYPE/>), </single/>, </dnl
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
            VAR(_A_) = theIterator->first.VAR(_A_);
/>)dnl
            INNER_GLA& gla = theIterator->second;
            gla.GetResult(ARGS(INNER_GLA</_OUTPUT/>));
            ++theIterator;

            return true;
/>, </multi/>, </dnl
            bool gotResult = false;
            while( theIterator != endIt && !gotResult ) {
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
                VAR(_A_) = theIterator->first.VAR(_A_);
/>)dnl

                INNER_GLA& gla = theIterator->second;
                gotResult = gla.GetNextResult(ARGS(INNER_GLA</_OUTPUT/>));
                if( !gotResult )
                    ++theIterator;
            }

            return gotResult;
/>, </state/>, </dnl
m4_foreach(</_A_/>,</GBY_ATTS/>,</dnl
            VAR(_A_) = theIterator->first.VAR(_A_);
/>)dnl
            INNER_GLA& gla = theIterator->second;
            state = STATE((void*) &gla, M4_HASH_NAME(INNER_GLA));
            ++theIterator;

            return true;
/>, </dnl
</#/>error Unsupported inner GLA result type.
/>)dnl
        }
    }
};
m4_ifdef(</USE_FRAGMENTS/>,</dnl use fragment interface
typedef GLA_NAME::GLA_NAME<//>_Iterator GLA_NAME<//>_Iterator;
/>)dnl
/>)dnl # end of the GroupByTemplate

dnl # Synonym for compatibility reasons
GLA_TEMPLATE_DESC(</GroupByGLA/>)
m4_define(</GroupByGLA/>, m4_defn(</GroupBy/>))
