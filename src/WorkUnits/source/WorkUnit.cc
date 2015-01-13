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
#include <string.h>

#include "WorkUnit.h"
#include "WorkUnitImp.h"
#include "AggregateWorkUnitImp.h"
#include "JoinWorkUnitImp.h"
#include "SelectionWorkUnitImp.h"
#include "PrintWorkUnitImp.h"

WayPointID &WorkUnit :: GetOwner () {
	return data->GetOwner();
}

void WorkUnit :: SetOwner (WayPointID &newOwner) {
	data->SetOwner (newOwner);
}

void WorkUnit :: swap (WorkUnit &withMe) {
	WorkUnitImp *tmp;
	tmp = data;
	data = withMe.data;
	withMe.data = tmp;
}

WorkUnit :: WorkUnit () {
	data = new WorkUnitImp;
}

WorkUnit :: ~WorkUnit () {
	delete data;
	data = NULL;
}

void WorkUnit :: Run () {
	data->Run ();
}

const char* WorkUnit :: GetDescription () {
	return data->GetDescription ();
}

WorkUnit :: operator JoinWorkUnit &() {
	JoinWorkUnitImp &tmp = dynamic_cast <JoinWorkUnitImp &> (*data);
	return *((JoinWorkUnit *) this);
}

WorkUnit :: operator AggregateWorkUnit &() {
	AggregateWorkUnitImp &tmp = dynamic_cast <AggregateWorkUnitImp &> (*data);
	return *((AggregateWorkUnit *) this);
}

WorkUnit :: operator PrintWorkUnit &() {
	PrintWorkUnitImp &tmp = dynamic_cast <PrintWorkUnitImp &> (*data);
	return *((PrintWorkUnit *) this);
}

WorkUnit :: operator SelectionWorkUnit &() {
	SelectionWorkUnitImp &tmp = dynamic_cast <SelectionWorkUnitImp &> (*data);
	return *((SelectionWorkUnit *) this);
}
