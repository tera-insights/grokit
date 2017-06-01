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
#ifndef _TABLESCAN_ID_H_
#define _TABLESCAN_ID_H_


#include <map>
#include <string>

#include "Config.h"
#include "IDUnique.h"
#include "EventProcessor.h"
#include "TwoWayList.h"
#include "TwoWayList.cc"
#include "InefficientMap.h"
#include "InefficientMap.cc"

// forward definitions
class TableScanInfo;
class TableScanInfoImp;

typedef Keyify<size_t> Size_t_key;

/** This class implements ids of tablescans. These ids are different
  than waypoint ids and should be used only by the disk component */

class TableScanID : public IDUnique {
    private:
        // constructor from int used by TableScanInfoImp
        TableScanID(size_t _id):IDUnique(_id){ }

        static std::map<std::string, size_t> nameToID;

    public:
        // constructor that creates a new waypoint. This should be used sparingly
        // this interface should be evolved as more info is asociated with a TableScan
        // the name is the symbolic name. This is used as a synnonim to the actual id
        TableScanID(std::string& name);
        TableScanID(const char* name);

        // Default constructor
        TableScanID(){ }
        // Copy Constructor
        TableScanID(const TableScanID& other);


        // the info for the id. Just creates the TableScanInfo object
        virtual void getInfo(IDInfo& where) const OVERRIDE_SPEC;

        // copy the content from another object
        void copy(TableScanID& other);

        // need it for virtual functions
        virtual ~TableScanID(){}

        // function to test if an IDUnique reference is actually a WayPointID
        static bool IsThisType(IDUnique& otherId);

        // function to retreive a tablescan by name
        static TableScanID GetIdByName(const char* name);

        // friend declarations
        friend class TableScanInfo;
        friend class TableScanInfoImp;
};

/** class to create an Info object from the id

NOTE: when more information is associated with the waypoint, interfaces to extract
that information should be added to this class and to the implementation class
*/
class TableScanInfo : public IDInfo {
    public:
        // we just need a new constructor
        TableScanInfo(size_t id);

        // the default constructor
        TableScanInfo(){}

        // destructor
        virtual ~TableScanInfo(){}

        // method to create a TableScanID form the info message
        // used to recreate
        TableScanID getIDObject(void);
};

/** implementation of the Info class */
class TableScanInfoImp : public IDInfoImp {
    private:
        size_t id;

        struct Info {
            std::string relName;

            Info(std::string& _name):relName(_name){}
            Info(){ }
            void swap(Info& other){ SWAP_STD(relName, other.relName);}
        };

        // the Info class has the maps from id to info
        static InefficientMap<Size_t_key, Info> infoMap;

    protected:
        // this method adds/changes information associated with a Waypoint
        // only the TableScanID is allowed to call this function
        // NOTE: interface needs to evolve if more info added
        static void AddInfo(size_t id, std::string& name);

    public:
        // we just need a new constructor
        TableScanInfoImp(size_t _id):id(_id){ };

        virtual TableScanID getIDObject(void);
        virtual std::string getIDAsString() const OVERRIDE_SPEC;
        virtual std::string getName() const OVERRIDE_SPEC;


        virtual ~TableScanInfoImp(){};

        // friend declaration
        friend class TableScanID;
};


/////////////////
// INLINE METHODS

inline
TableScanID TableScanID::GetIdByName(const char* name){
    std::map<std::string, size_t>::iterator it=nameToID.find(name);
    if (it==nameToID.end()){
        // did not find it. Return default id
        return TableScanID();
    } else {
        TableScanID rez;
        rez.id = it->second;
        return rez;
    }
}

inline
void TableScanID::copy(TableScanID& swapMe){
    memcpy ((void*) this, (void*) &swapMe, sizeof (TableScanID));
}


inline
bool TableScanID::IsThisType(IDUnique& otherId){
    TableScanID scanId;
    return ( typeid(scanId).name() == typeid(otherId).name() );
}

inline
std::string TableScanInfoImp::getIDAsString() const {
    Size_t_key key(id);
    Info& myInfo=infoMap.Find(key);

    return myInfo.relName;
}

inline
std::string TableScanInfoImp::getName() const {
    Size_t_key key(id);
    Info& myInfo=infoMap.Find(key);

    return myInfo.relName;
}


inline
TableScanID::TableScanID(std::string& _relName){
    std::map<std::string, size_t>::iterator it=nameToID.find(_relName);
    if (it==nameToID.end()){
        // did not find it. Return default id
        NewID();
        TableScanInfoImp::AddInfo(id, _relName);
        nameToID.insert(std::pair<std::string, size_t>(_relName,id));
    } else {
        id = it->second;
    }
}

inline
TableScanID::TableScanID(const char* _relName){
    std::string tmp(_relName);
    std::map<std::string, size_t>::iterator it=nameToID.find(tmp);
    if (it==nameToID.end()){
        // did not find it. Return default id
        NewID();
        TableScanInfoImp::AddInfo(id, tmp);
        nameToID.insert(std::pair<std::string, size_t>(tmp,id));
    } else {
        id = it->second;
    }
}

inline
TableScanID::TableScanID(const TableScanID& other):IDUnique(other.id){}

inline
void TableScanID::getInfo(IDInfo& where) const {
    TableScanInfo ret(id);
    where.swap(ret);
}

inline
TableScanInfo::TableScanInfo(size_t id){
    info = new TableScanInfoImp(id);
}

inline
TableScanID TableScanInfo::getIDObject(void){
    TableScanInfoImp& aux = dynamic_cast<TableScanInfoImp&>(*info);
    return aux.getIDObject();
}

inline
TableScanID TableScanInfoImp::getIDObject(void){
    TableScanID ret(id);
    return ret;
}

inline
void TableScanInfoImp::AddInfo(size_t id, std::string& name){
    Info obj(name);
    Size_t_key key(id);
    infoMap.Insert(key,obj);
}

#endif // _TABLESCAN_ID_H_
