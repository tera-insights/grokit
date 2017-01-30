//
//  Copyright 2017 Jess Smith
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
   RateLimiter calculates the minimum start time at which chunks should be sent.
   These times are determined by delays from the current CLOCK_MONOTONIC time. 
   The delays are calculated from a few factors: the average ack time, the send times
   of chunks currently in-flight, and how many times chunks have been dropped.
*/


#include "RateLimiter.h"
#include "ExecEngineImp.h"

timespec getNow() {
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now;
}

RateLimiter :: RateLimiter() : 
  slowdownParam(1.01),
  averageAckTime(1000 * 1000),
  oldAckWeight(0.5),
  inFlight()
{}

int chunksAcked = 0;

void RateLimiter :: ChunkOut(int id) {
  ChunkWrapper cw;
  cw.usedInCalc = false;
  cw.start = getNow();
  inFlight[id] = cw;
}

// New Ack Time = L * Old Ack Time + (1 - L) * Time for this chunk
void RateLimiter :: ChunkAcked(int id) {
  chunksAcked++;
  std::printf("chunks acked = %d\n", chunksAcked);
  auto now = getNow();
  timespec start = inFlight.at(id).start;
  
  // nanoseconds
  uint64_t elapsed = 1000 * 1000 * 1000 * (now.tv_sec - start.tv_sec);

  // Explicitly handling the subtraction case to avoid problems in case
  // tv_nsec can't handle negative values.
  if (now.tv_nsec >= start.tv_nsec) {
    elapsed += now.tv_nsec - start.tv_nsec;
  } else {
    elapsed -= start.tv_nsec - now.tv_nsec;
  }

  averageAckTime = oldAckWeight * averageAckTime + 
    (1 - oldAckWeight) * elapsed;
}

// Ack Time *= slowdownParam
void RateLimiter :: ChunkDropped(int id) {
  inFlight.erase(id);
  // averageAckTime *= slowdownParam;
}

/**
   GetMinStart uses the average ack time to propose a minimum start time m_p.
   m_p = time_for_oldest_chunk + average_ack_time;
*/
timespec RateLimiter :: GetMinStart() {
  timespec oldest = getNow(); 
  int oldestKey;

  for (auto iter = inFlight.begin(); iter != inFlight.end(); iter++) {
    auto current = inFlight[iter->first];
    if (!current.usedInCalc && current.start < oldest) {
      oldest = current.start;
      oldestKey = iter->first;
    }
  }

  inFlight[oldestKey].usedInCalc = true;

  timespec now = getNow();
  uint64_t elapsed = 1000ULL * 1000 * 1000 * (now.tv_sec - oldest.tv_sec);
  if (now.tv_nsec >= oldest.tv_nsec) {
    elapsed += now.tv_nsec - oldest.tv_nsec;
  } else {
    elapsed -= oldest.tv_nsec - now.tv_nsec;
  }
  // std::printf("diff b/w now and oldest = %lld\n", elapsed);
  
  oldest.tv_nsec += averageAckTime;
  if (oldest.tv_nsec >= 1000000000L) {
    oldest.tv_sec++;  
    oldest.tv_nsec = oldest.tv_nsec - 1000000000L;
  }
  return oldest;
}

uint64_t RateLimiter :: GetAverageAckTime() {
  return averageAckTime;
}
