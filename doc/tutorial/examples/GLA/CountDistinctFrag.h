//
//  Copyright 2012 Christopher Dudley
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#ifndef _COUNT_DISTINCT_FRAG_H_
#define _COUNT_DISTINCT_FRAG_H_

// Standard library includes
#include <map>
#include <vector>
#include <cinttypes>

// DataPath library includes
#include "base/Types/INT.h"

//! [desc]
/*  System Description of GLA
 *  GLA_DESC
 *      NAME(</CountDistinct/>)
 *      INPUTS(</(x, INT)/>)
 *      OUTPUTS(</(x, INT), (count, INT)/>)
 *      RESULT_TYPE(</fragment/>)
 *  END_DESC
 */
//! [desc]

/*!
 *  @brief  A GLA to track distinct elements in a set of numbers, and how many
 *          times each distinct element appeared.
 */

class CountDistinctFrag {

    // A map type for mapping numbers to counts.
    typedef std::map<INT, INT> MyMap;

    // Map containing mapping of distinct elements to their count.
    MyMap countMap;

    // Iterators representing the different fragments of the output.
    typedef std::vector<MyMap::const_iterator> IterVec;
    IterVec iterators;

    // Constant defining the number of result tuples per fragment produced.
    static const uint64_t FRAGMENT_SIZE = 100000;

public:

    /*!
     *  Default constructor. It doesn't have to do anything special, as default
     *  initialization of the members is fine.
     */
    CountDistinctFrag() { }

    /*!
     *  Default destructor. It doesn't have to do anything special, as default
     *  destruction of its members is fine.
     */
    ~CountDistinctFrag() { }

    /*!
     *  Takes a single element of the dataset, and increments the counter for
     *  it.
     *
     *  @param[in]  x   The element from the dataset.
     */
    //! [add-item]
    void AddItem( const INT& x ) {
        // If no count for x exists, the map will create a count for it using
        // the default constructor for INT, will will give it a value of 0.
        countMap[x] += 1;
    }
    //! [add-item]

    /*!
     *  Adds the counts from other into this state.
     *
     *  @param[in]  other   The state to add to this one.
     */
    //! [add-state]
    void AddState( CountDistinct& other ) {
        // Reference to the other map to improve the readibility of the code.
        const MyMap & otherMap = other.countMap;

        for( MyMap::const_iterator it = otherMap.begin();
                it != otherMap.end();
                ++it ) {

            // Like in AddItem, if the element doesn't have a count in our map
            // yet, it will default to 0, which we will then immediately
            // increment.
            countMap[it->first] += it->second;
        }
    }
    //! [add-state]

    /*!
     *  Returns the number of fragments that will be produced and prepares
     *  the internal iterators needed for producing the fragments.
     *
     *  @return the number of fragments to be produced.
     */
    //! [get-num-fragments]
    int GetNumFragments( void ) {
        // Make sure we actually have some elements to produce
        if( countMap.begin() == countMap.end() )
            return 0;

        iterators.clear();
        MyMap::const_iterator it = myMap.begin();
        iterators.push_back( it );

        // Split the map into ceil(size / FRAGMENT_SIZE) segments
        int numFragments = 0;
        while( it != countMap.end() ) {
            // Increment it by either FRAGMENT_SIZE or however many elements we have
            // left in the map, whichever is less.
            for( uint64_t i = 0; i < FRAGMENT_SIZE && it != countMap.end(); ++i ) {
                ++it;
            }
            // Push a copy of the iterator into the vector
            iterators.push_back(it);
            ++numFragments;
        }

        return numFragments;
    }
    //! [get-num-fragments]

    /*!
     *  The iterator type returned by the Finalize method.
     */
    //! [iterator]
    class Iterator {
        typedef MyMap::const_iterator iter_type;
        iter_type begin;
        iter_type end;

    public:

        Iterator( iter_type _begin, iter_type _end ) :
            begin(_begin), end(_end) { }

        bool GetNextResult( INT& x, INT& count ) {
            // Make sure we have tuples to produce
            if( begin == end )
                return false;

            x = begin->first;
            count = begin->second;

            ++begin;
            return true;
        }
    };
    //! [iterator]

    /*!
     * Prepares to generate the given fragment by creating an iterator for it.
     *
     * @param[in]   fragNum     The number of the fragment to produce.
     *
     * @return the iterator for that fragment.
     */
    //! [finalize]
    Iterator* Finalize( int fragNum ) {
        // iterators[fragNum] is the beginning of the fragment and
        // iterators[fragNum+1] is the end of the fragment.
        return new Iterator(iterators[fragNum], iterators[fragNum+1]);
    }
    //! [finalize]

    /*!
     *  Produces a single output tuple if possible.
     *
     *  @param[in]  iter    The iterator for the fragment we are producing
     *                      values for.
     *  @param[out] x       The element whose count is being produced.
     *  @param[out] count   The number of times the element was seen.
     *
     *  @return true if the output tuple is valid, false if there was no more
     *          output to be produced.
     */
    //! [get-next-result]
    bool GetNextResult( Iterator* iter, INT& x, INT& count ) {
        return iter->GetNextResult(x, count);
    }
    //! [get-next-result]
};

//! [iterator-typedef]
// Typedef for the iterator type so it will be compatible with the system.
typedef CountDistinctFrag::Iterator CountDistinctFrag_Iterator;
//! [iterator-typedef]

#endif // _COUNT_DISTINCT_FRAG_H_
