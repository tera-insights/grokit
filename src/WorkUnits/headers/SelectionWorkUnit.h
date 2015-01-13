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
// for Emacs -*- c++ -*-

#ifndef _SELECTWORKUNIT_H_
#define _SELECTWORKUNIT_H_

#include "WorkUnit.h"
#include "Chunk.h"

/**
 * This is a WorkUnit specialized for the selection operation
*/

class SelectionWorkUnit : public WorkUnit {

public:

	// generic constructor and destructor
	SelectionWorkUnit ();
	virtual ~SelectionWorkUnit ();

	// loads up a probe request for one table into the WorkUnit
	void LoadScanJob (int (*ScanChunk) (Chunk &), Chunk &processMe);

	// extract the output chunk... returns -1 if there is nothing to extract
	int GetOutput (Chunk &output);

};

#endif
