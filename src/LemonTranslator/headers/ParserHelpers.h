// Copyright 2013 Tera Insights, LLC. All Rights Reserved.

#ifndef _PARSER_HELPERS_H_
#define _PARSER_HELPERS_H_

// A couple definitions that need to be defined across various parts of the parser.

// Clang has weird issues with extern variables
struct ParserHelpers {
    static std::string qry;
    static std::string qryShort;
    static int tempCounter;
    static bool haveErrors;
};

// Some useful functions
std::string StripQuotes(std::string str);
std::string NormalizeQuotes(std::string str);
std::string GenerateTemp(const char* pattern);


#endif // _PARSER_HELPERS_H_
