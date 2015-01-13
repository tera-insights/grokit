//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
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
#ifndef _AVERAGE_GLA_H_
#define _AVERAGE_GLA_H_

//! [average-desc]
/*
 *  GLA_DESC
 *      NAME(</Average/>)
 *      INPUTS(</(x, DOUBLE)/>)
 *      OUTPUTS(</(_count, BIGINT), (_sum, DOUBLE), (avg, DOUBLE)/>)
 *      RESULT_TYPE(</single/>)
 *  END_DESC
 */
//! [average-desc]

class Average {
    long long int count; // keeps the number of tuples aggregated
    long double sum; // sum of the values
public:
    //! [average-constructor]
    Average() : count(0), sum(0)
    { }
    //! [average-constructor]

    //! [average-additem]
    void AddItem(const DOUBLE& x){
        count++;
        sum += x;
    }
    //! [average-additem]

    //! [average-add-state]
    void AddState(Average& o){
        count += o.count;
        sum+ = o.sum;
    }
    //! [average-add-state]

    // we only support one tuple as output
    //! [average-getresult]
    void GetResult(BIGINT& _count, DOUBLE& _sum, DOUBLE& avg){
        _count = count;
        _sum = sum;
        avg = (count>0) ? (sum/count) : 0.0;
    }
    //! [average-getresult]
};

// Synonym for compatibility reasons.
// SYN_DEF(</AverageGLA/>, </Average/>)

#endif // _AVERAGE_GLA_H_
