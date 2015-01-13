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

#include "SelectionWorkUnitImp.h"

SelectionWorkUnitImp :: SelectionWorkUnitImp () {
	data.numTuplesInChunk = -1;
	data.processChunk = NULL;
}

SelectionWorkUnitImp :: ~SelectionWorkUnitImp () {
}

void SelectionWorkUnitImp :: LoadScanJob (int (*ScanChunk) (Chunk &), Chunk &processMe) {
	processMe.swap (data.chunkToProcess);
	data.processChunk = ScanChunk;
}

int SelectionWorkUnitImp :: GetOutput (Chunk &output) {
	if (data.numTuplesInChunk == -1)  {
		cerr << "You are trying to unload a workunit that has not actually been run.\n";
		exit (1);
	}
	output.swap (data.chunkToProcess);
	return data.numTuplesInChunk;
}

void SelectionWorkUnitImp :: Run () {
	description = "Selection";

	data.numTuplesInChunk = (*data.processChunk) (data.chunkToProcess);
}
