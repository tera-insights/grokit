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
#ifndef _SUM_GLA_H_
#define _SUM_GLA_H_

/** Info for meta-GLAs
 * GLA_DESC
 *
 * NAME(</Sum/>)
 * INPUTS(</(x, DOUBLE)/>)
 * OUTPUTS(</(_sum, DOUBLE)/>)
 * RESULT_TYPE(</single/>)
 *
 * END_DESC
*/

class Sum {
  long double sum; // sum of the values
public:
  Sum(){ sum=0.0; }

  void AddItem(const DOUBLE& x){ sum+=x; }

  void AddState(Sum& o){ sum+=o.sum; }

  // we only support one tuple as output
  void GetResult(DOUBLE& _sum){
      _sum=sum;
  }
};

// Synonym for compatibility purposes
// SYN_DEF(</SumGLA/>, </Sum/>)

#endif // _SUM_GLA_H_
