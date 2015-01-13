//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
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
#ifndef _EXTERNAL_COMMANDS_H
#define _EXTERNAL_COMMANDS_H

/** header containing functions to run external commands
    
 */

// execute external command
int execute_command(const char* command);


// function to initialieze the external commands capability
void external_commands_init(void);

#endif // _EXTERNAL_COMMANDS_H
