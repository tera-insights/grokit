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

#ifndef _PATTERN_MATCHER_H_
#define _PATTERN_MATCHER_H_

// Standard library includes
#include <string>
#include <regex>

// DataPath base library includes
#include "base/Types/STRING_LITERAL.h"
#include "base/Types/HString.h"

//! [description]
/* System description
 *  GF_DESC
 *      NAME(</PatternMatcher/>)
 *      INPUTS(</(str, HString)/>)
 *      CONSTRUCTOR(</(regexp, STRING_LITERAL)/>)
 *  END_DESC
 */
//! [description]

class PatternMatcher {

    const std::regex reg;

public:

    //! [constructor]
    PatternMatcher( STRING_LITERAL regexp ) : reg(regexp) {
    }
    //! [constructor]

    ~PatternMatcher(void) {}

    //! [filter]
    bool Filter( const HString & str ) const {
        const char * target = str.GetStr();

        std::match_results result;
        return std::regex_match( target, result, reg );
    }
    //! [filter]

};

#endif // _PATTERN_MATCHER_H_
