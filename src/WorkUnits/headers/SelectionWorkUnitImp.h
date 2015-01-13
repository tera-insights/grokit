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

#ifndef _SELECTWORKUNIT_IMP_H_
#define _SELECTWORKUNIT_IMP_H_

#include "WorkUnitImp.h"
#include "SelectionWorkUnit.h"
#include "Chunk.h"

class SelectionWorkUnitImp : public WorkUnitImp {

protected:

	struct Info {

		// this is the function that actually does the work
		int (*processChunk) (Chunk &);

		// this is the chunk that will actually be prcessed by the function
		Chunk chunkToProcess;

		// this is the number of tuples in the resulting chunk
		int numTuplesInChunk;
	};

	Info data;

public:

	// generic constructor and destructor
	SelectionWorkUnitImp ();
	virtual ~SelectionWorkUnitImp ();

	// these mirror the functions for SelectionWorkUnit
	void LoadScanJob (int (*ScanChunk) (Chunk &), Chunk &processMe);
	int GetOutput (Chunk &output);

	// do the work
	void Run ();
};

#endif
