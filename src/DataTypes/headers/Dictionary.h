#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <string>
#include <unordered_map>
#include <cinttypes>

// This class represents a dictionary that maps integer values to strings.
class Dictionary {
public:
    typedef int32_t         DiffType; // return value of compare function
    typedef uint32_t        IntType; // int is enough, no need for int64
    typedef std::string     StringType;
    typedef std::unordered_map<IntType, IntType>    TranslationTable;

private:
    typedef std::unordered_map<IntType, StringType> IndexMap;
    typedef std::unordered_map<StringType, IntType> ReverseMap;

public:
    typedef IndexMap::const_iterator const_iterator;

private:

    // Mapping from ID to String
    IndexMap indexMap;
    // Mapping from String to ID
    ReverseMap reverseMap;
    // Mapping from ID to index in sorted order
    TranslationTable orderMap;

    // Next ID to be given
    IntType nextID;

    // Whether or not the dictionary has been modified since loading.
    bool modified;

    // Whether or not orderMap is valid
    bool orderValid;

public:

    // Constructor
    Dictionary( void );

    // Construct from dictionary name
    Dictionary( const std::string name );

    // Destructor
    ~Dictionary( void );

    // Look up an ID and get the String
    const char * Dereference( const IntType id ) const;

    // Look up a String and get the ID
    // Return invalid if not found
    IntType Lookup( const char * str, const IntType invalid ) const;

    // Insert a value into the Dictionary and return the ID.
    // Throw an error if the new ID is larger than maxID
    IntType Insert( const char * str, const IntType maxID );

    // Integrate another dictionary into this one, and produce a translation
    // table for any values whose ID has changed.
    void Integrate( Dictionary& other, TranslationTable& trans );

    // load/save dictionary from SQL
    // name specifies which dictionary to load/save
    void Load(const char* name);
    void Save(const char* name);

    // Compare two factors lexicographically.
    // The return value will be as follows:
    //  first > second  : retVal > 0
    //  first = second  : retVal = 0
    //  first < second  : retVal < 0
    DiffType Compare( IntType firstID, IntType secondID ) const;

    const_iterator begin( void) const;
    const_iterator cbegin( void ) const;

    const_iterator end( void ) const;
    const_iterator cend( void ) const;

private:
    // Helper method for reverse lookups
    IntType Lookup( const StringType& str, const IntType invalid ) const;

    // Helper method for inserting strings
    IntType Insert( StringType& str );

    // Helper method to compute the sorted order.
    void ComputeOrder( void );

/* ***** Static Members ***** */
private:
    typedef std::unordered_map<std::string, Dictionary> DictionaryMap;

    // Storage for global dictionaries
    static DictionaryMap dicts;

/* ***** Static Methods ***** */
public:
    static Dictionary & GetDictionary( const std::string name );
};

#endif //_DICTIONARY_H_
