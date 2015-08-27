//
//  Copyright 2012 Christopher Dudley
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#ifndef _ARG_FORMAT_H
#define _ARG_FORMAT_H

#include <string>

/**
   This class is used to transport information about function arguments between
   the DataTypeManager (which determines the actual types of the function being
   called and performs necessary type conversions) and the expression list in the parser
   (which knows the actual values of the arguments and is responsible for
   populating the constants string).
 */

class ArgFormat {
  std::string type;

  // Wrapper contains a %s where the origional expression is supposed to appear.
  std::string wrapper;

  // Whether or not we are allowed to extract this argument as a constant
  bool noExtract;

 public:
  /**
     Creates a new argument format with the specified type, expression wrapping the
     argument, and whether or not constant extraction is explicitly disallowed for
     this argument.
   */
  ArgFormat( const std::string _type, const std::string _wrapper, bool _noExtract );

  /**
     Copy constructor, creates an exact copy of the argument.
   */
  ArgFormat( const ArgFormat & );

  /**
     Destructor.
   */
  ~ArgFormat();

  /**
     This method takes the name of the argument and properly formats it with any
     wrapping functions required to perform the correct type conversions.
   */
  std::string format( const std::string & name ) const;

  /**
     Returns the type of the argument.
   */
  std::string getType() const;

  /**
     Returns whether or not we should disallow extracting this argument as a constant.
   */
  bool forceNoExtract() const;
};

#endif
