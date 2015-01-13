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

#ifndef _F_TO_C_H_
#define _F_TO_C_H_

// DataPath base library includes
#include "base/Types/DOUBLE.h"

//! [description]
/* System description
 *  GT_DESC
 *      NAME(</FtoC/>)
 *      INPUTS(</(tempF, DOUBLE)/>)
 *      OUTPUTS(</tempC, DOUBLE)/>)
 *      RESULT_TYPE(</single/>)
 *  END_DESC
 */
//! [description]

/*!
 *  @brief  A simple transform to convert from the fahrenheit temperature scale
 *          to celsius.
 */
class FtoC {

public:

    //! [constructor]
    FtoC() { }
    //! [constructor]

    //! [process-tuple]
    void ProcessTuple( const DOUBLE& tempF, DOUBLE& tempC ) {
        // tempF is the input
        // tempC is the output
        tempC = (tempF - 32) / 1.8;
    }
    //! [process-tuple]
};

#endif // _F_TO_C_H_
