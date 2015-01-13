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

#include <set>

#include "TableScanHelpers.h"

using namespace std;

QueryIDSet QEToBitstring::queryExitToQueryIDSet(Bitstring what){
    QueryIDSet rez;

    QueryExitContainer qeList;
    bitstringToQueryExitContaiener(what, qeList); // the what as a QueryExit list
    // we now scan the qeList and get the queries
    for (qeList.MoveToStart(); !qeList.AtEnd(); qeList.Advance()){
        QueryExit qe = qeList.Current();

        rez.Union(qe.query);
    }

    return rez;
}

void QEToBitstring::bitstringToQueryExitContainer(Bitstring what, QueryExitContainer& where){
    QueryExitContainer rez;

    QueryIDSet s=what;
    while (!s.IsEmpty()){
        QueryID query = s.GetFirst();
        // look for the query in the map and copy the corresponding entry in the result
        map < Bitstring, QueryExit >::iterator it=bitstringToQueryExitMap.find(query);
        assert(it!=bitstringToQueryExitMap.end()); // we'd better find it

        QueryExit qe=(*it).second; // forces a copy

        //		qe.query.PrintBinary();

        rez.Append(qe); // swaps in the copy

    }
    where.swap(rez);
}

Bitstring QEToBitstring::queryExitToBitstring(QueryExitContainer& what, bool create){
    Bitstring rez;
    for (what.MoveToStart(); !what.AtEnd(); what.Advance()){
        QueryExit qe = what.Current();

        assert(!qe.query.IsEmpty() && qe.exit.IsValid());

        // find the bitstring corresponding to qe
        map < QueryExit, Bitstring >::iterator it = queryExitToBitstringMap.find(qe);

        Bitstring query;
        // if not found, we have to crate a Bitstring
        if (it == queryExitToBitstringMap.end() ){
#ifdef DEBUG
            if (!create) qe.Print(); else { cout << "Created "; qe.Print(); }
#endif // DEBUG
            FATALIF(!create, "We should have found the bitstring and we did not");

            query = queryExits.GetNew();
            assert(!query.IsEmpty()); // if it is empty, we are in trouble since we are out of querries
            queryExits.Union(query); // we add the query to our set

            // insert the new bitstring
            queryExitToBitstringMap.insert(pair<QueryExit, Bitstring>(qe, query));
            bitstringToQueryExitMap.insert(pair<Bitstring, QueryExit>(query, qe));
            // WARNING: the query is not inserted in the global qureyExits
        } else {
            query = (*it).second;
        }
        // we now have the query (one way or another), we add it to rez
        rez.Union(query);
    }

    return rez;
}

void QEToBitstring::deleteQueryExits(QueryExitContainer& qExitsDone){

    // get rid of queryExits  that finished
    Bitstring qDone;
    for (qExitsDone.MoveToStart(); !qExitsDone.AtEnd(); qExitsDone.Advance()){
        QueryExit qe = qExitsDone.Current();
        assert( queryExitToBitstringMap.find(qe)!=queryExitToBitstringMap.end() );
        Bitstring query = queryExitToBitstringMap[qe];
        qDone.Union(query);

        // remove the entries corresponding toe qe and query from all the maps
        queryExitToBitstringMap.erase( queryExitToBitstringMap.find(qe) );
        bitstringToQueryExitMap.erase( bitstringToQueryExitMap.find(query) );
    }

    // detete the queries form queryExits
    queryExits.Difference(qDone);
}

void ColumnManager::ChangeMapping(QueryExitToSlotsMap& _queryColumnsMap,
        SlotToSlotMap& _columnsToSlotsMap, SlotToSlotMap& _storeColumnsToSlotsMap,
        QueryExitContainer& qExitsDone){
    // add slots of these queries to our pool
    queryColumnsMap.SuckUp( _queryColumnsMap );

    // add all the slots to columns info
    // must reverse map
    for (_columnsToSlotsMap.MoveToStart(); !_columnsToSlotsMap.AtEnd(); _columnsToSlotsMap.Advance()){
        SlotID phCol = _columnsToSlotsMap.CurrentKey();
        SlotID slot = _columnsToSlotsMap.CurrentData();
        slotsToColumnsMap.Insert(slot, phCol);
    }

    for (_storeColumnsToSlotsMap.MoveToStart(); !_storeColumnsToSlotsMap.AtEnd(); _storeColumnsToSlotsMap.Advance()){
        SlotID phCol = _storeColumnsToSlotsMap.CurrentKey();
        SlotID slot = _storeColumnsToSlotsMap.CurrentData();
        storeSlotsToColumnsMap.Insert(slot, phCol);
    }

    // delete done queryies

    for (qExitsDone.MoveToStart(); !qExitsDone.AtEnd(); qExitsDone.Advance()){
        QueryExit qe = qExitsDone.Current();

        QueryExit dummyKey;
        QueryExitToSlotsMap::dataType dummyData;
        queryColumnsMap.Remove(qe, dummyKey, dummyData);
    }

}

void ColumnManager::UnionColumns(QueryExitContainer& queries, SlotPairContainer& where) {

    // need to gather first all the columns that have to be filled for the
    // given set of queries and then to translate that into slots
    // the actual map of columns needed into stots is provided

    // result set of columns. It is a set so that we do not have duplicates
    set<SlotID> unionColumns;

    for (queries.MoveToStart(); !queries.AtEnd(); queries.Advance()){
        QueryExit qe = queries.Current();

        FATALIF( !queryColumnsMap.IsThere(qe), "We cannot see a map for a query exit" );
        SlotContainer& slots = queryColumnsMap.Find(qe);

        // go through the slots and add them to the set
        for( slots.MoveToStart(); !slots.AtEnd(); slots.Advance() ){
            unionColumns.insert(slots.Current());
        }
    }

    // now unionColumns has all the columns we care about
    // transform it into an IntContainer
    //SlotContainer rez;
    SlotPairContainer rez;
    for( set<SlotID>::iterator it=unionColumns.begin(); it!=unionColumns.end(); it++){
        SlotID col= (*it);
        FATALIF( !slotsToColumnsMap.IsThere(col), "No info on column %d\n", (int)col );
        SlotID phCol = slotsToColumnsMap.Find(col);
        SlotPair pair(col,phCol);

        rez.Insert( pair );
    }

    rez.swap(where);
}

void ColumnManager::GetColsToWrite(SlotPairContainer& where){
    SlotPairContainer rez;

    map<SlotID, SlotID> m;
    for (storeSlotsToColumnsMap.MoveToStart(); !storeSlotsToColumnsMap.AtEnd(); storeSlotsToColumnsMap.Advance()){
        SlotID col = storeSlotsToColumnsMap.CurrentKey();
        SlotID phCol = storeSlotsToColumnsMap.CurrentData();
        m[phCol] = col;
        //SlotPair pair(col,phCol);
        //rez.Append( pair );
    }
    for (map<SlotID, SlotID>::iterator it = m.begin(); it != m.end(); ++it) {
        SlotPair pair(it->second, it->first);
        rez.Append( pair );
    }

    where.swap(rez);
}
