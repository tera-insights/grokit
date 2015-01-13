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
#ifndef _SCANNER_CONFIG_H_
#define _SCANNER_CONFIG_H_

#include "Swap.h"
#include "ID.h"
#include <string>

/** class to allow configuration of scanners.

  it contains the info in the message that gets sent to the
  scanner. This is the equivalent of the SymbolicWPConfig for normal.

Florin: add serialization to this
*/

class ScannerConfig {

    protected:
        QueryExitContainer queryExits;
        QueryExitContainer qExitsDone;
        SlotToSlotMap columnsToSlotsMap;
        QueryExitToSlotsMap queryColumnsMap;
        std::string relation; // relation name

    public:
        ScannerConfig(){}

        ScannerConfig(std::string _relation, QueryExitContainer& _queryExits, QueryExitContainer& _qExitsDone,
                SlotToSlotMap& _columnsToSlotsMap, QueryExitToSlotsMap& _queryColumnsMap);


        void GetData(std::string& _relation, QueryExitContainer& _queryExits,
                QueryExitContainer& _qExitsDone,
                SlotToSlotMap& _columnsToSlotsMap,
                QueryExitToSlotsMap& _queryColumnsMap);


        void swap(ScannerConfig& o);

};

// Override global swap
void swap( ScannerConfig & a, ScannerConfig & b );

typedef TwoWayList<ScannerConfig> ScannerConfigContainer;

// INLINE FUNCTIONS

inline ScannerConfig::ScannerConfig(std::string _relation, QueryExitContainer& _queryExits,
        QueryExitContainer& _qExitsDone,
        SlotToSlotMap& _columnsToSlotsMap,
        QueryExitToSlotsMap& _queryColumnsMap)
    :relation(_relation){

        queryExits.swap(_queryExits);
        qExitsDone.swap(_qExitsDone);
        columnsToSlotsMap.swap(_columnsToSlotsMap);
        queryColumnsMap.swap(_queryColumnsMap);
    }

inline void ScannerConfig::GetData(std::string& _relation, QueryExitContainer& _queryExits,
        QueryExitContainer& _qExitsDone,
        SlotToSlotMap& _columnsToSlotsMap,
        QueryExitToSlotsMap& _queryColumnsMap){

    _relation=relation;
    queryExits.swap(_queryExits);
    qExitsDone.swap(_qExitsDone);
    columnsToSlotsMap.swap(_columnsToSlotsMap);
    queryColumnsMap.swap(_queryColumnsMap);
}

inline void ScannerConfig::swap(ScannerConfig& o){
    SWAP_STD(relation, o.relation);
    queryExits.swap(o.queryExits);
    qExitsDone.swap(o.qExitsDone);
    columnsToSlotsMap.swap(o.columnsToSlotsMap);
    queryColumnsMap.swap(o.queryColumnsMap);
}

inline
void swap( ScannerConfig & a, ScannerConfig & b ) {
    a.swap(b);
}


#endif //  _SCANNER_CONFIG_H_
