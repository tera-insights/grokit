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
#ifndef CLUSTER_WP_IMP
#define CLUSTER_WP_IMP

#include "WayPointImp.h"
#include "WorkDescription.h"

#include <map>
#include <cstddef>

class ClusterWayPointImp : public WayPointImp {
private:
	std::string relation;

	void SendFlushRequest();

public:
	ClusterWayPointImp();
	virtual ~ClusterWayPointImp();

	virtual void TypeSpecificConfigure(WayPointConfigureData& configData) override;
	virtual void ProcessHoppingDownstreamMsg(HoppingDownstreamMsg& message) override;
	virtual void DoneProducing(
		QueryExitContainer &whichOnes,
		HistoryList &history,
		int result,
		ExecEngineData& data) override;
	virtual void ProcessHoppingDataMsg(HoppingDataMsg& data);
};

#endif // CLUSTER_WP_IMP