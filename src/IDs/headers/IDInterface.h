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
#ifndef _ID_INTERFACE_H_
#define _ID_INTERFACE_H_

#include <string>
#include <typeinfo>
#include <limits.h>
#include <iostream>

#include "Swap.h"

#define ID_UNINITIALIZED SSIZE_MAX

/** This header defines an abstract class specifying the interface for IDs
  This class does not implement anything, it is a pure interface specification

  Here are the assumptions made when specifying this interface:
  1. The IDs will be placed into containers but not heterogeneous containers.
  For example, you can have a container of WaypointsID but not a mixed container
  of WaypointIDs and QueryIDs.

  2. All ID behave like a small identifier and they can be copied easily around
  This means that all copy constructors and = operator are allowed and are efficient

  3. The IDs have to be efficient. Most of the time, information is not requested about
  them (this can be a little inefficient) but the usage as an index in a map has
  to be efficient

*/


// forward definition of IDInfo so we can first specify the IDInterface class
class IDInfo;

/** The interrace class */

class IDInterface {
    public:

        /** Return detailed info about an ID.
          The information returned is a derived class from IDInfo.
          The actual content is ID specific. Whoever is supposed to
          read the content should know how to convert the IDInfo&
          into the correct type.

          The IDInfo is placed in the object IDInfo and the old contend is distroyed.
          This adheres to the swapping paradigm principles
          */
        void getInfo(IDInfo& where){};

        /** The following functions have to be implemented as well:

          operator==:
          operator< : if the ID will be used with STL sets,maps, etc.

          operator const int():
          Constructor from int: if the ID is used as index in arrays

          copy constructor:
          operator= : in order to allow STL containers to be used and easy propagation of IDs

          swap(ID&):
          Default constructor: so tha the swapping paradigm can be used to manipulate them
          Only the same type of ID should be swappable

          The signatures of the above mehods depend on the ID so they
          cannot be specified here.

*/
};


// foreward definition of IDInfoImp so we can define IDInfo first
class IDInfoImp;

/** The base class for all info objects.

  This object defines the interface common to all the Info
  objects. If the interface is extended, the exension has to inherit
  from this class. The implementation inherits separately from IDInfoImp.


NOTE: If part of the functionality of derived classes is to recreate the ID,
the mehod should have the signature:

ID getIDObject(void);

*/

class IDInfo{
    protected:
        IDInfoImp* info; // the pointer to the implementation of the info object

        // private copy constructor and operator= to avoid weird behavior
        // when these objects are manipulated
        IDInfo(const IDInfo&);
        IDInfo& operator=(const IDInfo&);

    public:
        /** Default constructor so we can use the swapping paradigm */
        IDInfo();

        /** Destructor. This needs not be virtual since any extension of this class
          can work with info of the IDInfoImp* type. For the same reason, there is no
          need for virtual methods in this hierarchy.
          */
        virtual ~IDInfo();

        /** Each object info can only be created by the derived class that
          correspond to that implementation. No other constructor should
          be provided in the base class.
          */

        /** Prints the ID in a string.
          Useful for the  code generator.

          If the id is synonimous with an int, this is just the int. For
          all other ids, it should be a string identifying the
          typefollowed by the value of the id.
          */
        std::string getIDAsString() const;

        /** get the name of the id.
          Useful for code generator and symbolic representations

          This gets a complete name for the ID. This is the symbolic
          representation of the id. If the id was created from sybmolic
          data, this function should return the symbolic name. If no
          symbolic name was provided at creation, the string returned by
          getString() can be used.
          */
        std::string getName() const;

        /** This function returns the name of the class the pointer info has.
          This is accomplished using the typeid facility in the newer C++ standard.

          This can be used, for example to determine the class the object
          can be upconverted to to exract all the information.
          */
        std::string getTypeName();

        /** swap function for the swapping paradigm */
        void swap(IDInfo& other);

        /** No copy is provided for now since it does not seem
          useful. Should that change, a refference count needs to be
          implemented. The ID can produce as many IDInfo objects as it
          wants, so if multiple copies are needed, we can just ask the ID
          to produce another one.

          For specific IDs, the IDInfo might be able to recreate the
          id. This is ID specific and cannot be made generic.
          */

};


/** base class for all implementations of info objects

  For various reasons, the implementation of this class can be very
  different for similar types. For this reason, no implementation
  other than error messages is provided in the base class.

  Any functionality that can be useful can be implemented since the
  interface can evolve as well.

*/

class IDInfoImp {
    public:
        virtual std::string getIDAsString() const;
        virtual std::string getName() const;

        // the destructor has to be virtual to ensure that the whole hierarchy is virtual
        virtual ~IDInfoImp();
};

/////////////////////////////
// INLINE FUNCTIONS


#endif // _ID_INTERFACE_H_
