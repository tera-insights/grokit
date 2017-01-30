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

#ifndef _RATE_LIMITER_H_
#define _RATE_LIMITER_H_

#include <cinttypes>
#include <ctime>
#include <unordered_map>

struct ChunkWrapper {
  bool usedInCalc; // To avoid computing the same delays
  timespec start; // When we think it was sent
};

class RateLimiter {
  double slowdownParam;
  double oldAckWeight;

  uint64_t averageAckTime; // in nanoseconds
  std::unordered_map<int, ChunkWrapper> inFlight;

 public:
  RateLimiter();

  void ChunkOut(int id);
  void ChunkAcked(int id);
  void ChunkDropped(int id);
  timespec GetMinStart(void);
  uint64_t GetAverageAckTime(void);
};

#endif //  _RATE_LIMITER_H_
