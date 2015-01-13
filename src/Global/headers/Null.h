// Copyright 2014 Tera Insights, LLC. All Rights Reserved.

#ifndef _GROKIT_NULL_H_
#define _GROKIT_NULL_H_

class GrokitNull {
    private:
        // An empty struct can't have a size of 0 in C++, so since the compiler
        // will give it a size, we might as well define a 1 byte member to
        // give it a size ourselves.
        char dummy;

        GrokitNull(void) { }

    public:
        // Singleton
        static const GrokitNull Value;

        static bool StringIsNull( const char * );
};

#endif // _GROKIT_NULL_H_
