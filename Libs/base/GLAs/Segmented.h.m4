dnl #
dnl #  Copyright 2012 Christopher Dudley
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
dnl # This macro generates a class that represents a GLA with state that has
dnl # been split into several distinct segments. These segments are global.
dnl # Data is first aggregated into a set of local states, and then merged
dnl # into the global states after a chunk has been fully processed.
dnl # This allows for larger amounts of information to be stored in the
dnl # state, as there will only be one set active in the system for any
dnl # real length of time. This also allows results to be extracted from
dnl # the GLA segments in parallel.

dnl # TODO: Make it so that instead of getting a BIGINT key, it instead
dnl # has an object of a particular class that it gives information to
dnl # and receives a key from. Could possibly just be a templated
dnl # function.

GLA_TEMPLATE_DESC(</Segmented/>)
dnl # Usage
dnl # $1 = GLA_NAME     name of the class generated
dnl # $2 = SEG_KEY      data used to generate the key for the tuple
dnl #                   currently a tuple (name, type)
dnl # $3 = INNER_GLA    name of the GLA that will be segmented.
dnl # $4 = KEY_GEN      name of the class used to generate keys.
m4_define(</Segmented/>, </dnl
m4_redefine(</GLA_NAME/>, </$1/>)dnl
m4_redefine(</SEG_KEY/>, </$2/>)dnl
m4_redefine(</INNER_GLA/>, </$3/>)dnl
m4_redefine(</KEY_GEN/>, </$4/>)dnl
dnl
m4_define(</USE_FRAGMENTS/>, </NUM_EXEC_ENGINE_THREADS/>)dnl
dnl
m4_define(</MY_REZTYPE/>, </fragment/>)dnl
dnl
m4_redefine(</MY_INPUT/>, m4_quote(GLUE_LISTS(m4_quote(SEG_KEY), m4_quote(m4_defn(INNER_GLA</_INPUT/>)))))dnl
m4_redefine(</MY_OUTPUT/>, m4_quote(m4_defn(INNER_GLA</_OUTPUT/>)))dnl
m4_ifdef(INNER_GLA</_INIT/>, </dnl
<//>m4_redefine(</MY_INIT/>, m4_quote(m4_defn(INNER_GLA</_INIT/>)))<//>dnl
/>, </dnl
<//>m4_redefine(</MY_INIT/>, <//>)<//>dnl
/>)dnl
m4_redefine(</INNER_INPUT/>, m4_quote(m4_defn(INNER_GLA</_INPUT/>)))dnl
m4_redefine(</INNER_OUTPUT/>, m4_quote(m4_defn(INNER_GLA</_OUTPUT/>)))dnl
m4_redefine(</INNER_REZTYPE/>, m4_quote(m4_defn(</GLA_REZTYPE_/>INNER_GLA)))dnl

/* Information for meta GLAs
 * GLA_DESC
 *
 * NAME(GLA_NAME)
 * INPUTS(MY_INPUT)
 * OUTPUTS(MY_OUTPUT)
 * CONSTRUCTOR(MY_INIT)
 * RESULT_TYPE(MY_REZTYPE)
 *
 * OPT_CHUNK_BOUNDARY
 *
 * END_DESC
 */

#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <inttypes.h>
#include "HashFunctions.h"
#include <utility>
#include <map>
#include "base/include/SplitState.h"

using namespace std;

class GLA_NAME {

    // Static global state
    static SplitState<INNER_GLA> globalStates;

m4_ifval(KEY_GEN, </dnl
    // Static key generator
    static KEY_GEN keyGenerator;

/>)dnl
    // Local state
    INNER_GLA ** localState;

public:
    // Forward definition.
    class GLA_NAME</_Iterator/>;

private:
    // Used to store the iterators.
    vector< GLA_NAME</_Iterator/> > myIterators;

public:

    // Constructor
    GLA_NAME<//>() {
        localState = new INNER_GLA *[USE_FRAGMENTS];

        for( size_t i = 0; i < USE_FRAGMENTS; ++i ) {
            localState[i] = new INNER_GLA<//>(MY_INIT);
        }
    }

    void AddItem( TYPED_CONST_REF_ARGS(MY_INPUT) ) {
m4_ifval(KEY_GEN, </dnl
        uint64_t segNum = keyGenerator.GetKey( ARGS(SEG_KEY) );
        segNum %= USE_FRAGMENTS;
/>, </dnl
        uint64_t segNum = H_b;
m4_foreach(</_A_/>, </SEG_KEY/>, </dnl
        segNum = CongruentHash( Hash( VAR(_A_) ), segNum );
/>)dnl
        segNum = segNum % USE_FRAGMENTS;
/>)dnl

        localState[segNum]->AddItem( ARGS(INNER_INPUT) );
    }

    void ChunkBoundary(void) {
        // Merge the local states into the global state.

        int theseAreOk[USE_FRAGMENTS];
        for( int i = 0; i < USE_FRAGMENTS; ++i )
            theseAreOk[i] = 1;

        int segsLeft = USE_FRAGMENTS;

        while( segsLeft > 0 ) {
            INNER_GLA * checkedOut = NULL;

            int whichOne = globalStates.CheckOutOne( theseAreOk, checkedOut );

            if( checkedOut == NULL ) {
                checkedOut = new INNER_GLA<//>(MY_INIT);
            }

            checkedOut->AddState( *( localState[whichOne] ) );

            globalStates.CheckIn( whichOne, checkedOut );

            theseAreOk[whichOne] = 0;
            --segsLeft;
        }

        // Re-initialize the local states
        for( int i = 0; i < USE_FRAGMENTS; ++i ) {
            delete localState[i];
            localState[i] = new INNER_GLA<//>(MY_INIT);
        }
    }

    void AddState( GLA_NAME & other ) {
        // Do absolutely nothing.
    }

    class GLA_NAME<//>_Iterator {
        INNER_GLA * myState;
m4_if( INNER_REZTYPE, fragment, </dnl
        int fragNum;
        INNER_GLA</_Iterator/> * iter;    // Iterator for the fragment.
/>)dnl

    public:
        GLA_NAME<//>_Iterator<//>( int segNum, int fragNum ) : fragNum( fragNum ), iter(NULL) {
            myState = globalStates.Peek( segNum );
        }

        ~GLA_NAME</_Iterator/>( ) { }

        void Finalize() {
m4_case( INNER_REZTYPE, fragment, </dnl
            iter = myState->Finalize( fragNum );
/>, multi, </dnl
            myState->Finalize();
/>)dnl
        }

        bool GetNextResult( TYPED_REF_ARGS(MY_OUTPUT) ) {
m4_case(INNER_REZTYPE, single, </dnl
</#/>error Having an inner GLA that returns a single value makes no sense here!
/>, multi, </dnl
            return myState->GetNextResult( ARGS(MY_OUTPUT) );
/>, fragment, </dnl
            return myState->GetNextResult( iter, ARGS(MY_OUTPUT) );
/>, </dnl
</#/>error Result type INNER_REZTYPE of inner GLA unsupported.
/>)dnl
        }
    };

    int GetNumFragments(void) {
        int myFrags = 0;
        myIterators.clear();

        for( int i = 0; i < USE_FRAGMENTS; ++i ) {
            INNER_GLA * statePtr = globalStates.Peek( i );

            if( statePtr != NULL ) {
m4_if( INNER_REZTYPE, fragment, </dnl
                int numFrags = statePtr->GetNumFragments();

                for( int j = 0; j < numFrags; ++j ) {
                    ++myFrags;
                    myIterators.push_back( GLA_NAME</_Iterator/>( i, j ) );
                }
/>, </dnl
                ++myFrags;
                myIterators.push_back( GLA_NAME</_Iterator/>(i, 0) );
/>)dnl
            }
        }

        return myFrags;
    }

    GLA_NAME</_Iterator/> * Finalize( int fragment ) {
        // Get the iterator.
        GLA_NAME</_Iterator/> & iter = myIterators[fragment];

        // Finalize
        iter.Finalize();

        return &iter;
    }

    bool GetNextResult( GLA_NAME</_Iterator/> * it, TYPED_REF_ARGS(MY_OUTPUT) ) {
        return it->GetNextResult( ARGS(MY_OUTPUT) );
    }
};

// Initializers for static members

SplitState<INNER_GLA> GLA_NAME :: globalStates = SplitState<INNER_GLA>(USE_FRAGMENTS);
m4_ifval(KEY_GEN, </dnl
KEY_GEN GLA_NAME :: keyGenerator = KEY_GEN<//>();
/>)dnl

// Typedef for iterator type
typedef GLA_NAME::GLA_NAME</_Iterator/> GLA_NAME</_Iterator/>;

/>)
