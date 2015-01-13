//
//  Copyright 2014 Tera Insights, LLC
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

#include "LT_Cluster.h"
#include "AttributeManager.h"
#include "WorkFuncs.h"

#include <iostream>

LT_Cluster::LT_Cluster(WayPointID id, std::string _rel, SlotID _att, QueryID _query):
	LT_Waypoint(id),
	relation(_rel),
	clusterAtt(_att),
	query(_query)
{
	SlotSet tmp;
	tmp.insert(clusterAtt);
	used[query] = tmp;
	queriesCovered.Union(query);
}

bool LT_Cluster::PropagateDownTerminating(
	QueryID query,
	const SlotSet& atts,
	SlotSet& result,
	QueryExit qe)
{
	result.clear();
	result = used[query];
	queryExitTerminating.Insert(qe);

	return true;
}

bool LT_Cluster::PropagateUp(QueryToSlotSet& result) {
	result.clear();

	if( !IsSubSet(used, downAttributes) ) {
		std::cerr << "Cluster WP: Attribute mismatch: used is not "
			"a subset of attributes coming from below";
		return false;
	}

	downAttributes.clear();
	return true;
}

void LT_Cluster::ReceiveAttributesTerminating(QueryToSlotSet& atts) {
	for (QueryToSlotSet::const_iterator iter = atts.begin();
		iter != atts.end();
		++iter)
	{
		QueryID query = iter->first;
		if (DoIHaveQueries(query))  {
			SlotSet atts_s = iter->second;
			CheckQueryAndUpdate(query, atts_s, downAttributes);
		}
	}
}

Json::Value LT_Cluster::GetJson() {
	Json::Value data(Json::objectValue);
	AttributeManager& am = AttributeManager::GetAttributeManager();

	data[J_PAYLOAD] = am.GetAttributeName(clusterAtt);
	data[J_NAME] = GetWPName();
	data[J_TYPE] = JN_CLUSTER_WP;

	return data;
}

bool LT_Cluster::GetConfig(WayPointConfigureData& where) {
	WayPointID myID = GetId();

	// Set up the work function container
	WorkFuncContainer myWorkFuncs;
	WorkFunc nullFunc = 0;
	ClusterProcessChunkWorkFunc processChunkWF(nullFunc);
	myWorkFuncs.Insert(processChunkWF);

	QueryExitContainer endingQueryExits;
	QueryExitContainer thruQueryExits;
	GetQueryExits(thruQueryExits, endingQueryExits);

	ClusterConfigureData configData(
		myID,
		myWorkFuncs,
		endingQueryExits,
		thruQueryExits,
		relation
		);

	where.swap(configData);

	return true;
}