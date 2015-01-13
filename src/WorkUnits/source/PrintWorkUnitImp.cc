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

#include "PrintWorkUnitImp.h"

PrintWorkUnitImp :: PrintWorkUnitImp () {
	data.numTuplesInChunk = -1;
	data.processChunk = NULL;
}

PrintWorkUnitImp :: ~PrintWorkUnitImp () {
}

void PrintWorkUnitImp :: LoadPrintJob (int (*PrintChunk) (Chunk &), Chunk &processMe) {
	processMe.swap (data.chunkToProcess);
	data.processChunk = PrintChunk;
}

int PrintWorkUnitImp :: GetOutput (Chunk &output) {
	if (data.numTuplesInChunk == -1)  {
		cerr << "You are trying to unload a workunit that has not actually been run.\n";
		exit (1);
	}
	output.swap (data.chunkToProcess);
	return data.numTuplesInChunk;
}

void PrintWorkUnitImp :: Run () {
	description = "Print";

	data.numTuplesInChunk = (*data.processChunk) (data.chunkToProcess);
}
