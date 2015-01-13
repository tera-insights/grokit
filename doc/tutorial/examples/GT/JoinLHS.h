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
#ifndef _JOIN_LHS_H_
#define _JOIN_LHS_H_

// DataPath library includes
#include "base/Types/BIGINT.h"
#include "base/Types/HString.h"

//! [description]
/* System description block
 *  GT_DESC
 *      NAME(</JoinLHS/>)
 *      INPUTS(</(key, BIGINT), (value, HString)/>)
 *      OUTPUTS(</(key, BIGINT), (valueLHS, HString), (valueRHS, HString)/>)
 *      RESULT_TYPE(</multi/>)
 *
 *      REQ_CONST_STATES(</(hashTable, JoinRHS)/>)
 *  END_DESC
 */
//! [description]

/*
 *  This GT takes key-value pairs and looks them up in a hash table created by
 *  a JoinRHS GLA. If it finds a match, it joins the tuples together and
 *  outputs them.
 */

class JoinLHS {
    // Typedef for the iterator we'll be using
    typedef JoinRHS::Iterator   IterType;

    // Reference to the JoinRHS GLA.
    const JoinRHS& hashTable;

    // Internal iterator for producing values for a key
    IterType curIter;

    // Cached input values for producing output
    BIGINT key;
    HString valueLHS;

public:

    //! [constructor]
    /*
     *  Constructor
     *  We take in a reference to the hash table as the parameter. The system
     *  will make sure that the JoinRHS has run to completion before this
     *  is constructed.
     */
    JoinLHS( const JoinRHS& _hashTable ) : hashTable( hashTable )
    { }
    //! [constructor]

    //! [destructor]
    // Destructor
    ~JoinLHS() noexcept
    { }
    //! [destructor]

    //! [process-tuple]
    // Cache the input tuple and look up the key in the hash table.
    void ProcessTuple( const BIGINT& key, const BIGINT& value ) {
        this->key = key;
        this->valueLHS = value;

        curIter = hashTable.Lookup( key );
    }
    //! [process-tuple]

    //! [get-next-result]
    // If we have another tuple matched in the RHS, join them together and
    // return it.
    bool GetNextResult( BIGINT& key, HString& valueLHS, HString& valueRHS ) {
        if( curIter.GetNext( valueRHS ) ) {
            // We found a new tuple in the RHS.
            key = this->key;
            valueLHS = this->valueLHS;
            return true;
        }

        return false;
    }
    //! [get-next-result]
};

#endif // _JOIN_LHS_H_
