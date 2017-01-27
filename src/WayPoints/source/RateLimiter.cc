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

#include "RateLimiter.h"

RateLimiter :: RateLimiter() : 
  speedupParam(10 * 1000 * 1000), // 10ms
  slowdownParam(1.2), // half
  delay(1000 * 1000) // 1ms
{}

void RateLimiter :: ChunkOut() {
  chunksOut++;
}

// Delay never goes below 1ms
void RateLimiter :: ChunkAcked() {
  chunksAcked++;
  delay -= speedupParam;
  if (delay < 1000 * 1000) { // 1ms minimum
    delay = 1000 * 1000;
  }
}

void RateLimiter :: ChunkDropped() {
  chunksDropped++;
  delay *= slowdownParam;
  if (delay > 1000 * 1000 * 1000) {
    delay = 1000 * 1000 * 1000;
  }
}

timespec RateLimiter :: GetMinStart() {
  timespec when;
  clock_gettime(CLOCK_MONOTONIC, &when);
  when.tv_nsec = when.tv_nsec + delay;
  if (when.tv_nsec >= 1000000000L) {
    when.tv_sec++;  
    when.tv_nsec = when.tv_nsec - 1000000000L;
  }
  return when;
}

int64_t RateLimiter :: GetDelay() {
  return delay;
}
