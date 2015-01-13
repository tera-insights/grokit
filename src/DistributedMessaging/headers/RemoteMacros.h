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
#ifndef _MACROS_
#define _MACROS_

////////////////////////////////////////////////
// MACRO DEFINITIONS

//in-place string construction from char*
#define IN_PLACE_STRING_CONSTRUCTOR(S, S_LEN, CHAR_BUFFER) \
	S.resize(S_LEN); \
	char* CHAR_BUFFER = const_cast<char*>(S.data());


#endif
