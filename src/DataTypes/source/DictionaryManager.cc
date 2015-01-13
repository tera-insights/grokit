#include "Dictionary.h"
#include "DictionaryManager.h"
#include "Errors.h"

///// Static Initialization /////

pthread_mutex_t DictionaryManager::mutex = PTHREAD_MUTEX_INITIALIZER;

Dictionary DictionaryManager::DictionaryWrapper::NullDict = Dictionary();

DictionaryManager::DictMap DictionaryManager::dictionaryMap = DictionaryManager::DictMap();

void DictionaryManager::EnsureReadAccess(const char* factor,
        Dictionary& dictionary){
    StringType fact(factor);
    bool loaded = true;
    // guard structural change to dictionary
    pthread_mutex_lock(&mutex);
    if (dictionaryMap.find(fact) == dictionaryMap.end()){
        // not in dictionary, add
        dictionaryMap.insert(make_pair(fact,DictionaryWrapper(dictionary)));
        loaded=false;
    }
    DictionaryWrapper& dict = dictionaryMap[fact];
    pthread_mutex_unlock(&mutex);

    if (!loaded){
        dict.WLock();
        dict.GetDictionary().Load(factor);
        dict.UnLock();
    }

    dict.RLock(); // lock for reading
}

void DictionaryManager::ReleaseReadAccess(const char* factor){
    StringType fact(factor);
    pthread_mutex_lock(&mutex);
    FATALIF(dictionaryMap.find(fact) == dictionaryMap.end(),
            "Merging a directory that was not initialized");
    DictionaryWrapper& dict = dictionaryMap[fact];
    pthread_mutex_unlock(&mutex);
    dict.UnLock();
}

void DictionaryManager::MergeLocalDictionary
(const char* factor,
 Dictionary& dictionary,
 Dictionary::TranslationTable& transTable){
    StringType fact(factor);
    pthread_mutex_lock(&mutex);
    FATALIF(dictionaryMap.find(fact) == dictionaryMap.end(),
            "Merging a directory that was not initialized");
    DictionaryWrapper& dict = dictionaryMap[fact];
    pthread_mutex_unlock(&mutex);

    // now we can mess with the integration
    dict.WLock();
    Dictionary& gDict = dict.GetDictionary();
    gDict.Integrate(dictionary, transTable);
    dict.UnLock();
}


void DictionaryManager::Flush(void){
    pthread_mutex_lock(&mutex);
    for( DictMap::iterator el = dictionaryMap.begin(); el != dictionaryMap.end(); ++el ) {
        StringType name = el->first;
        DictionaryWrapper& dict = el->second;
        // need to unlock here to ensure somebody can make progress
        // we could deadlock otherwise (another thread has a WLock)
        pthread_mutex_unlock(&mutex);
        dict.RLock();
        dict.Save(name.c_str());
        dict.UnLock();
        pthread_mutex_lock(&mutex);
    }
    pthread_mutex_unlock(&mutex);
}

