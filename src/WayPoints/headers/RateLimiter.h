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

class RateLimiter {
  double speedupParam;
  double slowdownParam;

  uint64_t delay; // in nanoseconds

  uint64_t chunksOut;
  uint64_t chunksAcked;
  uint64_t chunksDropped;

 public:
  RateLimiter();

  void ChunkOut(void);
  void ChunkAcked(void);
  void ChunkDropped(void);
  timespec GetMinStart(void);
};

#endif //  _RATE_LIMITER_H_
