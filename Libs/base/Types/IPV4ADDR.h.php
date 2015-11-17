<?
//
//  Copyright 2012 Alin Dobra and Christopher Jermaine, Apache 2.0
//  Copyright 2013, Tera Insights. All rights reserved

function IPV4ADDR(){
    $constructors = [];
    $methods = [];
    $functions = [];

    $globalContent = "";
?>

/** This type implements an efficient IP v4 address
        Internal representation is an int (for efficiency).
*/


class IPv4;

bool operator < (const IPv4 &d1, const IPv4 &d2);
bool operator <= (const IPv4 &d1, const IPv4 &d2);
bool operator > (const IPv4 &d1, const IPv4 &d2);
bool operator >= (const IPv4 &d1, const IPv4 &d2);
bool operator == (const IPv4 &d1, const IPv4 &d2);
bool operator != (const IPv4 &d1, const IPv4 &d2);

// function to extract the domain (class C)
IPv4 Domain(IPv4 x);

class IPv4 {
private:
    union addr_rep {
        unsigned int asInt;
        struct {
            unsigned char c1;
            unsigned char c2;
            unsigned char c3;
            unsigned char c4;
        } split;
    };
    addr_rep addr;

public:
<?  $constructors[] = [[], true];  ?>
    // Default constructor
    IPv4(void){
        addr.asInt = 0;
    }

<?  $constructors[] = [['BASE::STRING_LITERAL'], true];  ?>
    /* Constructor from string "xxx.xxx.xxx.xxx". The format is fixed */
    IPv4 (const char *_addr){
        FromString(_addr);
    }

    // constructor from integers
<?  $constructors[] = [['BASE::BYTE', 'BASE::BYTE', 'BASE::BYTE', 'BASE::BYTE'], true];  ?>
    IPv4(char c1, char c2, char c3, char c4){
        addr.split.c1 = c1;
        addr.split.c2 = c2;
        addr.split.c3 = c3;
        addr.split.c4 = c4;
        }

<?  $constructors[] = [['BASE::NULL'], true];  ?>
    IPv4( const GrokitNull & nullval ) {
        addr.asInt = 0;
    }

    void FromString(const char *_addr) {
        unsigned int c1;
        unsigned int c2;
        unsigned int c3;
        unsigned int c4;
        sscanf(_addr, "%u.%u.%u.%u", &c1, &c2, &c3, &c4);

        addr.split.c1 = c1;
        addr.split.c2 = c2;
        addr.split.c3 = c3;
        addr.split.c4 = c4;
    }

    int ToString(char* text) const{
        return 1+sprintf(text,"%u.%u.%u.%u",
                                            (unsigned int) addr.split.c1,
                                            (unsigned int) addr.split.c2,
                                            (unsigned int) addr.split.c3,
                                            (unsigned int) addr.split.c4);
    }

    void Print(void){ printf("%u.%u.%u.%u",
                                                     (unsigned int) addr.split.c1,
                                                     (unsigned int) addr.split.c2,
                                                     (unsigned int) addr.split.c3,
                                                     (unsigned int) addr.split.c4);
    }

<?  $methods[] = ['IsValid', [], 'BASE::BOOL', true];  ?>
    bool IsValid(void) const {
        return addr.asInt != 0;
    }

<?  $methods[] = ['IsNull', [], 'BASE::BOOL', true];  ?>
    bool IsNull(void) const {
        return addr.asInt == 0;
    }

    uint32_t asInt(void) const {
        return addr.asInt;
    }

    /* operators */
    friend bool operator < (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt<d2.addr.asInt);
    }

    friend bool operator <= (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt<=d2.addr.asInt);
    }

    friend bool operator > (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt>d2.addr.asInt);
    }

    friend bool operator >= (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt>=d2.addr.asInt);
    }

    friend bool operator == (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt==d2.addr.asInt);
    }

    friend bool operator != (const IPv4 &d1, const IPv4 &d2) {
        return (d1.addr.asInt!=d2.addr.asInt);
    }

    IPv4& operator =( const IPv4& other ) {
        addr.asInt = other.addr.asInt;
        return *this;
    }

<?  $functions[] = ['Domain', ['@type'], '@type', true ]; ?>
    friend IPv4 Domain(IPv4 x){
        IPv4 rez=x;
        rez.addr.split.c4=0;
        return rez;
    }

    static bool Between (const IPv4 &d, const IPv4 &dl, const IPv4 &dr) {
        return (d.addr.asInt >= dl.addr.asInt && d.addr.asInt <= dr.addr.asInt); //>
    }
};

// compatibility with the other type definitions
typedef IPv4 IPV4ADDR;

<?  ob_start(); ?>

inline void FromString( @type & x, const char* text){
    x.FromString(text);
}

inline int ToString(const @type& x, char* text){
    return x.ToString(text);
}

inline
void ToJson( const @type & x, Json::Value & dest ) {
    char buffer[20];
    ToString(x, buffer);
    dest = buffer;
}

inline
void FromJson( const Json::Value & src, @type & dest ) {
    FromString(dest, src.asCString());
}

// hash function, just return the unsigned int inside
<?  $functions[] = ['Hash', ['@type'], 'BASE::BIGINT', true, true ]; ?>
inline uint64_t Hash(const @type & d){
    return d.asInt();
}

// Deep copy
inline
void Copy( @type& to, const @type& from ) {
    to = from;
}

#ifdef _HAS_STD_HASH
#include <functional>
// C++11 STL-compliant hash struct specialization

namespace std {

template <>
class hash<@type> {
public:
    size_t operator () (const @type& key) const {
        return Hash(key);
    }
};

}
#endif // _HAS_STD_HASH

<?  $functions[] = ['IsNull', ['@type'], 'BASE::BOOL', true, true ]; ?>
inline
bool IsNull( const @type & d ) {
    return d.IsNull();
}

<?  $globalContent .= ob_get_clean(); ?>

<?

return array(
    'kind'             => 'TYPE',
    'constructors'     => $constructors,
    'methods'          => $methods,
    'functions'        => $functions,
    'global_content'   => $globalContent,
    "user_headers"     => array ("Config.h"),
    "system_headers"   => array ( "stdlib.h", "stdio.h" ),
    "complex"          => false,
    'binary_operators' => [ '==', '!=', '>', '<', '>=', '<=' ],
    'describe_json'    => DescribeJson('ipv4addr'),
    'extras'           => [ 'size.bytes' => 4 ],
    );

} // end of function



?>

