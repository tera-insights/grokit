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
#ifndef _COUNT_GLA_H_
#define _COUNT_GLA_H_

/** Info for the meta-GLAs
 * GLA_DESC
 *
 * NAME(</Count/>)
 * INPUTS(</(dummy, INT)/>)
 * OUTPUTS(</(_count, BIGINT)/>)
 * RESULT_TYPE(</single/>)
 *
 * END_DESC
 */

class Count {
  long long int count; // keeps the number of tuples aggregated
public:
  Count(){ count=0; }
  void AddItem(const int& dummy){ count++; }
  void AddState(Count& o){ count+=o.count; }

  // we only support one tuple as output
  void GetResult(BIGINT& _count){
      _count=count;
  }
};

// Synonym for compatibility purposes
// SYN_DEF(</CountGLA/>, </Count/>)

#endif // _COUNT_GLA_H_
