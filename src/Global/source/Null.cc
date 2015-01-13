#include "Null.h"

// Allocation of state member
const GrokitNull GrokitNull::Value;

bool GrokitNull :: StringIsNull( const char * str ) {
    // The only difference between upper and lowercase letters in ASCII is the
    // 6th bit (which is set for lowercase letters). If we mask this off, we
    // can do case-insensitive comparisons without branches. Since we are only
    // testing the string against a certain alphabetic string, we don't care what
    // this will do to non-alphabetic characters.
    constexpr char UPPER_MASK = 0xDF;

    return (str[0] & UPPER_MASK) == 'N' &&
        (str[1] & UPPER_MASK) == 'U' &&
        (str[2] & UPPER_MASK) == 'L' &&
        (str[3] & UPPER_MASK) == 'L' &&
        str[4] == '\0';
}
