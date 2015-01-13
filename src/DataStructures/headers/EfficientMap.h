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

#ifndef _EFFICIENT_MAP_H
#define _EFFICIENT_MAP_H

// This is a template for a lookup table, associating a key
// with some data.  For large tables, this is going to be more
// efficient than the InefficientMap template, since it makes use
// of a reasonably complex skip list data struture to obtain log-n
// inserts, lookups, and removes.  However, for small tables (less
// than 20 items) it is probably going to be less efficient due to
// the extra overhead that the more complex data structure incurres.
// Also, unlike the InefficientMap, it requires the LessThan () over keys.
//
// Key requires swap (), IsEqual (), LessThan ()... Data requires swap ()
//

// this constant is relatively unimportant... the data structure will have
// efficient performance up to approximately 2^MAXLEVELS items, but there is
// no reason to have it too large!
#define MAXLEVELS 64

#include <cstddef>
#include "Config.h"
#include "Swap.h"
#include "Swapify.h"
#include "SerializeJson.h"

// Helper macros for Frequently Done Actions
// Macro to streamline scan of TwoWayLists
// list "list" is scanned and element is local variable that is
// instantiated as each element of the list
// Usage Scenario:
//   FOREACH_EM(key, data, myMap){
//     do something with key and data
//   }END_FOREACH
//
//
#ifdef _HAS_AUTO
#define FOREACH_EM(key, data, map) \
    for((map).MoveToStart(); !(map).AtEnd(); (map).Advance()) { \
    auto & key = (map).CurrentKey(); \
    auto & data = (map).CurrentData();
#else // _HAS_AUTO
#define FOREACH_EM(key, data, map) \
    for((map).MoveToStart();    !(map).AtEnd(); (map).Advance()) { \
    typeof((map).CurrentKey())& key = (map).CurrentKey(); \
    typeof((map).CurrentData())& data = (map).CurrentData();
#endif // _HAS_AUTO

#ifndef END_FOREACH
#define END_FOREACH }
#endif


template <class Key, class Data>
class EfficientMap {

    // forward declaration
    struct Node;

public:
    typedef Key keyType;
    typedef Data dataType;


    // constructor and destructor
    EfficientMap ();
    virtual ~EfficientMap ();

    EfficientMap( EfficientMap<Key, Data> & other );
    EfficientMap & operator = ( EfficientMap<Key, Data> & other );

    // remove all the content
    void Clear(void);

    // inserts the key/data pair into the structure
    void Insert (Key &key, Data &data);

    // eat up another map
    // plays nicely and removes duplicates
    void SuckUp(EfficientMap<Key, Data>& other);

    // get the content from another map (without destroying it)
    void copy(EfficientMap<Key, Data>& other);

    // removes one (any) instance of the given key from the map...
    // returns a 1 on success and a zero if the given key was not found
    int Remove (const Key &findMe, Key &putKeyHere, Data &putDataHere);

    // attempts to locate the given key
    // returns 1 if it is, 0 otherwise
    int IsThere (const Key &findMe);

    // returns a reference to the data associated with the given search key
    // if the key is not there, then a garbage (newly initialized) Data item is
    // returned.  "Plays nicely" with IsThere in the sense that if IsThere found
    // an item, Find will immediately return that item w/o having to locate it
    Data &Find (const Key &findMe);

    // swap two of the maps
    void swap (EfficientMap<Key, Data> &withMe);

    ///////////// ITERATOR INTERFAACE //////////////
    // look at the current item
    Key& CurrentKey () const;
    Data& CurrentData () const;

    // move the current pointer position backward through the list
    void Retreat ();

    // move the current pointer position forward through the list
    void Advance ();

    // operations to consult state
    bool AtStart () const;
    bool AtEnd () const;

    // operations to move the the start of end of a list
    void MoveToStart ();
    void MoveToFinish ();

    // debugging function
    void Print();

    // Go To/From JSON
    void toJson( Json::Value & dest ) const;
    void fromJson( const Json::Value & src );

private:

    // Data invalidData; // object returned when Find() fails

    // these are versions of the above ops that work only at a specific level of the skip list
    void Insert (Node *temp, int whichLevel);
    void Remove (Node *&removeMe, int whichLevel);
    void Advance (int whichLevel);
    int AtEnd (int whichLevel) const;
    Key& CurrentKey (int i) const;
    Data& CurrentData (int i) const;

    struct Node {

        // data
        Key key;
        Data data;

        // backward link
        Node *previous;

        // forward links
        Node **next;
        int numLinks;

        // constructors and destructor
        Node (int numLinksIn) {numLinks = numLinksIn;
            next = new Node *[numLinks];}
        Node () {previous = next = NULL;
            numLinks = 0;}
        virtual ~Node () {delete [] next;}

    };

    struct Header {
        // data
        Node * first;
        Node * last;
        Node * current;
        int curDepth;
    };

    // the list itself is pointed to by this pointer
    Header *list;
};

template< typename K, typename V >
void ToJson( const EfficientMap<K, V> & src, Json::Value & dest ) {
    src.toJson(dest);
}

template< typename K, typename V >
void FromJson( const Json::Value & src, EfficientMap<K, V> & dest ) {
    dest.fromJson(src);
}

template< class Key, class Data >
void swap( EfficientMap<Key, Data>& a, EfficientMap<Key, Data>& b ) {
    a.swap(b);
}

#include "EfficientMap.cc"

#endif
