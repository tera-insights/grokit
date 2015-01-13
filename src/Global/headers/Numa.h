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
/**
	The header provides numa support for the system

	It is designed to gratiously default to good behavior if numa is
	not supported.
*/

#ifndef _NUMA_DATAPATH_H_
#define _NUMA_DATAPATH_H_

// Constants
// no node preference in NUMA node pinning
#define NUMA_ALL_NODES -1

// Interface functions
// number of numa nodes
int numaNodeCount(void);

// on what node are we running now
int numaCurrentNode(void);


#ifdef USE_NUMA

#define MAX_CPU_SETS 10 // the total number of cpus is MAX_CPU_SETS*64

// the right headers
#define _GNU_SOURCE
#include <sched.h>
#include <numaif.h>
#include <numa.h>

inline int numaNodeCount(void){ return numa_max_node() ; }
inline int int numaCurrentNode(void){ return numa_preferred(); }

#else // no NUMA

// default behavior to have a uniform treatment
inline int numaNodeCount(void){ return 1; }
inline int numaCurrentNode(void){ return 0; }

#endif // USE_NUMA

#endif // _NUMA_DATAPATH_H_
