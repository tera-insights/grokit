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
#ifndef _FILEWRITER_H_
#define _FILEWRITER_H_

#include "EventProcessor.h"






// include the base class definition
#include "EventProcessor.h"

// include the implementation definition
#include "FileWriterImp.h"

/** Class to provide an interface to FileWriterImp class.

    See FileWriterImp.h for a description of the functions 
    and behavior of the class
*/
class FileWriter : public EventProcessor {
public:

	
  // constructor (creates the implementation object)
  FileWriter(const char * _metadataFile, long unsigned int _noCols){
    evProc = new FileWriterImp(_metadataFile, _noCols);
  }

	
  // constructor (creates the implementation object)
  FileWriter(const char * _metadataFile){
    evProc = new FileWriterImp(_metadataFile);
  }

	
  // default constructor
  FileWriter(void){
    evProc = NULL;
  }


  // the virtual destructor
  virtual ~FileWriter(){}
};


#endif // _FILEWRITER_H_
