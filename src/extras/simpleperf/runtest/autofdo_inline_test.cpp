/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>  // For usleep function

typedef uint32_t UInt32;
typedef uint16_t UInt16;

void cond_branch_example_function(uint16_t* prob, uint8_t* buf) {
  UInt32 range = 0xFFFFFFFF;
  UInt32 code = 0;
  UInt32 bound;
  UInt16 ttt;
  UInt32 symbol = 0;

  do {
    ttt = *(prob + symbol);
    if (range < ((UInt32)1 << 24)) {
      range <<= 8;
      code = (code << 8) | (*buf++);
    }
    bound = (range >> 11) * ttt;
    if (code < bound) {  // <== This is mispredicted branch (conditional branch)
      range = bound;
      *(prob + symbol) = (UInt16)(ttt + (((1 << 11) - ttt) >> 5));
      symbol = (symbol + symbol);
    } else {
      range -= bound;
      code -= bound;
      *(prob + symbol) = (UInt16)(ttt - (ttt >> 5));
      symbol = (symbol + symbol) + 1;
    }
  } while (symbol < 0x100);  // conditional branch
}

int main() {
  uint16_t prob[256] = {0};
  uint8_t buf[256] = {0};

  usleep(15000000);

  // Call the conditional branch example function
  cond_branch_example_function(prob, buf);

  return 0;
}
