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

/* This file is generated from the corresponding .h.m4 file (look in m4/ directory)

   ANY MODIFICATIONS ARE WIPED OUT NEXT TIME maker RUNS OR CHANGES ARE MADE TO .h.m4 file. 
	 BETTER NOT TO TOUCH -- looking is encouraged, though
*/




// This file contains properties used in the system

#ifndef PROPERTIES_H_
#define PROPERTIES_H_

#include "Data.h"

// This property is used by the cleaner and the joins to ensure 
// that the joins do not update the hash while the cleaner needs to run




// forward definition
class CleanerStatus;

/** Defining the implementation class first */
class CleanerStatusImp : public DataImp {
protected:
  //members

  bool busy;

public:
  //constructor
  CleanerStatusImp(bool const & _busy ):DataImp( ),
    // copy constructed members
    busy(_busy)
  {
    // swap members
  }



  //destructor
  virtual ~CleanerStatusImp() {}

  // type
  virtual const off_t Type(void) const { return 0xc7da86a63027cec5ULL
 ;}
	virtual const char* TypeName(void) const { return "CleanerStatus"; }

	friend class CleanerStatus;

};

/* The front end class CleanerStatus now */

class CleanerStatus : public Data {
public:
  // the type
  static const off_t type=0xc7da86a63027cec5ULL
 ;

  //constructor
  CleanerStatus(bool const & _busy){
	  data = new CleanerStatusImp(_busy);
  }

  // default constructor
	CleanerStatus():Data(){}


  // access methods for all new data
  bool& get_busy(){ 
    CleanerStatusImp* myData = dynamic_cast<CleanerStatusImp*>(data);
    FATALIF(myData == NULL, "Trying to get member busy of an invalid or wrong type object");
    return myData->busy; 
  }

	// Test if the object is what it claims to be
	bool IsValid(){
		return (dynamic_cast<CleanerStatusImp*>(data) != NULL);
  }	

};



#endif 
