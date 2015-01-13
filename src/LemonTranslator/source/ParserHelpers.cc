#include <string>

#include "ParserHelpers.h"

using namespace std;

// Define storage for attributes
string ParserHelpers::qry = "";
string ParserHelpers::qryShort = "";
int ParserHelpers::tempCounter = 1;
bool ParserHelpers::haveErrors = false;

std::string StripQuotes(std::string str) {
  std::string rez;
  rez = str.substr(1, str.size() - 2);
  return rez;
}
std::string NormalizeQuotes(std::string str) {
  std::string temp;
  temp = str.substr(1, str.size() - 2);
  std::string rez;
  rez += "\"";
  rez += temp;
  rez += "\"";
  return rez;
}

std::string GenerateTemp(const char* pattern){
  char buffer[100];
  sprintf(buffer, pattern, ParserHelpers::tempCounter++);
  std::string rez = buffer;
  return rez;
}

