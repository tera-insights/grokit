#ifndef _DICTIONARYS_MANAGER_H_
#define _DICTIONARYS_MANAGER_H_

#include <pthread.h>
#include <unordered_map>
#include "Dictionary.h"

/** Class to manage the dictionaries for Factors. This mechanism uses
    Dictionary class in Dictionary.h and it is generic.
 */

class DictionaryManager {
  // keep track of global dictionaries
  // these dictionaries correspond to static members of Factors
  // they are never allocated
  class DictionaryWrapper{
    // This is so we have something to initialize dictionary to for the default
    // constructor
    static Dictionary NullDict;

    Dictionary& dictionary;
    pthread_rwlock_t lock; // rw-lock to access the dictionary

    public:
    DictionaryWrapper() : dictionary(NullDict) {
    }

    DictionaryWrapper(Dictionary&_dictionary):
      dictionary(_dictionary){
      pthread_rwlock_init(&lock, NULL);
    }

    Dictionary& GetDictionary(){ return dictionary; }
    void RLock(){pthread_rwlock_rdlock(&lock);}
    void WLock(){pthread_rwlock_wrlock(&lock);}
    void UnLock(){pthread_rwlock_unlock(&lock);}

    void Save(const char * str) { dictionary.Save(str); }
    void Load(const char * str) { dictionary.Load(str); }

    ~DictionaryWrapper(){ pthread_rwlock_destroy(&lock);}
  };
    typedef std::string StringType;
    typedef std::unordered_map<StringType, DictionaryWrapper> DictMap;

    static pthread_mutex_t mutex; // global mutex to perform operations on manager
    static DictMap dictionaryMap; // map of all dictionaries indexed by hash of Factor name

 public:
  /* Usage Scenario:
     EnsureReadAccess("factor1", gDictFact1);
     use gDictFact1 in read only mode
     optionaly create localDictionary
     ReleaseReadAccess("factor1");

     if (localDictCreated){
       Translation trans;
       MergeLocalDictionary("factor1", localDictionary);
       applyTranslation;
     }

   */


  // Method to grant read access to a dictionary
  // if dictionary not registered, it will do so and load from
  // the disk
  // Acquires a read lock as well
  static void EnsureReadAccess(const char* factor,
			       Dictionary& dictionary);

  // release read lock
  static void ReleaseReadAccess(const char* factor);

  // Method to add info from local dictionary to global dictionary for a
  // factor. transTable is filled with translation information
  static void MergeLocalDictionary
    (const char* factor,
     Dictionary& dictionary,
     Dictionary::TranslationTable& transTable);

  // Flush all dictionaries to the disk
  static void Flush(void);
};

#endif // _DICTIONARYS_MANAGER_H_
