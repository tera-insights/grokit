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
#include "SymbolicWaypointConfig.h"
#include "QueryManager.h"
#include "Swap.h"

using namespace std;

WPTypeTranslator nameofTypes[NUM_WAYPOINT_TYPES] = {
	{ "Selection", SelectionWaypoint},
	{ "Join", JoinWaypoint},
	{ "Print", PrintWaypoint},
  { "Scanner", ScannerWaypoint}
};


SymbolicWaypointConfig::SymbolicWaypointConfig() {
}

SymbolicWaypointConfig::SymbolicWaypointConfig(WaypointType type, WayPointID id) {
	// copy values
	wtype = type;
	wid = id;
}

void SymbolicWaypointConfig::SetType(const char* type){
	wtype=InvalidWaypoint;
	for (int i=0; i<NUM_WAYPOINT_TYPES; i++){
		if (strcasecmp(type, nameofTypes[i].typeName)==0){
			wtype=nameofTypes[i].type;
			break;
		}
	}

	FATALIF(wtype==InvalidWaypoint, "value %s does not name a type", type);
}


/** The format of the entry assumed is:
	* WaypointName WaypointType  #noParams
	* param_name p_type value
	* param_name p_type value
	* param_name p_type value

	* If the value is a string, it has to be $ terminated. This allows multi-line
	* arguments that are needed for large parameters.

	* The argument is the stream from which we read the description
**/

SymbolicWaypointConfig::SymbolicWaypointConfig(FILE* fromMe){
	// we read the entry in the stream and load up the parameters
	char name[100];
	char type[30];
	int numParams;

	fscanf(fromMe, "%s %s %d", name, type, &numParams);

	wid=WayPointID(name);

	SetType(type);

	QueryManager& qm=QueryManager::GetQueryManager();

	// we now read the parameters
	for (int p=0; p<numParams; p++){
		fscanf(fromMe, "%s %s", name, type);
		// have to figure out if the parameter is (query,name) or (name,type)
		// is arg1 a query?
		QueryID query=qm.GetQueryID(name);
		if (query.IsEmpty()){
			// then type is the parameter name

			if (strcasecmp(type,"int")==0){
				int val;
				fscanf(fromMe, "%d", &val);
				SetIntParam(string(name), val);
			}
			else if (strcasecmp(type, "double")==0){
				double val;
				fscanf(fromMe, "%le", &val);
				SetDoubleParam(string(name), val);
			}
			else {
				// everything else is considered a string parameter
				// we read from the input until we see $
				// then we have our string parameter
				char buffer[10000]; // to allow for very long descriptions
				int pos=0;
				char c;

				// discard trailing separators
				while ( (c=fgetc(fromMe)) == ' ' || c=='\t'){}
				// c is now the first good character
				while ( c != '$' && pos<10000){
					buffer[pos++]=c;
					c=fgetc(fromMe);
				}
				buffer[pos]='\0'; // end of string
				SetStringParam(string(name), string(buffer));
			}
		}
		else {
		  // we have a regular parameter
		  char buffer[10000]; // to allow for very long descriptions
		  int pos=0;
			char c;
			while ( (c=fgetc(fromMe)) != '$' && pos<10000 ){
				buffer[pos++]=c;
			}
			buffer[pos]='\0'; // end of string
			SetStringParam(query, string(type), string(buffer));
		}
	}
}

void SymbolicWaypointConfig::swap(SymbolicWaypointConfig &who) {
	char *foo = new char[sizeof (SymbolicWaypointConfig)];
	memmove (foo, this, sizeof (SymbolicWaypointConfig));
	memmove (this, &who, sizeof (SymbolicWaypointConfig));
	memmove (&who, foo, sizeof (SymbolicWaypointConfig));
	delete [] foo;
}

int SymbolicWaypointConfig::GetIntParam(string name) {
	// search
	map<string,int>::iterator it = intParams.find(name);

	// check
	FATALIF(it == intParams.end(), "Integer parameter %s not found", name.c_str());

	// ok, return
	return(it->second);
}

double SymbolicWaypointConfig::GetDoubleParam(string name) {
	// search
	map<string,double>::iterator it = doubleParams.find(name);

	// check
	FATALIF(it == doubleParams.end(), "Double parameter %s not found", name.c_str());

	// ok, return
	return(it->second);
}

string SymbolicWaypointConfig::GetStringParam(string name) {
	// search
	map<string,string>::iterator it = stringParams.find(name);

	// check
	FATALIF(it == stringParams.end(), "String parameter %s not found", name.c_str());

	// ok, return
	return(it->second);
}

string SymbolicWaypointConfig::GetStringParam(QueryID query, string name){
	// search
	QueryParameterMap::iterator it = queryParameterMap.find(QueryParameter(query,name));

	// check
	FATALIF(it == queryParameterMap.end(), "String parameter %s not found", name.c_str());

	// ok, return
	return(it->second);
}
