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
#include "LT_Print.h"
#include "WayPointConfigureData.h"
#include <ctime>
#include "Stl.h"
#include <boost/algorithm/string.hpp>

using namespace std;

bool LT_Print::GetConfig(WayPointConfigureData& where){

    // get the ID
    WayPointID printIDOne = GetId ();

    // first, get the function we will send to it
    WorkFunc tempFunc = NULL;
    PrintWorkFunc myPrintOneWorkFunc (tempFunc);
    PrintFinalizeWorkFunc myPrintFinalizeWorkFunc(NULL);
    WorkFuncContainer myPrintOneWorkFuncs;
    myPrintOneWorkFuncs.Insert (myPrintOneWorkFunc);
    myPrintOneWorkFuncs.Insert(myPrintFinalizeWorkFunc);


    // this is the set of query exits that end at it, and flow through it
    QueryExitContainer myPrintOneEndingQueryExits;
    QueryExitContainer myPrintOneFlowThroughQueryExits;
    GetQueryExits (myPrintOneFlowThroughQueryExits, myPrintOneEndingQueryExits);

    PDEBUG("Printing query exits for PRINT WP ID = %s", printIDOne.getName().c_str());
#ifdef DEBUG
        cout << "\nFlow through query exits\n" << flush;
        myPrintOneFlowThroughQueryExits.MoveToStart();
        while (myPrintOneFlowThroughQueryExits.RightLength()) {
                (myPrintOneFlowThroughQueryExits.Current()).Print();
                myPrintOneFlowThroughQueryExits.Advance();
        }
        cout << "\nEnding query exits\n" << flush;
        myPrintOneEndingQueryExits.MoveToStart();
        while (myPrintOneEndingQueryExits.RightLength()) {
                (myPrintOneEndingQueryExits.Current()).Print();
                myPrintOneEndingQueryExits.Advance();
        }
        cout << endl;
#endif

        QueryToFileInfoMap info;
        FOREACH_STL(el, infoMap){
            QueryID query = el.first;
            Json::Value & qInfo = el.second;
            string fileName = "RESULTS/";

            if( qInfo[J_FILE].asString() == "" )
            {
                fileName+=GetQueryName(query)+".csv";
                time_t date = time(NULL);
                struct tm* ptm = localtime(&date);
                char buff[64];
                strftime(buff, 64, "%m.%d.%Y %H:%M:%S", ptm);
                fileName+=" ";
                fileName+=buff;
            }
            else {
                string fileN=qInfo[J_FILE].asString();
                if (fileN[0]=='/') // absolute path
                    fileName=fileN;
                else // normal filename, put in directory RESULTS
                    fileName+=fileN;
            }

            PrintHeader header;
            header.fromJson(qInfo[J_HEADER]);
            string sep = qInfo[J_SEP].asString();
            string type = qInfo[J_TYPE].asString();
            int limit = qInfo[J_LIMIT].asInt();

            PrintFileInfo inf(fileName, sep, type, limit, header);
            info.Insert(query, inf);
        }END_FOREACH;

    // here is the waypoint configuration data
        PrintConfigureData printOneConfigure(
            printIDOne,
            myPrintOneWorkFuncs,
            myPrintOneEndingQueryExits,
            myPrintOneFlowThroughQueryExits,
            info);

    where.swap (printOneConfigure);

    return true;
}

void LT_Print::ReceiveAttributesTerminating(QueryToSlotSet& atts)
{
  for (QueryToSlotSet::const_iterator iter = atts.begin();
                                      iter != atts.end();
                                      ++iter) {
    QueryID query = iter->first;
    if (DoIHaveQueries(query))  {
      SlotSet atts_s = iter->second;
      CheckQueryAndUpdate(query, atts_s, downAttributes);
    }
  }
}

void LT_Print::DeleteQuery(QueryID query)
{
    DeleteQueryCommon(query);
    infoMap.erase(query);
}

void LT_Print::ClearAllDataStructure() {
    ClearAll();
    infoMap.clear();
}
bool LT_Print::AddPrint(QueryID query, SlotSet& atts, Json::Value& info)
{
    infoMap[query] = info;

    cout << "Adding query " << GetQueryName(query) << endl;
    bool isNew = CheckQueryAndUpdate(query, atts, newQueryToSlotSetMap);

    queriesCovered.Union(query);
    return true;
}

// Implementation top -> down as follows per query:
// 1. used = used + new attributes added since last analysis
// 2. clear the new attributes
// 3. result = used attributes
bool LT_Print::PropagateDownTerminating(QueryID query, const SlotSet& atts/*blank*/, SlotSet& result, QueryExit qe)
{

    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();
    result = used[query];
    queryExitTerminating.Insert(qe);
    return true;
}

// Implementation bottom -> up as follows for all queries together:
// 1. used = used + new queries attributes added since last analysis
// 2. clear the new data
// 3. result = NONE
// 4. Print is last destination hence result is blank
// 5. old used + new = used is good to check correctness if they are subset of down attributes
bool LT_Print::PropagateUp(QueryToSlotSet& result)
{
    CheckQueryAndUpdate(newQueryToSlotSetMap, used);
    newQueryToSlotSetMap.clear();
    result.clear();

    // Correctness
    // used should be subset of what is coming from below
    if (!IsSubSet(used, downAttributes))
    {
        cout << "Print WP : Attribute mismatch : used is not subset of attributes coming from below\n";
        return false;
    }
    downAttributes.clear();
    return true;
}

Json::Value LT_Print::GetJson() {
    Json::Value data(Json::objectValue);

    data[J_PAYLOAD] = MapToJson(infoMap);
    data[J_NAME] = GetWPName();
    data[J_TYPE] = JN_PRINT_WP;

    SlotToQuerySet reverse;
    AttributesToQuerySet(used, reverse);
    data[J_ATT_MAP] = JsonAttToQuerySets(reverse);

    return data;
}
