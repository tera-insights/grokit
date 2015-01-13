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
#ifndef _SWAP_H_
#define _SWAP_H_

#include "Constants.h"
#include "Config.h"
#include <cstring>
#include "Pragma.h"

/**
  This header contains macro definitions to streamline the swapping of data.
  The main macro defined is SWAP(a,b) that swaps the content of a and b as
  long as they are of the same type and they support the = operator.

  Notice that the type of the a and b is not passed and it is infered using typeof()
  operator.
*/

// Macro to swap variables using assignment and a temporary
#ifdef _HAS_CPP_11
#define SWAP_ASSIGN(a,b) { \
    decltype(a) tmp = ( a ); \
    ( a ) = ( b ); \
    ( b ) = tmp; \
}
#else
#define SWAP_ASSIGN(a,b) { \
    typeof(a) tmp = ( a ); \
    ( a ) = ( b ); \
    ( b ) = tmp; \
}
#endif

// Macro to swap variables using swap()
#ifdef _HAS_CPP_11
#include <utility>
#define SWAP_STD(a, b) { \
    using std::swap; \
    swap( a, b ); \
}
#else
#include <algorithm>
#define SWAP_STD(a, b) { \
    using std::swap; \
    swap( a, b ); \
}
#endif
/*
#define SWAP_STD(a, b) SWAP_ASSIGN(a, b)
*/

// Old swap macro
#define SWAP(a, b) \
    PRAGMA_MSG("Warning: Using old swap macro") \
    SWAP_ASSIGN( a , b )

/* Macro to swap classes using memmove.

   WARNING: this macro introduces a horrible bug if the class contains
   any STL datastructres. Use explicit SWAP.
*/

#define SWAP_memmove(class_name, object)		\
  char storage[sizeof (class_name)];			\
  memmove ((void *) storage, (void *) this, sizeof (class_name));		\
  memmove ((void *) this, (void *) &object, sizeof (class_name));		\
  memmove ((void *) &object, (void *) storage, sizeof (class_name));


#endif //_SWAP_H_
