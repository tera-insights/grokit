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
/** Class to implement performance counter capabilities using the perf interface in the
    Linux kernel. Oher platforms can be suported through performance libraries. */

#ifndef _PERFORMANCE_COUNTERS
#define _PERFORMANCE_COUNTERS

#include <inttypes.h>
#include "Errors.h"

#ifndef USE_PERF_LINUX
#warning "Performance counters not working"
#endif

#ifdef  USE_PERF_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <cstdio>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "Swap.h"
#include <iostream>

/* perf_event_open syscall wrapper */
static inline int
sys_perf_event_open(struct perf_event_attr *attr,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags)
{
  attr->size = sizeof(*attr);
  return syscall(__NR_perf_event_open, attr, pid, cpu,
                 group_fd, flags);
}

/** gettid syscall */
static inline pid_t
gettid()
{
  return syscall(SYS_gettid);
}

/** translation table from events to perf datastructures */
static perf_event_attr translation_table[] = {
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_CPU_CYCLES         },
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_INSTRUCTIONS       },
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_CACHE_REFERENCES   },
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_CACHE_MISSES       },
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
  { PERF_TYPE_HARDWARE, 0, PERF_COUNT_HW_BRANCH_MISSES      },
  { PERF_TYPE_SOFTWARE, 0, PERF_COUNT_SW_CONTEXT_SWITCHES   },
  { PERF_TYPE_SOFTWARE, 0, PERF_COUNT_SW_TASK_CLOCK   }
};


#endif // USE_PERF_LINUX

/** This class implements performance counters. They can be used to track in detail aspects of program execution
    They are very accurate

    Example usage:
      PerfCounter cycles(PerfCounter::Cycles);
      // do some computation
      uint64_t cyclesC = cycles.GetCount(); //number of cycles it took
      cycles.Reset(); // reset the counter
      // do more computation
      if (cyclesC < cycles.GetCount()){
        // the second computation took longer
      }
  */
class PerfCounter {
 public:
  /** Types of event to track */
  enum EventType {
    Cycles = 0,
    Instructions = 1,
    Cache_References = 2,
    Cache_Misses = 3,
    Branch_Instructions = 4,
    Branch_Misses = 5,
    Context_Switches = 6,
    Task_Clock = 7
  };

  /** Names of counters */
  // Initialized in .cc file so that storage is allocated for the strings.
  const static char* names[];

 private:

#ifdef USE_PERF_LINUX
  int file; // the file descriptor to read performance counter
#endif

 public:
  /* default constructor */
 PerfCounter(): file(-1) {}

  /* Constructor. global indicases whether do count per process(true) or per task(false) */
  PerfCounter(EventType evType, bool global = false /* if true, PC per process */);

  /* reset the counter. Offers convenience to avoid remembering start value of counter */
  void Restart(void){
#ifdef USE_PERF_LINUX
    ioctl(file, PERF_EVENT_IOC_ENABLE | PERF_EVENT_IOC_RESET);
#endif
  }

  /** get the value of the counetr (scalled) */
  uint64_t  GetCount(void);

  /** swapping paradigm */
  void Swap(PerfCounter& other){ SWAP_STD(file, other.file); }
  void swap(PerfCounter & other) { SWAP_STD(file, other.file); }

  /* desctuctor */
  ~PerfCounter();
};

// Override global swap
void swap( PerfCounter & a, PerfCounter & b );

////////// INLINE DEFINITIONS ////////////

inline
void swap( PerfCounter & a, PerfCounter & b ) {
    a.swap(b);
}

inline
PerfCounter :: PerfCounter(EventType evType, bool global /* if true, PC per process */) {
#ifdef USE_PERF_LINUX
    pid_t pid;
    if (global)
      pid = getpid();/* per this process */
    else
      pid = gettid(); /* per thread */

    perf_event_attr attr = translation_table[evType];
    attr.disabled = 0; // enable it already
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING; // enxtended info
    file = sys_perf_event_open(&attr, pid , -1 /** all cpus */, -1, 0);
    if( file == -1 ) {
        WARNING("Performance counter could not be started. It will return 1 for all calls");
        perror("PerfCounter");
    }
    //std::cout << "New PerfCounter with fd: " << file << " pid: " << pid << std::endl;
#endif
  }

inline
uint64_t  PerfCounter :: GetCount(void){
#ifdef USE_PERF_LINUX
    struct {
        uint64_t value;
        uint64_t enabled;
        uint64_t running;
    } count = {0,0,0};

    if (file!=-1){
        int ret = read(file, &count, sizeof(count));
        uint64_t newVal = 0;
#if 0
        printf("Read: %d RAW Counter: %ld %ld %ld\n", ret, count.value, count.enabled, count.running);
#endif
        if ( ret == sizeof(uint64_t))
            newVal = count.value;
        else if (ret == sizeof(count)){
            if (count.running>0 && count.running < count.enabled){ /* we need to scale the counter */
                newVal = (uint64_t)((double)count.value * count.enabled / count.running + 0.5);
            }
            else {
                newVal =  count.value; /* not scaled */
            }
        }

        return newVal;
    }
    return 1;
#else
    return 0;
#endif
  }

inline
PerfCounter ::  ~PerfCounter(){
#ifdef USE_PERF_LINUX
    if (file!=-1)
      close(file); //
#endif
  }

#endif // _PERFORMANCE_COUNTERS
