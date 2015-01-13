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

#include <sys/mman.h>
#include "Numa.h"

/** Macros defined for low level allocations:

    SYS_PAGE_SIZES: the size  of a system page in bytes
    SYS_MMAP_ALLOC(size): allocte size bytes from the system  and return pointer
    SYS_MMAP_FREE(pointer): free the memory
    SYS_MMAP_CHECK(pointer): test if the ALLOC call was successfull

    WARNING: DO NOT USE THE mmap SYSCALL DIRECTLY

    TODO: Add API for NUMA pinning
*/

/** If huge pages are desired, define the variable USE_HUGE_PAGES
		in the make file
*/

#ifdef USE_HUGE_PAGES
//  #ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
//	#endif
#define MAP_FLAGS (MAP_PRIVATE | MAP_ANON | MAP_HUGETLB)
#define SYS_PAGE_SIZE (1ULL<<21) // 2M PAGES
#else
#define MAP_FLAGS (MAP_PRIVATE | MAP_ANON)
#define SYS_PAGE_SIZE (1ULL<<12) // 4K PAGES
#endif

#define SYS_MMAP_ALLOC(size)			\
  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_FLAGS, 0, 0)
#define SYS_MMAP_CHECK(pointer)			\
  (pointer != MAP_FAILED)
#define SYS_MMAP_FREE(pointer, size)			\
  munmap(pointer, size)

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
#define ALLOC_PAGE_SIZE_EXPONENT 19 /* 512KB pages */

// macros to do page alignment an manipulation
#define MMAP_PAGE_SIZE (1<<ALLOC_PAGE_SIZE_EXPONENT)
#define PAGES_TO_BYTES(x) (((size_t)x) << ALLOC_PAGE_SIZE_EXPONENT)
#define BYTES_TO_PAGES(x)  ( (PAGES_TO_BYTES((x)>>ALLOC_PAGE_SIZE_EXPONENT) == (x)) ? ((x)>>ALLOC_PAGE_SIZE_EXPONENT) : (((x)>>ALLOC_PAGE_SIZE_EXPONENT)+1))

// macros to provide the low level memory allocation from teh system

// Do we need to use the debuggable malloc based allocation?
#ifdef MMAP_IS_MALLOC

#define mmap_alloc(numBytes,node) malloc(numBytes)
#define mmap_free(ptr) free(ptr)
#define MMAP_DIAG // nothing

#else

// function to allocate memory using the mmap subsystem
// this bypasses malloc facility and provides alligned pages that
// are efficient with respect to I/O using dma (zero copy in the kernel)
// the argument is the amount to allocate
// If node is specified, the system tries to allocate memory from a specific numa node
// To avoid unnecessary complications, this is just a preference
extern void* mmap_alloc_imp(size_t noBytes, int node = NUMA_ALL_NODES, const char* file=NULL, int line = -1);

#define mmap_alloc(numBytes,node) mmap_alloc_imp(numBytes,node,__FILE__,__LINE__)

// deallocation function
// the argument is the pointer to the region that was previously allocated
// A fatal error will be emited if the pointer was not previously allocated
extern void mmap_free_imp(void* ptr, const char* file=NULL, int line=-1);

#define mmap_free(ptr) 	mmap_free_imp(ptr,__FILE__,__LINE__)


// how much memory is allocated in the system (inefficient function)
extern off_t mmap_used(void);

//#ifdef MMAP_CHECK
extern void mmap_diagnose();
//#endif

#define MMAP_DIAG {\
	mmap_diagnose();\
}

#endif // MMAP_IS_MALLOC

#endif // _MMAP_ALLOCATOR_H_
