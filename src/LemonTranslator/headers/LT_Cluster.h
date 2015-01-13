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
#ifndef _LT_CLUSTER_H_
#define _LT_CLUSTER_H_

#include "LT_Waypoint.h"
#include "AttributeManager.h"

class LT_Cluster : public LT_Waypoint {
private:

	std::string relation;
	SlotID clusterAtt;
	QueryID query;

public:

	LT_Cluster(WayPointID id, std::string _rel, SlotID _att, QueryID _query);

	virtual WaypointType GetType() {
		return ClusterWaypoint;
	}

	virtual bool PropagateDownTerminating(
		QueryID query, const SlotSet& atts,
		SlotSet& result, QueryExit qe);

	virtual bool PropagateUp(QueryToSlotSet& result);

	virtual void ReceiveAttributesTerminating(QueryToSlotSet& atts);

	virtual Json::Value GetJson();

	virtual bool GetConfig(WayPointConfigureData& where);
};

#endif // _LT_CLUSTER_H_