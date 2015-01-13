#ifndef _COLUMN_ITERATOR_DICT_H_
#define _COLUMN_ITERATOR_DICT_H_

#include "ColumnIterator.h"
#include "Dictionary.h"
#include "DictionaryManager.h"

/** This class extends ColumnIterator with features needed by
  the fixed sized datatypes that need a dictionary

  This mostly means modified constructors and Done method.

  Same signature as ColumnIterator except Done

  DataType argument must behave like a Factor

*/

template <class DataType>
class ColumnIteratorDict
: public ColumnIterator<DataType>{

    public:
        // creates a column iterator for the given column... the requests for data that
        // are sent to the column are of size stepSize.  iterateMe is consumed.
        ColumnIteratorDict (Column &iterateMe, int stepSize = COLUMN_ITERATOR_STEP);

        // This iterates from [fragmentStart, fragmentEnd]
        ColumnIteratorDict (Column &iterateMe, int fragmentStart, int fragmentEnd, int stepSize = COLUMN_ITERATOR_STEP);

        ColumnIteratorDict ();
        // destructor... if there is a column left in the ColumnIteratorDict, it will be
        // destroyed as well
        ~ColumnIteratorDict (){};

        // new Done interface. When used, the local dictionary must be
        // integrated into the global dictionary
        void Done (Column &iterateMe, Dictionary& localDictionary);
        void Done (Column &iterateMe);
        // this function is strictly needed otherwise we get a deadlock
        // if the same factor is used multiple times by a waypoint
        // all the PreDone calls need to be called before any Done()
        void PreDone(void);

};

template <class DataType>
inline ColumnIteratorDict<DataType>::ColumnIteratorDict(void):ColumnIterator<DataType>(){}


template <class DataType>
inline ColumnIteratorDict<DataType>::ColumnIteratorDict (Column &iterateMe, int stepSize):ColumnIterator<DataType>(iterateMe, stepSize){
    // ensure the dictionary is readable/available
    DictionaryManager::EnsureReadAccess(DataType::DictionaryName,
            Dictionary::GetDictionary(DataType::DictionaryName));
}


template <class DataType>
inline void ColumnIteratorDict <DataType> :: Done (Column &iterateMe){
    // if we do not have a second argument, we never introduced any new
    // factors
    // just release the read lock
    DictionaryManager::ReleaseReadAccess(DataType::DictionaryName);
    ColumnIterator<DataType>::Done(iterateMe);
}

template <class DataType>
inline void ColumnIteratorDict<DataType>::PreDone (void){
    // release read lock
    DictionaryManager::ReleaseReadAccess(DataType::DictionaryName);
}

template <class DataType>
inline void ColumnIteratorDict<DataType>::Done (Column &iterateMe,
        Dictionary& localDictionary){

    FATALIF(!ColumnIterator<DataType>::it.IsWriteOnly(), "This function cannot be used for RO columns");

    // merge local dictionary
    Dictionary::TranslationTable trans;
    DictionaryManager::MergeLocalDictionary(DataType::DictionaryName,
            localDictionary, trans);

    // apply the translation now to the entire column
    ColumnIterator<DataType>::Restart();
    while(!ColumnIterator<DataType>::AtUnwrittenByte()){
        // big hack
        ((DataType*)ColumnIterator<DataType>::it.GetData ())->Translate(trans);
        ColumnIterator<DataType>::Advance();
    }
    ColumnIterator<DataType>::Done(iterateMe);
}


#endif // _COLUMN_ITERATOR_DICT_H_
