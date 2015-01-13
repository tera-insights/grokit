#include "PatternMatcherOnig.h"

#include <cstring>

// Static member initiallization
PatternMatcherOnig::Mutex PatternMatcherOnig::mutex;

PatternMatcherOnig::PatternMatcherOnig( const char * regexp ) {
    int r;
    UChar * pattern = (UChar *) regexp;
    OnigErrorInfo einfo;

    {
        ScopedLock guard(mutex);
        r = onig_new(&reg, pattern, pattern + strlen(regexp),
                ONIG_OPTION_DEFAULT, ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, &einfo);
    }

    if( r != ONIG_NORMAL ) {
        char s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str((UChar*) s, r, &einfo);
        FATAL( "Onig pattern matcher error: %s\n", s);
    }
}

bool PatternMatcherOnig :: Match( const char * target ) const {
    int size = strlen(target);

    int r = onig_match(
        reg,
        (UChar *) target,
        (UChar *) target + size,
        (UChar *) target,
        NULL,
        ONIG_OPTION_NONE
    );

    return r >= 0;
}

PatternMatcherOnig :: ~PatternMatcherOnig(void) {
    onig_free(reg);
}
