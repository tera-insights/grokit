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
  cw.start = RateLimiter::GetNow();
  inFlight[id] = cw;
}

// New Ack Time = L * Old Ack Time + (1 - L) * Time for this chunk
void RateLimiter :: ChunkAcked(int id) {
  chunksAcked++;
  std::printf("chunks acked = %d\n", chunksAcked);
  schedule_time now = RateLimiter::GetNow();
  schedule_time start = inFlight.at(id).start;
  std::chrono::nanoseconds elapsed = now - start;
  averageAckTime = std::chrono::duration_cast<std::chrono::nanoseconds>(oldAckWeight * averageAckTime + 
									(1 - oldAckWeight) * elapsed);
}

// Ack Time *= slowdownParam
void RateLimiter :: ChunkDropped(int id) {
  inFlight.erase(id);
  averageAckTime *= slowdownParam;
}

/**
   GetMinStart uses the average ack time to propose a minimum start time m_p.
   m_p = time_for_oldest_chunk + average_ack_time;
*/
schedule_time RateLimiter :: GetMinStart() {
  schedule_time oldest = RateLimiter::GetNow(); 
  int oldestKey;

  for (auto iter = inFlight.begin(); iter != inFlight.end(); iter++) {
    auto current = inFlight[iter->first];
    if (!current.usedInCalc && current.start < oldest) {
      oldest = current.start;
      oldestKey = iter->first;
    }
  }

  inFlight[oldestKey].usedInCalc = true;

  schedule_time now = RateLimiter::GetNow();
  std::chrono::nanoseconds elapsed = now - oldest;
  // std::printf("diff b/w now and oldest = %lld\n", elapsed);
  
  std::printf("returning delay of %lld\n", oldest + averageAckTime);
  return oldest + averageAckTime;
}

std::chrono::nanoseconds RateLimiter :: GetAverageAckTime() {
  return averageAckTime;
}

schedule_time RateLimiter :: GetNow() {
  return std::chrono::steady_clock::now();
}
