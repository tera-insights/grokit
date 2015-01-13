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

#ifndef AGG_STOR
#define AGG_STOR

#include "Errors.h"
#include "Swap.h"
#include <stdlib.h>


/**
   Class is a container for the state of aggregates. The object does
   not now what it holds inside so it cannot manipulate it.

**/
// this is the generic aggregate machine
class AggStorage {
private:

    void* content; // Item inside

public:
    AggStorage ():content(NULL){};
    AggStorage (void* _content):content(_content){}
    ~AggStorage (){ FATALIF(content!=NULL, "you are throwing away aggregates. This must be wrong"); };

    // get access to content
    void* GetContent(void){ FATALIF(content == NULL, "Null aggregate accesed"); return content; }

    // free space and reset
    void Reset(){ content = NULL; }

    // swap two agg storage structs
    void swap (AggStorage &withMe){ SWAP_STD(content,withMe.content); }
};

// Override global swap
inline
void swap( AggStorage & a, AggStorage & b ) {
    a.swap(b);
}

#endif
