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
#include "TileManager.h"
#include<assert.h>
#include<iostream>
#include "HashTableMacros.h"

using namespace std;

ChunkMetaData :: ChunkMetaData () {
	isSlicedAlready = false;
	numFragments = 0;
}

ChunkMetaData :: ~ChunkMetaData () {
}

// Helper function
void ChunkMetaData :: Insert (int bucket, ChunkID chunkID, BucketToChunkMap& ChunksToDisk) {

	BucketToChunkMap::iterator it = ChunksToDisk.find (bucket);
	if (it == ChunksToDisk.end()) {
		// If bucket not found, create a set and insert it
		set<ChunkID> chk;
		chk.insert(chunkID);
		ChunksToDisk[bucket] = chk;
		// double check if its inserted or not, remove later
		assert (ChunksToDisk[bucket].find(chunkID) != ChunksToDisk[bucket].end());
	} else {
		assert ((it->second).find(chunkID) == (it->second).end()); // keep this check
		(it->second).insert(chunkID);
		assert ((it->second).find(chunkID) != (it->second).end()); // remove this later
	}
}

// Insert chunks and keep separate lists for left and right chunks in bucket -> chunks mapping
void ChunkMetaData :: ChunkToDisk (int bucket, ChunkID chunkID, bool isLeft) {

	if (isLeft == true)
		Insert (bucket, chunkID, leftChunksToDisk);
	else
		Insert (bucket, chunkID, rightChunksToDisk);
}

// Get the tile from floor and also give back bucket number
bool ChunkMetaData :: GetTile (__uint64_t& bucket, vector<ChunkID>& chunkIDLHS, vector<ChunkID>& ChunkIDRHS) {

	// Get the desired pseudo bucket
	BucketToFloorMap::iterator it = myBucketToFloorMap.begin();
	// If empty bucket, means we already extracted all tiles
	if (it == myBucketToFloorMap.end())
		return false;
	// Just return very first tile
	bucket = it->first;
	chunkIDLHS = ((it->second).begin())->first;
	ChunkIDRHS = ((it->second).begin())->second;
	(it->second).erase((it->second).begin()); // remove first element of inner map
	if ((it->second).empty()) // If inner map is empty, erase the bucket
		myBucketToFloorMap.erase(it);
}

bool ChunkMetaData :: GetFloor (__uint64_t& bucket, vector<ChunkID>& lhs, vector<ChunkID>& rhs) {

	// get the first floor
	//BucketToChunkMapSmall::iterator it = leftChunksToDiskSmaller.begin();
	BucketToChunkMap::iterator it = leftChunksToDisk.begin();
	// If empty, means we already extracted all floors
	//if (it == leftChunksToDiskSmaller.end())
	if (it == leftChunksToDisk.end())
		return false;
	// Just return very first floor
	bucket = it->first;
	for (set<ChunkID>::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
		lhs.push_back(*iter);
	}

	//BucketToChunkMapSmall::iterator itr = rightChunksToDiskSmaller.begin();
	BucketToChunkMap::iterator itr = rightChunksToDisk.begin();
	//assert(itr != rightChunksToDiskSmaller.end());
	assert(itr != rightChunksToDisk.end());
	for (set<ChunkID>::iterator iter = (itr->second).begin(); iter != (itr->second).end(); iter++) {
		rhs.push_back(*iter);
	}

	// now erase the first floor
	//leftChunksToDiskSmaller.erase(it);
	//rightChunksToDiskSmaller.erase(itr);
	leftChunksToDisk.erase(it);
	rightChunksToDisk.erase(itr);

	return true;
}

bool ChunkMetaData :: GetSlice (__uint64_t& bucket, vector<ChunkID>& lhs, vector<ChunkID>& rhs) {

	lhs.clear();
	rhs.clear();
	// Before getting the first slice, first slice all the floors into several slices
	if (isSlicedAlready == false) {
		SubDivide();
		isSlicedAlready = true;
	}
	// get the first floor
	BucketToChunkMapSmall::iterator it = leftChunksToDiskSmaller.begin();
	// If empty, means we already extracted all floors
	if (it == leftChunksToDiskSmaller.end())
		return false;
	// Just return very first floor
	bucket = it->first;
	for (set<ChunkID>::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
		lhs.push_back(*iter);
/*
			IDInfo li;
			(const_cast<ChunkID&>(*iter)).getInfo(li);
			//cout << " " << (li.getIDAsString()).c_str();
			cout << "\n****l " << (li.getName()).c_str() << " l******"; fflush(stdout);
*/
	}

	BucketToChunkMapSmall::iterator itr = rightChunksToDiskSmaller.begin();
	assert(itr != rightChunksToDiskSmaller.end());
	for (set<ChunkID>::iterator iter = (itr->second).begin(); iter != (itr->second).end(); iter++) {
		rhs.push_back(*iter);
/*
			IDInfo li;
			(const_cast<ChunkID&>(*iter)).getInfo(li);
			//cout << " " << (li.getIDAsString()).c_str();
			cout << "\n****r " << (li.getName()).c_str() << " r*****"; fflush(stdout);
*/
	}

	// now erase the first floor
	leftChunksToDiskSmaller.erase(it);
	rightChunksToDiskSmaller.erase(itr);

	return true;
}

multimap<vector<ChunkID>, vector<ChunkID> > ChunkMetaData :: GetAllTiles (__uint64_t bucket) {

	// Get the desired pseudo bucket
	BucketToFloorMap::iterator it = myBucketToFloorMap.find(bucket);
	// If empty bucket, means we already extracted all tiles
	assert (it != myBucketToFloorMap.end());
	return it->second;
}
/*
// This must be called after all insertions, this is where floor is created consisting of tiles (pair of chunks)
void ChunkMetaData :: DoneWriting () {

	// Left and right list of chunks must be equal (as far as current assumption, otherwise remove this check)
	assert (leftChunksToDisk.size() == rightChunksToDisk.size());

	// Now start populating mySubBucketToFloorMap
	for (BucketToChunkMap::iterator itmap1 = leftChunksToDisk.begin(); itmap1 != leftChunksToDisk.end(); ++itmap1) {
		// find the same bucket in rhs list
		BucketToChunkMap::iterator itmap2 = rightChunksToDisk.find(itmap1->first);
		// It must be present, no matter what
		assert (itmap2 != rightChunksToDisk.end());
		// Now create cross product, means all combination of pairs of same bucket from 2 lists
		for (set<ChunkID>::iterator itset1 = (itmap1->second).begin(); itset1 != (itmap1->second).end(); ++itset1) {
			for (set<ChunkID>::iterator itset2 = (itmap2->second).begin(); itset2 != (itmap2->second).end(); ++itset2) {
				
				BucketToFloorMap::iterator itout = myBucketToFloorMap.find(itmap1->first);
				if (itout == myBucketToFloorMap.end()) { // insert new bucket
					multimap<ChunkID, ChunkID> outmap;
					outmap.insert(pair<ChunkID, ChunkID>(*(itset1), *(itset2))); // multimap dont have []
					myBucketToFloorMap[itmap1->first] = outmap;
				} else {
					(itout->second).insert(pair<ChunkID, ChunkID>(*(itset1), *(itset2))); // add new pair in existing bucket
				}
			}
		}
	}
}
*/

// We subdivide each chunk so that same range of hash values can fit into memory at once. Basically we subdivide
// each chunk into max number of fragments but while using it, we may club together several fragments
// so that we don't have too small memory floor
void ChunkMetaData :: SubDivide (BucketToChunkMap& ChunksToDisk, BucketToChunkMapSmall& ChunksToDiskSmaller) {

	// Make sure this pseudo bucket for same bucket are together located in map, so that we get all slices
	// of same floor first
	__uint64_t pseudoBucketNumber = 0;
	// iterate over all the buckets, each bucket contain multiple chunks which need to be parted into many
	for (BucketToChunkMap::iterator itmap1 = ChunksToDisk.begin(); itmap1 != ChunksToDisk.end(); ++itmap1) {
		// iterate over each chunk and part each of them into many
		for (set<ChunkID>::iterator itset1 = (itmap1->second).begin(); itset1 != (itmap1->second).end(); ++itset1) {
			// Same pseudo bucket number for same range in different same bucket number chunks
			pseudoBucketNumber = (((__uint64_t)itmap1->first) << 32); // MSB contain bucket number
			ChunkID chunk = (*itset1);
			for (int i = 0; i < numFragments;) {
				// Below chunkID has [start, end] fragment range
				ChunkID newchunk((size_t)chunk.GetID(), chunk.GetTableScanId(), i, i + NUM_FRAGMENTS_PER_SLICE-1);
				i = i + NUM_FRAGMENTS_PER_SLICE;
				BucketToChunkMapSmall::iterator it = ChunksToDiskSmaller.find(pseudoBucketNumber); // search bucket
				if (ChunksToDiskSmaller.end() == it) {
					set<ChunkID> c;
					c.insert(newchunk);
					ChunksToDiskSmaller[pseudoBucketNumber] = c;
				} else {
					(it->second).insert(newchunk);
				}
				pseudoBucketNumber++; // LSB contain smaller pseudo bucket number
			}
		}
	}
}

void ChunkMetaData :: SubDivide () {

	SubDivide (leftChunksToDisk, leftChunksToDiskSmaller);
	// Check sanity
	int numdivisions = numFragments / NUM_FRAGMENTS_PER_SLICE;
	if (numFragments%NUM_FRAGMENTS_PER_SLICE!= 0)
		numdivisions++;
	assert (leftChunksToDiskSmaller.size() == numdivisions*leftChunksToDisk.size());

	SubDivide (rightChunksToDisk, rightChunksToDiskSmaller);
	// Check sanity
	assert (rightChunksToDiskSmaller.size() == numdivisions*rightChunksToDisk.size());
}

void ChunkMetaData :: PrintFloor (map<ChunkID, set<ChunkID> >& floor) {
	cout << "\nFloor starts here: size = " << floor.size();
	for (map<ChunkID, set<ChunkID> >::iterator it = floor.begin(); it != floor.end(); it++) {
		// Print one LHS
		IDInfo l;
		(const_cast<ChunkID&>(it->first)).getInfo(l);
		//cout << "\nLHS: " << (l.getIDAsString()).c_str();
		cout << "\nLHS: " << (l.getName()).c_str();
	
		// Print all RHS
		set<ChunkID>& list = it->second;
		cout << " RHS : ";
		for (set<ChunkID>::iterator iter = list.begin(); iter != list.end(); iter++) {
			IDInfo li;
			(const_cast<ChunkID&>(*iter)).getInfo(li);
			//cout << " " << (li.getIDAsString()).c_str();
			cout << " " << (li.getName()).c_str();
		}
		cout << endl;
	}
}

/*
	This will create the tiles in advance for every floor even before any chunk is read. Because we assume that
	all the chunks who belong to same range (subbucket) will be read all at once from the disk. And we already
	have created sub floors to send back to join merge. This will create sub floors for all at once because we
	already know what all we are gonna read in future. Because reading is always one subbucket for all chunks
	at a time.
*/
void ChunkMetaData :: DoneWriting () {

	SubDivide ();

	for (BucketToChunkMapSmall::iterator itmap1 = leftChunksToDiskSmaller.begin(); itmap1 != leftChunksToDiskSmaller.end(); ++itmap1) {
		// find the same bucket in rhs list
		BucketToChunkMapSmall::iterator itmap2 = rightChunksToDiskSmaller.find(itmap1->first);
		// It must be present, no matter what
		assert (itmap2 != rightChunksToDiskSmaller.end());
		// First create adjacency list so that we can extract small rectangles out of it
		map<ChunkID, set<ChunkID> > Floor;
		for (set<ChunkID>::iterator itset1 = (itmap1->second).begin(); itset1 != (itmap1->second).end(); ++itset1)
			Floor[(*itset1)] = itmap2->second; // copy complete RHS set as adjacency list of single LHS member

		//PrintFloor (Floor);

		int rightListSize = (itmap2->second).size();
		while (!Floor.empty()) {
			vector<ChunkID> RHSList;
			vector<ChunkID> LHSList;
			bool gotTile = false;
			bool firstTime = true;
			int i = 0;
			// Iterate the floor and slice out small rectangles from it
			for (map<ChunkID, set<ChunkID> >::iterator floorit = Floor.begin(); floorit != Floor.end() && i < SUBFLOOR_SIZE; i++) {
				int j = 0;
				gotTile = false;
				for (set<ChunkID>::iterator adjit = (floorit->second).begin(); adjit != (floorit->second).end() && j < SUBFLOOR_SIZE; ++j) {
					gotTile = true;
					if (firstTime)
						RHSList.push_back (*adjit);
					else
						assert (RHSList[j] == *adjit);
					set<ChunkID>::iterator adjit_temp = adjit++;
					(floorit->second).erase(adjit_temp); // remove adjacency list element
				}
				firstTime = false;
				if (gotTile) {
					LHSList.push_back(floorit->first);
				} else {
					i--;
				}

				if ((floorit->second).empty()) {
					map<ChunkID, set<ChunkID> >::iterator it_temp = floorit++;
					Floor.erase (it_temp);
				} else {
					++floorit;
				}
			}
			if (gotTile) {
				BucketToFloorMap::iterator bkit = myBucketToFloorMap.find (itmap1->first);
				if (bkit == myBucketToFloorMap.end()) {
					multimap<vector<ChunkID>, vector<ChunkID> > m;
					m.insert(pair<vector<ChunkID>, vector<ChunkID> >(LHSList, RHSList)); // multimap dont have []
					myBucketToFloorMap[itmap1->first] = m;
				} else {
					(bkit->second).insert(pair<vector<ChunkID>, vector<ChunkID> >(LHSList, RHSList));
				}
			}
		}
	}
}

void ChunkMetaData :: UnprocessedFloor(__uint64_t bucketID, set<ChunkID> chunkLHS, set<ChunkID> chunkRHS) {
	leftChunksToDisk[bucketID] = chunkLHS;
	rightChunksToDisk[bucketID] = chunkRHS;
}

void ChunkMetaData :: UnprocessedSlice(__uint64_t bucketID, vector<ChunkID> chunkLHS, vector<ChunkID> chunkRHS) {
	set<ChunkID> lhs;
	for (int i = 0; i < chunkLHS.size(); i++)
		lhs.insert(chunkLHS[i]);
	set<ChunkID> rhs;
	for (int i = 0; i < chunkRHS.size(); i++)
		rhs.insert(chunkRHS[i]);
	leftChunksToDiskSmaller[bucketID] = lhs;
	rightChunksToDiskSmaller[bucketID] = rhs;
}

// We append in our list the pair of chunks if they are sent back and then we give it back again
void ChunkMetaData :: UnprocessedTile(__uint64_t bucketID, vector<ChunkID> chunkLHS, vector<ChunkID> chunkRHS) {

	BucketToFloorMap::iterator it = myBucketToFloorMap.find(bucketID);
	if (it == myBucketToFloorMap.end()) {
		// If bucket is not found, that means we have already sent all small floors for processing. Hence
		// add new bucket entry
		multimap<vector<ChunkID>, vector<ChunkID> > outmap;
		outmap.insert(pair<vector<ChunkID>, vector<ChunkID> >(chunkLHS, chunkRHS));
		myBucketToFloorMap[bucketID] = outmap;
	} else {
		// If found, means we have more chunks left in the same bucket for processing. Add this one too.
		(it->second).insert(pair<vector<ChunkID>, vector<ChunkID> >(chunkLHS, chunkRHS));
	}
}

// As we keep on removing members when GetTile() so this check is good
bool ChunkMetaData :: IsEmpty () {
	return myBucketToFloorMap.empty();
}

// for debug
void ChunkMetaData :: PrintAllSlices() {

	for (BucketToChunkMapSmall::iterator it = leftChunksToDiskSmaller.begin(); it != leftChunksToDiskSmaller.end(); it++) {
		for (set<ChunkID>::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
			cout << "\nLeft Bucket : " << it->first;
			IDInfo li;
			(const_cast<ChunkID&>(*iter)).getInfo(li);
			//cout << " " << (li.getIDAsString()).c_str();
			cout << " " << (li.getName()).c_str();
		}
	}
	for (BucketToChunkMapSmall::iterator it = rightChunksToDiskSmaller.begin(); it != rightChunksToDiskSmaller.end(); it++) {
		for (set<ChunkID>::iterator iter = (it->second).begin(); iter != (it->second).end(); iter++) {
			cout << "\nRight Bucket : " << it->first;
			IDInfo li;
			(const_cast<ChunkID&>(*iter)).getInfo(li);
			//cout << " " << (li.getIDAsString()).c_str();
			cout << " " << (li.getName()).c_str();
		}
	}
}

void ChunkMetaData :: PrintAllTiles() {

	for (BucketToFloorMap::iterator itmap = myBucketToFloorMap.begin(); itmap != myBucketToFloorMap.end(); ++itmap) {
		for (multimap<vector<ChunkID>, vector<ChunkID> >::iterator itmultimap = (itmap->second).begin(); itmultimap != (itmap->second).end(); ++itmultimap) {
			cout << "\nBucket : " << itmap->first;
			vector<ChunkID>& vec1 = const_cast<vector<ChunkID>&>(itmultimap->first);
			cout << "\nList1 Size = " << vec1.size();
			for (int i = 0; i < vec1.size(); i++) {
				IDInfo li;
				vec1[i].getInfo(li);
				//cout << " " << (li.getIDAsString()).c_str();
				cout << " " << (li.getName()).c_str();
			}
			vector<ChunkID>& vec2 = const_cast<vector<ChunkID>&>(itmultimap->second);
			cout << "\nList2 size = " << vec2.size();
			for (int j = 0; j < vec2.size(); j++) {
				IDInfo li;
				vec2[j].getInfo(li);
				//cout << " " << (li.getIDAsString()).c_str();
				cout << " " << (li.getName()).c_str();
			}
		}
	}
	cout << endl;
}

