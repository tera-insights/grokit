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
#ifndef _MMAP_ALLOCATOR_H_
#define _MMAP_ALLOCATOR_H_

#include "Numa.h"

/** If huge pages are desired, define the variable USE_HUGE_PAGES
		in the make file
*/

/** If the mmap calls are not managed by Datapath(i.e. the mmaps
	* reused), we pay a big price since the pages are Zero-ed by the
	* kernel before being provided to the caller.

	* The allocator provides a level of indirection and hides calls to
	* mmap and munmap behind the functions mmap_alloc and mmap_free

	These functions are implemented for now by using a global
	MMapAllocator object.

	Warning: this allocator works reasonably only for large objects. Do
	not use as a replacement of malloc for small objects. The best use is
	for storing columns that are read from the disk.

 */

// constant to define the page size used by allocator
// the actual size is 2^exponent
// for size 17, the page is 128K (the minimum size we'll use in the system)
// all requests smaller than this are rounded up to this value
#ifdef USE_HUGE_PAGES
#define ALLOC_PAGE_SIZE_EXPONENT 21 /* 2M pages */
#else
#define ALLOC_PAGE_SIZE_EXPONENT 17 /* 128KB pages */
#endif

// function to allocate memory using the mmap subsystem
// this bypasses malloc facility and provides alligned pages that
// are efficient with respect to I/O using dma (zero copy in the kernel)
// the argument is the amount to allocate
// If node is specified, the system tries to allocate memory from a specific numa node
// To avoid unnecessary complications, this is just a preference
extern void* mmap_alloc(size_t noBytes, int node = NUMA_ALL_NODES);

// deallocation function
// the argument is the pointer to the region that was previously allocated
// A fatal error will be emited if the pointer was not previously allocated
extern void mmap_free(void* ptr);

#endif // _MMAP_ALLOCATOR_H_
