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
#ifndef _DISKARRAY_H_
#define _DISKARRAY_H_

#include "EventProcessor.h"
#include "DiskArrayImp.h"
#include "PageConversions.h"
#include "Errors.h"

#include <sqlite3.h>


/** Interface class for the DiskArrayImp. This class is a little
  special since it ensure a single instance in the system. This is
  important to minimize the interaction with the OS.

  The interface mimics the interface of DiskArrayImp. The object
  itself is stand-alone so it can be moved around

NOTE: one of the static functions to load or create an array has
to be called before any object can be instanced.
*/
class DiskArray : public EventProcessor {
    protected:
        // we inherit from EventProcessor so the object is evProc
        // we maintain a global single instance and set evProc to it in the constructor
        static DiskArrayImp* array; // the only instance in the system
        // one interface to it
        static DiskArray oneInstance;

        /** default constructor; This should prevent any manipulation of the
          object other than direct access to oneInstance */
        DiskArray(){ array=NULL; }

    public:
        //destructor
        virtual ~DiskArray() {
            evProc = NULL;

            //kill the array
            if (array != NULL)
                array->Seppuku();
            delete array;
        }

        /** global function to return the system disk array,
          a handle to the disk array is returned

          The function asumes that the metadata is in "Disks.meta"
          */
        static DiskArray& GetDiskArray();


        /** Function to load an existing disk array */
        static void LoadDiskArray(bool isReadOnly=false);

        // method to allocate pages. Behaves atomically. Should be called to
        // determine where to write info.
        off_t AllocatePages(off_t _noPages, int relID);

        // method to delete all content of a relation
        void DeleteRelationSpace(int relID);

        // function to force the array to writte metadata on disk
        // should be done at the end of each bulk load (pointless before sice we want the whole bulkload to succeed)
        void Flush(void){ array->Flush();}

        // update within another action (so that we can run a single transactin)
        void Flush(sqlite3* db){ array->Flush(db); }

        // arrayID access
        int getArrayID(void){ return array->getArrayID(); }

        // ask about the amount of IO
        off_t NumPagesProcessed(void);
        off_t NumPagesDelta(void); // since last call
};

// INLINE methods
inline off_t DiskArray::NumPagesProcessed(void){
    return array->NumPagesProcessed();
}

inline off_t DiskArray::NumPagesDelta(void){
    return array->NumPagesDelta();
}

inline void DiskArray::DeleteRelationSpace(int relID){
    array->DeleteRelationSpace(relID);
}

inline off_t DiskArray::AllocatePages(off_t _noPages, int relID){
    return array->AllocatePages(_noPages,relID);
}

inline DiskArray& DiskArray::GetDiskArray(){
    return oneInstance;
}

inline void DiskArray::LoadDiskArray(bool isReadOnly){
    FATALIF( array != NULL, "Array initialization can be done only once" );
    array = new DiskArrayImp(isReadOnly);

    oneInstance.evProc = array;

    // starting the disk array
    array->ForkAndSpin();
}

#endif // _DISKARRAY_H_


