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
// for Emacs -*- c++ -*-

#ifndef _WORKUNIT_H_
#define _WORKUNIT_H_

#include "ID.h"

/**
	* Class WorkUnit is an interface specification class for all work units

	* Only the interface provided in this class can be used inside the Scheduler.

	* Each work unit belongs to a WayPoint that needs to get it back
	* when it is executed. A type, which is WayPoint specific, can be
	* specified as well to help the WayPoint manage it better.

	* Any state changed by the WorkUnit and any output produced can be extracted
	* only by the WayPoint that knows how to create/use these type fo WorkUnits

	* The work units live only termporarily outside the WayPoint that
	* created them and they are not passed arround except for execution

	* Assumptions:
	* 1. The WorkUnit assumes that the method Run is executed only once
	* 		and that it is always executed once the WorkUnit is created
	* 2. The WorkUnit assumes that it is given back the the crator WayPoint
	* 		to extract the results.
*/

class WorkUnitImp;
class JoinWorkUnit;
class SelectionWorkUnit;
class AggregateWorkUnit;
class PrintWorkUnit;

class WorkUnit {

protected:

	WorkUnitImp *data;

public:

	// constructor and destructor
	WorkUnit ();
	virtual ~WorkUnit ();

	// gets the owner... used for routing
	WayPointID &GetOwner ();

	// sets the owner
	void SetOwner (WayPointID &owner);

	// swap two generic WorkUnits
	void swap (WorkUnit &withMe);

	// actually run the work that this guy needs to do
	void Run();

	// get the description of what this work unit does for the logging
	const char* GetDescription(void);

	// these are type-safe downcast operations
	operator JoinWorkUnit &();
	operator SelectionWorkUnit &();
	operator AggregateWorkUnit &();
	operator PrintWorkUnit &();
};

#endif
