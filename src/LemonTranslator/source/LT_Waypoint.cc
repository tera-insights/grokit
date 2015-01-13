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
#include "LT_Waypoint.h"
#include "QueryManager.h"
#include "AttributeManager.h"

using namespace std;

string LT_Waypoint::GetWPName(){
    IDInfo info;
    GetId().getInfo(info);
    return info.getName();

}

// Update (query, attributes) pair in destination if not bypassed
// return true if query is new
bool LT_Waypoint::CheckQueryAndUpdate(QueryID query,
        SlotSet& atts,
        QueryToSlotSet& destination,
        bool skipByPassQueries)
{
    bool isNew=false;
    //if (!skipByPassQueries && !query.IsSubsetOf(bypassQueries)) // subset not working good
    if (!skipByPassQueries)
    {
        QueryToSlotSet::const_iterator it = destination.find(query);
        if (it == destination.end()){// new query
            destination[query] = atts;
            isNew = true;
        }
        else {
            SlotSet newatts;
            set_union(atts.begin(), atts.end(), (it->second).begin(), (it->second).end(), inserter(newatts, newatts.begin()));
            destination[query] = newatts; // override query entry with new set of attributes
        }
    }
    return isNew;
}

// Iterate on each source and update (query, attributes) pair in destination
void LT_Waypoint::CheckQueryAndUpdate(QueryToSlotSet& source,
        QueryToSlotSet& destination,
        bool skipByPassQueries)
{
    for (QueryToSlotSet::const_iterator iter = source.begin();
            iter != source.end();
            ++iter) {
        QueryID query = iter->first;
        SlotSet atts = iter->second;
        CheckQueryAndUpdate(query, atts, destination, skipByPassQueries);
    }
}

bool LT_Waypoint::IsSubSet(QueryToSlotSet& subset, QueryToSlotSet& superset)
{
    // for each subset query, check in superset
    for (QueryToSlotSet::const_iterator it_sub = subset.begin(); it_sub != subset.end(); ++it_sub) {
        QueryToSlotSet::const_iterator it_super = superset.find(it_sub->first);
        if (it_super == superset.end()) {
            FATAL( "%s: query being tested is not in superset %s" , GetWPName().c_str(), (GetQueryName(it_sub->first)).c_str());
            cout << GetWPName() << ": query being tested is not in superset " << (GetQueryName(it_sub->first)).c_str();
            return false;
        }
        if (!IsSubSet(it_sub->second, it_super->second))
        {
            cout << "\n " << myId.getName() << " -> ";
            cout << "Some subset attributes are not found in superset query : " << (GetQueryName(it_sub->first)).c_str();
            return false;
        }
    }
    return true;
}

bool LT_Waypoint::IsSubSet(const SlotSet& subset, QueryToSlotSet& superset)
{
    // subset attributes should be present in all superset queries
    for (QueryToSlotSet::const_iterator it = superset.begin(); it!= superset.end(); ++it) {
        if (!IsSubSet(subset, it->second))
        {
            cout << "\nSome subset attributes are not found in superset query : " << (GetQueryName(it->first)).c_str();
            return false;
        }
    }
    return true;
}

bool LT_Waypoint::IsSubSet(const SlotSet& subset, const SlotSet& superset)
{
    SlotSet difference;
    set_difference(subset.begin(), subset.end(), superset.begin(), superset.end(), inserter(difference, difference.begin()));
    if (difference.size() != 0) {
        bool first;
        cout << "\nSome subset attributes are not found in superset: superset(";
        first = true;
        for (SlotSet::const_iterator i = superset.begin(); i != superset.end(); ++i)
        {
            SlotID s = (*i);
            AttributeManager& am = AttributeManager::GetAttributeManager();
            if( first )
                first = false;
            else
                cout << ", ";
            cout << am.GetAttributeName(s).c_str();
        }
        cout << ") subset(";
        first = true;
        for (SlotSet::const_iterator i = subset.begin(); i != subset.end(); ++i)
        {
            SlotID s = (*i);
            AttributeManager& am = AttributeManager::GetAttributeManager();
            if( first )
                first = false;
            else
                cout << ", ";
            cout << am.GetAttributeName(s).c_str();
        }
        cout << ") difference(";
        first = true;
        for (SlotSet::const_iterator i = difference.begin(); i != difference.end(); ++i)
        {
            SlotID s = (*i);
            AttributeManager& am = AttributeManager::GetAttributeManager();
            if( first )
                first = false;
            else
                cout << ", ";
            cout << am.GetAttributeName(s).c_str();
        }
        cout << ")";
        return false;
    }
    return true;
}

string LT_Waypoint::GetQueryName(QueryID query)
{
    string name;
    QueryManager& qm = QueryManager::GetQueryManager();
    FATALIF( !qm.GetQueryName(query, name), "Did not find the query name for QID=%d", query.GetInt());
    return name;
}

void LT_Waypoint::GetQueryExits (QueryExitContainer& qeflowthrough, QueryExitContainer& qeEnding) {
    qeflowthrough.copy(queryExit);
    qeEnding.copy(queryExitTerminating);
}

void LT_Waypoint::PrintAttributeList(const SlotSet& atts, ostream& out){
    AttributeManager& am = AttributeManager::GetAttributeManager();

    for (SlotSet::const_iterator it = atts.begin(); it != atts.end();)
    {
        SlotID s = *it;
        out << am.GetAttributeName(s).c_str();
        ++it;
        if (it!=atts.end())
            out << ",";
    }
}

void LT_Waypoint::PrintAllAttributes(const SlotSet& atts)
{
    AttributeManager& am = AttributeManager::GetAttributeManager();

    cout << "\nAttributes : ";
    PrintAttributeList(atts, cout);
}

void LT_Waypoint::PrintAllQueryAndAttributes(QueryToSlotSet& querySlotMap)
{
    for (QueryToSlotSet::const_iterator it = querySlotMap.begin(); it != querySlotMap.end(); ++it)
    {
        cout << "\nQuery " << (GetQueryName(it->first)).c_str();
        PrintAllAttributes(it->second);
    }
}

void LT_Waypoint::ReceiveAttributes(QueryToSlotSet& atts) {
    for (QueryToSlotSet::const_iterator iter = atts.begin();
                                                                            iter != atts.end();
                                                                            ++iter) {
        QueryID query = iter->first;
        if (DoIHaveQueries(query))    {
            SlotSet atts_s = iter->second;
            CheckQueryAndUpdate(query, atts_s, downAttributes);
        }
    }
}

void LT_Waypoint::ClearAll() {
    queriesCovered = 0;
    downAttributes.clear();
    used.clear();
    newQueryToSlotSetMap.clear();
    bypassQueries = 0;
    queryExit.Clear();
}

void LT_Waypoint::DeleteQueryCommon(QueryID query)
{
    queriesCovered.Difference(query);
    bypassQueries.Difference(query);
    used.erase(query);
    newQueryToSlotSetMap.erase(query);
    downAttributes.erase(query);

    queryExit.MoveToStart();
    while (queryExit.RightLength()) {
        if ((queryExit.Current()).query == query) {
            QueryExit tmp;
            queryExit.Remove (tmp);
        }
        queryExit.Advance ();
    }
}

void LT_Waypoint::AttributesToQuerySet(QueryToSlotSet& atts, SlotToQuerySet& slotToQueries) {
    if (atts.empty()) return;
    slotToQueries.clear();
    for (QueryToSlotSet::const_iterator it = atts.begin();
                                                                            it != atts.end();
                                                                            ++it) {
        QueryID q = it->first;
        const SlotSet& s = it->second;
        for (SlotSet::const_iterator its = s.begin(); its != s.end();    ++its) {
            SlotID slot = *its;
            SlotToQuerySet::const_iterator i = slotToQueries.find(slot);
            if (i == slotToQueries.end()) {
                // If slotID not found create a slot with the current query
                slotToQueries[slot]=q;
            } else {
                // found previous sets, add this query
                QueryIDSet qSet = i->second;
                qSet.Union(q);
                slotToQueries[slot]=qSet;
            }
        }
    }
}

void LT_Waypoint::PrintAttToQuerySets( SlotToQuerySet& input, ostream & out ) {
    AttributeManager& am = AttributeManager::GetAttributeManager();

    out << "</";
    for( SlotToQuerySet::const_iterator it = input.begin(); it != input.end(); ) {
        SlotID slot = it->first;
        QueryIDSet queries = it->second;
        out << "(" << am.GetAttributeName(slot) << ","
            << queries.ToString() << ")";

        ++it;

        if( it != input.end() )
            out << ", ";
    }

    out << "/>";
}

void LT_Waypoint::PrintAttToQuerySetsJoin( SlotToQuerySet& input, ostream & out ) {
    AttributeManager& am = AttributeManager::GetAttributeManager();

    out << "(";
    for( SlotToQuerySet::const_iterator it = input.begin(); it != input.end(); ) {
        SlotID slot = it->first;
        QueryIDSet queries = it->second;
        out << "(" << am.GetAttributeName(slot) << ","
            << queries.ToString() << ")";

        ++it;

        if( it != input.end() )
            out << ", ";
    }

    out << ")";
}

Json::Value LT_Waypoint::JsonAttToQuerySets(SlotToQuerySet& input){

    AttributeManager& am = AttributeManager::GetAttributeManager();

    Json::Value list(Json::objectValue);
    for (SlotToQuerySet::const_iterator it=input.begin();
             it!=input.end(); ++it){
        SlotID slot = it->first;
        QueryIDSet queries = it->second;
        list[am.GetAttributeName(slot)] = queries.ToString();
    }

    return list;
}

Json::Value LT_Waypoint::JsonAttToQuerySets(QueryToSlotSet& atts){
    SlotToQuerySet reverse;
    AttributesToQuerySet(atts, reverse);
    return JsonAttToQuerySets(reverse);
}

Json::Value LT_Waypoint::MapToJson(QueryToJson& map) {
    Json::Value res(Json::objectValue);

    for( auto el : map ) {
        QueryID id = el.first;
        Json::Value & val = el.second;
        res[GetQueryName(id)] = val;
    }

    return res;
}

Json::Value LT_Waypoint::JsonAttributeList(const SlotSet& atts){
    AttributeManager& am = AttributeManager::GetAttributeManager();

    Json::Value list(Json::arrayValue);
    for (SlotSet::const_iterator it = atts.begin(); it != atts.end(); ++it)
    {
        list.append(am.GetAttributeName(*it).c_str());
    }

    return list;
}

Json::Value LT_Waypoint::JsonAttributeList(const SlotVec& atts) {
    AttributeManager& am = AttributeManager::GetAttributeManager();

    Json::Value list(Json::arrayValue);
    for (SlotVec::const_iterator it = atts.begin(); it != atts.end(); ++it)
    {
        list.append(am.GetAttributeName(*it).c_str());
    }

    return list;
}

Json::Value LT_Waypoint::JsonQuerySet(const QueryIDSet& queries) {
    QueryManager & qm = QueryManager::GetQueryManager();
    Json::Value ret(Json::arrayValue);

    QueryIDSet tmp = queries;
    while( !tmp.IsEmpty() ) {
        QueryID x = tmp.GetFirst();
        std::string name;
        qm.GetQueryName(x, name);
        ret.append(name);
    }

    return ret;
}


void LT_Waypoint::SlotContainerToSet( SlotContainer& cont, SlotSet& result ) {
    result.clear();

    for( cont.MoveToStart(); !cont.AtEnd(); cont.Advance() ) {
        SlotID curID = cont.Current();
        result.insert(curID);
    }
}


