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
#ifndef _FILESCANNER_H_
#define _FILESCANNER_H_

#include "ID.h"
#include "EventProcessor.h"






// include the base class definition
#include "EventProcessor.h"

// include the implementation definition
#include "FileScannerImp.h"

/** Class to provide an interface to FileScannerImp class.

    See FileScannerImp.h for a description of the functions 
    and behavior of the class
*/
class FileScanner : public EventProcessor {
public:

	
  // constructor (creates the implementation object)
  FileScanner(const char * _metadataFile, const char* _scannerName, EventProcessor& _concurencyCorntroller, EventProcessor& _scheduler){
    evProc = new FileScannerImp(_metadataFile, _scannerName, _concurencyCorntroller, _scheduler);
  }

	
  // default constructor
  FileScanner(void){
    evProc = NULL;
  }

	
  TableScanID GetID(void){
    FileScannerImp& obj = dynamic_cast<FileScannerImp&>(*evProc);
    return obj.GetID();
  }


  // the virtual destructor
  virtual ~FileScanner(){}
};


#endif // _FILESCANNER_H_
