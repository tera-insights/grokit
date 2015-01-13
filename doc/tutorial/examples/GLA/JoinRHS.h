//
//  Copyright 2012 Christopher Dudley
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
#ifndef _JOIN_RHS_H_
#define _JOIN_RHS_H_

// STL includes
#include <utility>
#include <unordered_map>

// DataPath library includes
#include "base/Types/BIGINT.h"
#include "base/Types/HString.h"

//! [description]
/* System description block
 *  GLA_DESC
 *      NAME(</JoinRHS/>)
 *      INPUTS(</(key, BIGINT), (value, HString)/>)
 *      OUTPUTS(</(self, STATE)/>)
 *      RESULT_TYPE(</state/>)
 *  END_DESC
 */
//! [description]

/*
 *  This GLA takes key-value pairs and creates lists of values with the
 *  same key. This is used to create a map that is used as the right hand
 *  side of an asymmetric hash join.
 */

class JoinRHS {

public:
    // Typedefs for clarity
    typedef BIGINT                          KeyType;
    typedef HString                         ElemType;
    typedef std::pair<KeyType, ElemType>    ValueType;

    // The type of the map used
    typedef std::unordered_multimap<KeyType, ElemType> MapType;

    // An Iterator used to return values that have a certain key.
    class Iterator {
        // Internal typedef for the map's iterator type.
        typedef MapType::const_iterator IterType;

        // The current item
        IterType cur;
        // The end of the range of items
        IterType end;

    public:
        Iterator( IterType begin, IterType end ) : cur(begin), end(end)
        {}

        // If there is another value to produce, puts it in putHere and returns
        // true. Otherwise, returns false.
        bool GetNext( ElemType& putHere ) {
            if( cur == end )
                return false;

            putHere = cur->second;
            ++cur;
            return true;
        }
    };

private:

    // The map we are using as our hash table
    MapType hashTable;

public:

    //! [constructor]
    // Constructor
    // Don't need to do anything special
    JoinRHS()
    { }
    //! [constructor]

    //! [destructor]
    // Destructor
    ~JoinRHS() noexcept
    { }
    //! [destructor]

    //! [add-item]
    // Add a single item to the hash table
    void AddItem( const INT& key, const HString& value ) {
        ValueType newValue( key, value );
        hashTable.insert( newValue );
    }
    //! [add-item]

    //! [add-state]
    // Take the data from other and put it in our state.
    void AddState( JoinRHS& other ) {
        hashTable.insert( other.begin(), other.end() );
    }
    //! [add-state]

    //! [lookup]
    /* Look up values pertaining to a certain key.
     * Note: this object will be passed as a constant reference to anything
     * that would be using this function, so we have to mark it as const so
     * the compiler does not fail to compile the code due to violation of
     * const-ness.
     */
    Iterator Lookup( const KeyType& key ) const {
        // Typedef for readibility
        typedef std::pair<MapType::const_iterator, MapType::const_iterator> RangeType;

        RangeType result = hashTable.equal_range( key );
        return Iterator( result->first, result->second );
    }
    //! [lookup]
};

#endif // _JOIN_RHS_H_
