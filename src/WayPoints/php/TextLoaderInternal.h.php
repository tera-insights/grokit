<?php
//
//  Copyright 2013 Tera Insights LLC
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
?>
<?php
require_once('DataFunctions.php');
?>



#ifndef _BULK_LOADER_INTERNAL_H_
#define _BULK_LOADER_INTERNAL_H_

#include "ContainerTypes.h"

/* Data types used by Bulk Loader to do its internal work. Should be
of no interest to the rest of the system */

<?php
grokit\create_data_type( "TextLoaderDS", "Data", [ 'stream' => 'FILE*', 'file' => 'std::string', ], [ ] );
?>


// container for the above
typedef TwoWayList<TextLoaderDS> TextLoaderDSContainer;

#endif // _BULK_LOADER_INTERNAL_H_
