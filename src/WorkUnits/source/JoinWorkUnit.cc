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
#include "JoinWorkUnit.h"
#include "JoinWorkUnitImp.h"


void JoinWorkUnit :: GetInput (Chunk &putItHere) {
	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.GetInput (putItHere);
}

void JoinWorkUnit :: GetOutput (Chunk &putItHere) {
	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.GetOutput (putItHere);
}

JoinWorkUnit :: JoinWorkUnit () {
	delete data;
	data = new JoinWorkUnitImp;
}

JoinWorkUnit :: ~JoinWorkUnit () {
	delete data;
	data = NULL;
}

void JoinWorkUnit :: LoadMerge (HashTable &tableOne, HashTable &tableTwo,
	HT_INDEX_TYPE low, HT_INDEX_TYPE high) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadMerge (tableOne, tableTwo, low, high);
}

void JoinWorkUnit :: LoadFinalMergeAndAllocate (HashTable &mergeIntoMe) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadFinalMergeAndAllocate (mergeIntoMe);
}

void JoinWorkUnit :: LoadFinalMerge (HashTable &mergeIntoMe) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadFinalMerge (mergeIntoMe);
}

void JoinWorkUnit :: LoadAllocate (HashTable &allocateMe) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadAllocate (allocateMe);
}

void JoinWorkUnit :: LoadHashProbe (int (*func) (Chunk &, HashTable &,
	HashTable &, int, Chunk &), Chunk &processMe, HashTable &tableOne, int myJoinID) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadHashProbe (func, processMe, tableOne, myJoinID);
}

void JoinWorkUnit :: LoadHashProbe (int (*func) (Chunk &, HashTable &,
	HashTable &, int, Chunk &), Chunk &processMe, HashTable &tableOne,
	HashTable &tableTwo) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadHashProbe (func, processMe, tableOne, tableTwo);
}

void JoinWorkUnit :: LoadHashBuild (int (*func) (Chunk &, HashTable &,
	HashTable &, HT_INDEX_TYPE, HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
	Chunk &processMe, HashTable &tableOne, ChunkHeaderList &chunks, int myJoinID) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadHashBuild (func, processMe, tableOne, chunks, myJoinID);
}

void JoinWorkUnit :: LoadHashBuild (int (*func) (Chunk &, HashTable &,
	HashTable &, HT_INDEX_TYPE, HT_INDEX_TYPE, HashesToRunList&, ChunkHeaderList&),
	Chunk &processMe, HashTable &tableOne, HashTable &tableTwo,
	HT_INDEX_TYPE lowToMerge, HT_INDEX_TYPE highToMerge, HashesToRunList &hashes,
	ChunkHeaderList &chunks) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.LoadHashBuild (func, processMe, tableOne, tableTwo, lowToMerge,
		highToMerge, hashes, chunks);
}

int JoinWorkUnit :: AnyExtras () {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	return tmp.AnyExtras ();
}

int JoinWorkUnit :: WasMerging () {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	return tmp.WasMerging ();
}

int JoinWorkUnit :: GetType () {
	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	return tmp.GetType ();
}


void JoinWorkUnit :: AdjustReadWriteCounts () {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
}

HT_INDEX_TYPE JoinWorkUnit :: GetSlotsStillOutThere () {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	return tmp.GetSlotsStillOutThere ();
}

void JoinWorkUnit :: ModifyStillOutThere (int delta) {

	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	tmp.ModifyStillOutThere (delta);
}
