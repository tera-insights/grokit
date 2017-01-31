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
#include <unordered_map>
#include <chrono>

typedef std::chrono::time_point<std::chrono::steady_clock> schedule_time;

struct ChunkWrapper {
  bool usedInCalc; // To avoid computing the same delays
  schedule_time start; // When we think it was sent
};

class RateLimiter {
  double slowdownParam;
  double oldAckWeight;

  std::chrono::nanoseconds averageAckTime; // in nanoseconds
  std::unordered_map<int, ChunkWrapper> inFlight;

 public:
  RateLimiter();

  void ChunkOut(int id);
  void ChunkAcked(int id);
  void ChunkDropped(int id);
  schedule_time GetMinStart(void);
  std::chrono::nanoseconds GetAverageAckTime(void);

  static schedule_time GetNow(void);
};

#endif //  _RATE_LIMITER_H_
