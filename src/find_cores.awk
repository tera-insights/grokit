#
#  Copyright 2012 Christopher Dudley
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# This is a simple program designed to determine the total number of physical
# cores present in a system with an x86 architecture.
# The input to this program should be the contents of /proc/cpuinfo
#
# Note: this program could likely be simplified slightly by just adding
# together the value of "cpu cores" for each unique physical ID, but
# this current version just makes absolutely sure by counting the number
# of unique core ids for each physical id.

BEGIN {
    procID = -1;
    numCores = 0;
}

/physical id[[:space:]]+:[[:space:]]+[[:digit:]]+/ {
    procID = $4;
}

/core id[[:space:]]+:[[:space:]]+[[:digit:]]+/ {
    # The core ID should be the 4th field in this record
    coreID = $4;

    # count the number of times we have seen this core of this processor.
    cores[procID, coreID] += 1;

    # If we have a valid processor and we've only seen this core once,
    # increment the core count
    if( procID >= 0 && cores[procID, coreID] == 1 )
        numCores += 1;

    # Reset the processor ID
    procID = -1;
}

END {
    print numCores;
}
