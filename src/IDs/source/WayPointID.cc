// Copyright 2013 Tera Insights, LLC. All Rights Reserved

#include "WayPointID.h"

std::string WayPointInfoImp::getIDAsString() const {
    Info myInfo=infoMap[id];
    return myInfo.name;
}

std::string WayPointInfoImp::getName() const {
    Info myInfo=infoMap[id];
    return myInfo.name;
}

