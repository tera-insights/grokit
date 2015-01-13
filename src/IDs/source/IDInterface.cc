// Copyright 2013 Tera Insights, LLC. All Rights Reserved

#include "IDInterface.h"

IDInfo::IDInfo(){ info=NULL; }

IDInfo::~IDInfo(){ if (info!=NULL) delete info; }

std::string IDInfo::getIDAsString() const {
    if (info!=NULL)
        return info->getIDAsString();
    else
        return std::string("IDInfo--Uninitialized");
}

std::string IDInfo::getName() const {
    if (info!=NULL)
        return info->getName();
    else
        return std::string("IDInfo--Uninitialized");
}

std::string IDInfo::getTypeName() {
    if (info!=NULL)
        return typeid(*info).name();
    else
        return std::string("IDInfo--Uninitialized");
}

void IDInfo::swap(IDInfo& other){
    SWAP_ASSIGN(info, other.info);
}

std::string IDInfoImp::getIDAsString() const {
    return std::string("IDInfoImp--Uninitialized");
}

std::string IDInfoImp::getName() const {
    return std::string("IDInfoImp--Uninitialized");
}

IDInfoImp::~IDInfoImp(){}
