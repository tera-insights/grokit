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
#ifndef _COUNT_DISTINCT_H_
#define _COUNT_DISTINCT_H_

// STL includes
#include <map>

// DataPath library includes
#include "base/Types/INT.h"

//! [count-distinct-desc]
/*  System Description of GLA
 *  GLA_DESC
 *      NAME(</CountDistinct/>)
 *      INPUTS(</(x, INT)/>)
 *      OUTPUTS(</(x, INT), (count, INT)/>)
 *      RESULT_TYPE(</multi/>)
 *  END_DESC
 */
//! [count-distinct-desc]

/*!
 *  @brief  A GLA to track distinct elements in a set of numbers, and how many
 *          times each distinct element appeared.
 */

class CountDistinct {

    // A map type for mapping numbers to counts.
    typedef std::map<INT, INT> MyMap;

    // Map containing mapping of distinct elements to their count.
    MyMap countMap;

    // Internal iterator for keeping track of our current output.
    MyMap::const_iterator curOutput;

public:

    /*!
     *  Default constructor. It doesn't have to do anything special, as default
     *  initialization of the members is fine.
     */
    CountDistinct() { }

    /*!
     *  Default destructor. It doesn't have to do anything special, as default
     *  destruction of its members is fine.
     */
    ~CountDistinct() { }

    /*!
     *  Takes a single element of the dataset, and increments the counter for
     *  it.
     *
     *  @param[in]  x   The element from the dataset.
     */
    //! [count-distinct-add-item]
    void AddItem( const INT& x ) {
        // If no count for x exists, the map will create a count for it using
        // the default constructor for INT, will will give it a value of 0.
        countMap[x] += 1;
    }
    //! [count-distinct-add-item]

    /*!
     *  Adds the counts from other into this state.
     *
     *  @param[in]  other   The state to add to this one.
     */
    //! [count-distinct-add-state]
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
    //! [count-distinct-add-state]

    /*!
     *  Prepares the state for output production by setting the internal
     *  iterator.
     */
    //! [count-distinct-finalize]
    void Finalize( void ) {
        curOutput = countMap.begin();
    }
    //! [count-distinct-finalize]

    /*!
     *  Produces a single output tuple if possible.
     *
     *  @param[out] x       The element whose count is being produced.
     *  @param[out] count   The number of times the element was seen.
     *
     *  @return true if the output tuple is valid, false if there was no more
     *          output to be produced.
     */
    //! [count-distinct-get-next-result]
    bool GetNextResult( INT& x, INT& count ) {
        if( curOutput != countMap.end() ) {
            // We have more output to produce.
            x = curOutput->first;
            count = curOutput->second;

            return true;
        }
        else {
            // There is no more output.
            return false;
        }
    }
    //! [count-distinct-get-next-result]
};

#endif // _COUNT_DISTINCT_H_
