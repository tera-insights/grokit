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
#ifndef _PRAGMA_H_
#define _PRAGMA_H_

/*
 * This header file has a set of handy macros for using pragmas.
 */

// Helper macro for executing a pragma inside a macro definition
#define DO_PRAGMA(x) _Pragma (#x)

// Shortcut for the message pragma, which prints a message to the screen
// during compilation if expanded.
#define PRAGMA_MSG(x) DO_PRAGMA(message (x))

#endif // _PRAGMA_H_
