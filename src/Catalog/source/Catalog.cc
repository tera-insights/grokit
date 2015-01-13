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
#include "Catalog.h"
#include "SerializeJson.h"
#include "Catalog-MSG.h"
#include "CommunicationFramework.h"

using namespace std;

// instance default value
Catalog *Catalog::instance = NULL;


Catalog &Catalog::GetCatalog(void) {
    // check if instance already exists
    if (instance == NULL)
        instance = new Catalog();

    // return it
    return(*instance);
}

int Catalog::GetIntegerConstant(string varName) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, int>::iterator it = intConstants.find(varName);

    // die if not there
    FATALIF(it == intConstants.end(), "variable %s not found", varName.c_str());

    int val = it->second;

    // return
    return(val);
}

int Catalog::GetIntegerConstantWithDefault(string varName, int deflt) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, int>::iterator it = intConstants.find(varName);

    int val = -1;

    if (it == intConstants.end()){ // not found use and set default
        val = deflt;
        SetIntegerConstant(varName, deflt);
    } else {
        val = it->second;
    }

    // return
    return(val);
}


void Catalog::SetIntegerConstant(string varName, int value) {

    // lock up
    scoped_lock_t guard(constMutex);

    // set the value
    intConstants[varName] = value;

}

double Catalog::GetDoubleConstantWithDefault(string varName, double deflt) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, double>::iterator it = doubleConstants.find(varName);

    double val = -1;

    if (it == doubleConstants.end()){ // not found use and set default
        val = deflt;
        SetDoubleConstant(varName, deflt);
    } else {
        val = it->second;
    }

    // return
    return(val);
}

double Catalog::GetDoubleConstant(string varName) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, double>::iterator it = doubleConstants.find(varName);

    // die if not there
    FATALIF(it == doubleConstants.end(), "variable %s not found", varName.c_str());

    double val = it->second;

    // return
    return(val);
}

void Catalog::SetDoubleConstant(string varName, double value) {

    // lock up
    scoped_lock_t guard(constMutex);

    // set the value
    doubleConstants[varName] = value;

}

string Catalog::GetStringConstantWithDefault(string varName, string deflt) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, string>::iterator it = stringConstants.find(varName);

    string val;

    if (it == stringConstants.end()){ // not found use and set default
        val = deflt;
        SetStringConstant(varName, deflt);
    } else {
        val = it->second;
    }

    // return
    return(val);
}

string Catalog::GetStringConstant(string varName) {

    // lock up
    scoped_lock_t guard(constMutex);

    // get the value
    map<string, string>::iterator it = stringConstants.find(varName);

    // die if not there
    FATALIF(it == stringConstants.end(), "variable %s not found", varName.c_str());

    string val = it->second;

    // return
    return(val);
}

void Catalog::SetStringConstant(string varName, string value) {

    // lock up
    scoped_lock_t guard(constMutex);

    // set the value
    stringConstants[varName] = value;

}

int Catalog::GetNumDisks() {

    // lock up
    scoped_lock_t diskGuard(diskMutex);

    // get the size
    int val = diskPaths.size();

    return(val);
}

string Catalog::GetDiskPath(unsigned int whichDisk) {

    // lock up
    scoped_lock_t diskGuard(diskMutex);

    // check index and get
    FATALIF(whichDisk >= diskPaths.size(), "index out of bounds");
    string val = diskPaths[whichDisk];

    return(val);
}

void Catalog::SetDiskPaths(StringContainer disks) {

    // lock up
    scoped_lock_t diskGuard(diskMutex);

    // set
    diskPaths = disks;
}

bool Catalog::FindSchemaByName(string name) {

    bool found = false;

    // go to the beginning
    schemas.MoveToStart();

    // keep traversing until found/end
    while (!found && !schemas.AtEnd()) {
        // did we find it?
        if (schemas.Current().GetRelationName() == name)
            found = true;
        else
            // no? advance
            schemas.Advance();
    }

    return(found);
}

StringContainer Catalog::GetRelationNames() {

    // lock up
    scoped_lock_t schemaGuard(schemaMutex);

    // create our string container
    StringContainer val;

    // go through each element, adding names
    for (schemas.MoveToStart(); !schemas.AtEnd(); schemas.Advance()) {
        val.push_back(schemas.Current().GetRelationName());
    }

    WARNINGIF(val.size() == 0, "no schemas in catalog");

    return(val);
}

bool Catalog::GetSchema(string relName, Schema &outSchema) {

    // lock up
    scoped_lock_t schemaGuard(schemaMutex);

    // first, check if that schema has an alias floating around
    map<string, string>::iterator it = tableAliases.find(outSchema.GetRelationName());

    // if so, then set the name to the actual real name
    if (it != tableAliases.end()) {
        outSchema.SetRelationName(it->second);
    }

    // find the schema
    bool found = FindSchemaByName(relName);

    if (found)
        schemas.Current().CopyTo(outSchema);

    return(found);
}

bool Catalog::GetSchema(Schema &outSchema) {

    // lock up
    scoped_lock_t schemaGuard(schemaMutex);

    // first, check if that schema has an alias floating around
    map<string, string>::iterator it = tableAliases.find(outSchema.GetRelationName());

    // if so, then set the name to the actual real name
    if (it != tableAliases.end()) {
        outSchema.SetRelationName(it->second);
    }

    // find the schema
    bool found = FindSchemaByName(outSchema.GetRelationName());

    // check
    if (found)
        schemas.Current().CopyTo(outSchema);

    // return
    return(found);
}

void Catalog::AddSchema(Schema &inSchema) {
    {
        // lock up
        scoped_lock_t schemaGuard(schemaMutex);

        cout << "Adding schema for relation " << inSchema.GetRelationName() << endl;

        // find the schema
        bool found = FindSchemaByName(inSchema.GetRelationName());

        // if found, then swap it in
        if (found)
            inSchema.swap(schemas.Current());
        else
            // otherwise, insert
            schemas.Insert(inSchema);
    }

    SaveCatalog();

    SendUpdate();
}

void Catalog::RemoveSchema(std::string name) {
    {
        scoped_lock_t schemaGuard(schemaMutex);

        cout << "Deleting schema for relation " << name << endl;

        bool found = FindSchemaByName(name);

        if(found) {
            Schema temp;
            schemas.Remove(temp);
        } else {
            cerr << "Attempted to delete non-existent relation " << name << endl;
        }
    }

    SaveCatalog();

    SendUpdate();
}

void Catalog::AddSchemaAlias(string alias, string table) {

    // lock
    scoped_lock_t schemaGuard(schemaMutex);

    // add the alias
    tableAliases[alias] = table;
}

void Catalog :: toJson(Json::Value & dest) const {
    // Need to lock all the mutexes, so create locks for all of them but
    // defer actually locking for now.
    // The locks will automatically unlock the mutexes when they go out
    // of scope.
    unique_lock_t diskLock(diskMutex, std::defer_lock);
    unique_lock_t constLock(constMutex, std::defer_lock);
    unique_lock_t schemaLock(schemaMutex, std::defer_lock);

    // Now lock them all at once.
    std::lock(diskLock, constLock, schemaLock);

    // Only bother saving the schemas.
    ToJson(schemas, dest);
}

void Catalog :: fromJson(const Json::Value & src) {
    unique_lock_t diskLock(diskMutex, std::defer_lock);
    unique_lock_t constLock(constMutex, std::defer_lock);
    unique_lock_t schemaLock(schemaMutex, std::defer_lock);

    // Now lock them all at once.
    std::lock(diskLock, constLock, schemaLock);

    // We only bothered to store the schemas
    FromJson(src, schemas);
}

void ToJson(const Catalog & src, Json::Value & dest ) {
    src.toJson(dest);
}

void FromJson(const Json::Value & src, Catalog & dest ) {
    dest.fromJson(src);
}

void Catalog :: SendUpdate(void) {
    // Get proxy actor for the update logger
    HostAddress frontend;
    GetFrontendAddress(frontend);
    MailboxAddress loggerAddr(frontend, "schema");
    EventProcessor proxy;
    FindRemoteEventProcessor(loggerAddr, proxy);

    Json::Value val;
    toJson(val);

    CatalogUpdateMessage::Factory(proxy, val);
}

Catalog::~Catalog() {
}
