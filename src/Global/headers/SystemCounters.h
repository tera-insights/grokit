#include <map>
#include <pthread.h>
#include <sstream>
#include <inttypes.h>
#include <time.h>

#include "Timer.h"
#include "Errors.h"
#include "Stl.h"
#include "Logging.h"

/** Class provides a system wide interface to count various kinds of
    things such as chunks processed, tuples processed, bytes read,
    etc. 

    All counters are 64bit ints
*/

class SystemCounters {
  typedef std::map<std::string, uint64_t> CounterMap;
  CounterMap cMap; // map of counters

  pthread_mutex_t mutex;
  double lastTick; // clock at last tick
  clock_t lastCpu; // last cpu counter

  // singleton
  static SystemCounters* instance;
public:
  // singleton implementation
  static SystemCounters& GetSystemCounters();

  SystemCounters(void);

  // Increment a counter.
  // Name of counter unrestricted
  // No need to pre-define the counters
  void Increment(const char* counter, uint64_t value);

  // print in a string the values of all counters
  // The value of the counters is normalized by the amount of elapsed time
  std::string GetCounters(double newClock);

  // TODO, add a database inteface to dump the counters

  // print on stdout the counters if 1 second elapsed
  void PrintIfTick(void);

  ~SystemCounters(void);
};


/** INLINE FUNCTIONS */
inline SystemCounters::SystemCounters(void){
  lastTick=global_clock.GetTime();
  lastCpu=clock();
  pthread_mutex_init (&mutex, NULL);
}

inline SystemCounters& SystemCounters::GetSystemCounters() {
	//is instance already created?
	if(instance == NULL)
	{
		//creating it once
		instance = new SystemCounters();
	}
	return *instance;
}


inline void SystemCounters::Increment(const char* counter, uint64_t value){
  pthread_mutex_lock (&mutex);
  if (cMap.find(std::string(counter)) == cMap.end()){ // not found
    cMap.insert(std::make_pair(counter, value));
  } else {
    cMap[std::string(counter)]+=value;
  }
  pthread_mutex_unlock (&mutex);
}

inline std::string SystemCounters::GetCounters(double newClock){
  std::ostringstream out;
  clock_t newCpu=clock();
  out << "CPU:" << (newCpu-lastCpu)/(newClock-lastTick)/CLOCKS_PER_SEC << "\t";
  lastCpu = newCpu;
  FOREACH_STL(el, cMap){
    std::string counter=el.first;
    uint64_t& value=el.second;
    uint64_t val = value;
    if (val>0){
      val/=(newClock-lastTick);
      out << counter << ":";
      if (val>(1ULL<<22)){
      	val >>= 20;
      	out << val << "M";
      } else if (val>(1ULL<<12)) {
      	val >>= 10;
      	out << val << "K";
      } else
	out << val;
      out <<"\t";
    }
    value = 0; // reset value
  }END_FOREACH;
  lastTick = newClock;
  return out.str();
}

inline void SystemCounters::PrintIfTick(){
  pthread_mutex_lock (&mutex);
  double clock = global_clock.GetTime();
  if ( clock> 1.0+lastTick){
    std::string output = GetCounters(clock);
    printf("SC(%.2f)\t%s\n", clock, output.c_str());
  }
  pthread_mutex_unlock (&mutex);
}

inline SystemCounters::~SystemCounters(){
  pthread_mutex_destroy (&mutex);
}
