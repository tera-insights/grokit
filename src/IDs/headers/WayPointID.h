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
#ifndef _WAYPOINT_ID_H_
#define _WAYPOINT_ID_H_

#include "Config.h"
#include "IDUnique.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"

#include <map>
#include <string>

// forward definitions
class WayPointInfo;
class WayPointInfoImp;

/** This class implements ids used to identify Waypoints */

class WayPointID : public IDUnique {
private:
    // constructor from int used by WayPointInfoImp
    WayPointID(size_t _id):IDUnique(id){ }

    // map so that we give the same WayPointID to the same string
    static std::map<std::string, size_t> nameToID;

public:
    // constructor that creates a new waypoint. This should be used sparingly
    // this interface should be evolved as more info is asociated with a WayPoint
    // the name is the symbolic name. This is used as a synnonim to the actual id
    WayPointID(std::string& name);
    WayPointID(const char* name);

    // Default constructor
    WayPointID(){ }

    // the info for the id. Just creates the WayPointInfo object
    virtual void getInfo(IDInfo& where) const OVERRIDE_SPEC;

    // need it for virtual functions
    virtual ~WayPointID() {}

    // function to test if an IDUnique reference is actually a WayPointID
    static bool IsThisType(IDUnique& otherId);

    // function to retreive a waypointID by name
    static WayPointID GetIdByName(const char* name);

    // function to get the name
    std::string getName(void) const;

    // To/From JSON
    void toJson( Json::Value & dest ) const;
    void fromJson( const Json::Value & src );

    // friend declarations
    friend class WayPointInfo;
    friend class WayPointInfoImp;
};

// Container for WayPointIDs
typedef TwoWayList<WayPointID> WayPointIDContainer;


/** class to create an Info object from the id

        NOTE: when more information is associated with the waypoint, interfaces to extract
        that information should be added to this class and to the implementation class
 */
class WayPointInfo : public IDInfo {
public:
    // we just need a new constructor
    WayPointInfo(size_t id);

    // destructor
    virtual ~WayPointInfo(){}

    // method to create a WayPointID form the info message
    // used to recreate
    WayPointID getIDObject(void);
};

/** implementation of the Info class */
class WayPointInfoImp : public IDInfoImp {
private:
    size_t id;

    struct Info {
        std::string name;
        Info(std::string& _name):name(_name){}
        Info(){ }
    };

    // the Info class has the maps from id to info
    static std::map<size_t, Info> infoMap;

protected:
    // this method adds/changes information associated with a Waypoint
    // only the WayPointID is allowed to call this function
    // NOTE: interface needs to evolve if more info added
    static void AddInfo(size_t id, std::string& name);

public:
    // we just need a new constructor
    WayPointInfoImp(size_t _id):id(_id){ };

    // destructor so we get virtual functions
    virtual ~WayPointInfoImp(){};

    virtual WayPointID getIDObject(void);
    virtual std::string getIDAsString() const OVERRIDE_SPEC;
    virtual std::string getName() const OVERRIDE_SPEC;


    // friend declarations
    friend class WayPointID;
};

/////////////////
// INLINE METHODS
inline
std::string WayPointID::getName(void) const {
    IDInfo info;
    getInfo(info);
    return info.getName();
}

inline
WayPointID WayPointID::GetIdByName(const char* name){
    std::map<std::string, size_t>::iterator it=nameToID.find(name);
    if (it==nameToID.end()){
        // did not find it. Return default id
      WARNING("Did not find waypoint ID of %s\n", name);
        return WayPointID();
    } else {
        WayPointID rez;
        rez.id = it->second;
        return rez;
    }
}

inline
bool WayPointID::IsThisType(IDUnique& otherId){
    WayPointID wpId;
    return ( typeid(wpId).name() == typeid(otherId).name() );
}

inline
WayPointID::WayPointID(std::string& name){
    std::map<std::string, size_t>::iterator it=nameToID.find(name);
    if (it==nameToID.end()){
        // did not find it, new id
        NewID();
        WayPointInfoImp::AddInfo(id, name);
        nameToID.insert(std::pair<std::string, size_t>(name,id));
    } else {
        // found it
        id = it->second;
    }
}


inline
WayPointID::WayPointID(const char* name){
    std::string tmp(name);

    std::map<std::string, size_t>::iterator it=nameToID.find(tmp);
    if (it==nameToID.end()){
        // did not find it, new id
        NewID();
        WayPointInfoImp::AddInfo(id, tmp);
        nameToID.insert(std::pair<std::string, size_t>(tmp,id));
    } else {
        // found it
        id = it->second;
    }
}


inline
void WayPointID::getInfo(IDInfo& where) const {
    WayPointInfo ret(id);
    where.swap(ret);
}

inline
void WayPointID :: toJson( Json::Value & dest ) const {
    std::string name = getName();
    dest = name;
}

inline
void ToJson( const WayPointID & src, Json::Value & dest ) {
    src.toJson(dest);
}

inline
void WayPointID :: fromJson( const Json::Value & src ) {
    std::string name = src.asString();

    std::map<std::string, size_t>::iterator it=nameToID.find(name);
    if (it==nameToID.end()){
        // Didn't find this name, it refers to a waypoint that doesn't
        // exist in the system. This is bad.

        FATAL("Cannot deserialize WayPointID for waypoint %s: No such waypoint exists.", name.c_str());
    } else {
        // found it
        id = it->second;
    }
}

inline
void FromJson( const Json::Value & src, WayPointID & dest ) {
    dest.fromJson(src);
}

inline
WayPointInfo::WayPointInfo(size_t id){
    info = new WayPointInfoImp(id);
}

inline
WayPointID WayPointInfo::getIDObject(void){
    WayPointInfoImp& aux = dynamic_cast<WayPointInfoImp&>(*info);
    return aux.getIDObject();
}

inline
WayPointID WayPointInfoImp::getIDObject(void){
    WayPointID ret(id);
    return ret;
}

inline
void WayPointInfoImp::AddInfo(size_t id, std::string& name){
    Info obj(name);
    infoMap[id]=obj;
}

#endif // _WAYPOINT_ID_H_
