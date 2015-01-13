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

#ifndef INT_INT_MAP_H
#define INT_INT_MAP_H

#include "HashTableMacros.h"
#include "TwoWayList.cc"
#include "Errors.h"
#include <iostream>

// this is an int-to-int map (well, it's actually an HT_INDEX_TYPE-to-unsigned int map) that
// is implemented as a simple, non-self-balancing binary tree.  To guard against really bad
// behaviour when keys are inserted in order, it indexes based upon a hashed value of the key,
// rather than the key itself.  It is thread-safe as long as not more than one writer uses it
// at the same time.  Furthermore, it is efficient in the way it allocates memory, doing it
// a block-at-a-time.  This map is used by hash table segments to externally store offsets
// that are more than 10 bits.  Finally, the implementation assumes that keys which are queried
// for will ALWAYS be there, and that we'll never insert the same key twice.
class EfficientIntToIntMap {

    // this is the basic node type used to build the tree
    struct Node {
        HT_INDEX_TYPE key;
        unsigned int val;
        Node *left;
        Node *right;
    };

    // so that we don't call malloc all of the time, we allocate big chunks of memory,
    // pointed to by this particular pointer
    Node *curMemoryBlock;
    int numInCurBlock;
    int numUsedInCurBlock;

    // this keeps a record of all memory blocks that have been allocated
    typedef Swapify <Node *> SwapifiedPointer;
    TwoWayList <SwapifiedPointer> allMemIveAllocated;

    // this is the root of the tree
    Node *root;

    // this is the value that we XOR with to do the indexing on... that is, we don't index
    // on the key directly because we want to be at least sort of safe with respect to
    // keys coming in sorted order, and we are not doing any sort of automatic rotations
    // to keep the tree balanced
    HT_INDEX_TYPE xOrWithMe;

    void AllocateMoreMem ();

    public:

    ~EfficientIntToIntMap ();

    EfficientIntToIntMap ();

    // inserts a key/value pair
    inline void Insert (HT_INDEX_TYPE key, unsigned int val);

    // find the value for a particular key
    inline unsigned int Find (HT_INDEX_TYPE key);

};

inline void EfficientIntToIntMap :: Insert (HT_INDEX_TYPE key, unsigned int val) {

    // traverse down the tree to get to the insertion spot
    Node **current = &root;
    while (*current != 0) {
        HT_INDEX_TYPE hashedInsertKey = key^xOrWithMe;
        HT_INDEX_TYPE hashedNodeKey = ((*current)->key)^xOrWithMe;
        if (hashedInsertKey < hashedNodeKey)
            current = &((*current)->left);
        else if (hashedInsertKey > hashedNodeKey)
            current = &((*current)->right);
        else {
            std::cerr << hashedInsertKey << " " << hashedNodeKey << "\n";
            std::cerr << key << " " << ((*current)->key) << "\n";
            FATAL ("Why'd I get repeated key values!?\n");
        }
    }

    // get some memory for this new node
    Node *newNode = &(curMemoryBlock[numUsedInCurBlock]);
    numUsedInCurBlock++;

    // if we have no more memory available after this, allocate some more
    if (numUsedInCurBlock == numInCurBlock)
        AllocateMoreMem ();

    // and add the new node in
    newNode->left = 0;
    newNode->right = 0;
    newNode->key = key;
    newNode->val = val;
    *current = newNode;
}

inline unsigned int EfficientIntToIntMap :: Find (HT_INDEX_TYPE key) {

    // traverse down the tree to get to the insertion spot
    Node *current = root;
    while (current != 0) {
        HT_INDEX_TYPE hashedQueryKey = key^xOrWithMe;
        HT_INDEX_TYPE hashedNodeKey = ((current)->key)^xOrWithMe;
        if (hashedQueryKey < hashedNodeKey)
            current = current->left;
        else if (hashedQueryKey > hashedNodeKey)
            current = current->right;
        else
            return current->val;
    }

    FATAL ("You asked me to search for a key that was not there!\n");
}

#endif
