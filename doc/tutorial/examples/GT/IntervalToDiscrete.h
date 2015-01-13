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

#ifndef _INTERVAL_TO_DISCRETE_H_
#define _INTERVAL_TO_DISCRETE_H_

// DataPath base library includes
#include "base/Types/INT.h"

//! [description]
/* System description
 *  GT_DESC
 *      NAME(</IntervalToDiscrete/>)
 *      INPUTS(</(start, INT), (end, INT)/>)
 *      OUTPUTS(</(value, INT)/>)
 *      RESULT_TYPE(</multi/>)
 *      CONSTRUCTOR(</(inclusive, bool)/>)
 *  END_DESC
 */
//! [description]

/*!
 * @brief   A transform that takes an interval and converts it to a set of
 *          all of the discrete values within that interval.
 */
class IntervalToDiscrete {

    // Whether or not ranges are inclusvie
    bool inclusive;

    // Current place within the range and the end of the range
    INT curVal;
    INT curEnd;

public:

    //! [constructor]
    IntervalToDiscrete( bool _inclusive ) : inclusive( _inclusive )
    { }
    //! [constructor]

    //! [process-tuple]
    void ProcessTuple( const INT& begin, const INT& end ) {
        // If begin > end, switch them so we are always producing an
        // increasing sequence. This just makes the code to produce the results
        // cleaner and easier to write.
        if( begin > end )
        {
            curVal = end;
            curEnd = begin;
        }
        else {
            curVal = begin;
            curEnd = end;
        }
    }
    //! [process-tuple]

    //! [get-next-result]
    bool GetNextResult( INT& value ) {
        // If ranges are inclusive, only return false if the current value is
        // strictly greater than the end of the range. Otherwise, return false
        // if greater than or equal to the end of the range.
        // We made sure that curVal <= curEnd for valid output in ProcessTuple.
        if( inclusive ) {
            if( curVal > curEnd )
                return false;
        }
        else {
            if( curVal >= curEnd )
                return false;
        }

        // Assign the current value to the output and then increment it
        value = curVal++;

        return true;
    }
    //! [get-next-result]
};

#endif // _INTERVAL_TO_DISCRETE_H_
