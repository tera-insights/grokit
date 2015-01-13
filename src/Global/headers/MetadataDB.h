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
#ifndef _METADATADB_H_
#define _METADATADB_H_

#include "Constants.h"

/** header file specifying information about the global database in
		which metadata is stored. We want to ensure everybody uses the
		same file. The value is set in the main program before starting
		anything.

		The definitions are in MetadataDB.cc
*/

// access functions to set and get metadata
const char* GetMetadataDB(void);
void SetMetadataDB(const char*);

#endif // _METADATADB_H_
