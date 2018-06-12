/* Copyright (C) 2018 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Modified version of the xxHash{32,64} functions. The modification consists of skipping the
   endianess correction. This is OK, because we only guarantee that the hash value is stable within
   a single run of the program. We also use the 32-bit version on 32-bit computers, which means that
   there will be differences in outcome between different platforms. */

#include "modified_xxhash.h"

#include <cstdint>
#include <cstring>

namespace t3widget {
namespace {

constexpr uint32_t PRIME32_1 = UINT32_C(2654435761);
constexpr uint32_t PRIME32_2 = UINT32_C(2246822519);
constexpr uint32_t PRIME32_3 = UINT32_C(3266489917);
constexpr uint32_t PRIME32_4 = UINT32_C(668265263);
constexpr uint32_t PRIME32_5 = UINT32_C(374761393);

constexpr uint64_t PRIME64_1 = UINT64_C(11400714785074694791);
constexpr uint64_t PRIME64_2 = UINT64_C(14029467366897019727);
constexpr uint64_t PRIME64_3 = UINT64_C(1609587929392839161);
constexpr uint64_t PRIME64_4 = UINT64_C(9650029242287828579);
constexpr uint64_t PRIME64_5 = UINT64_C(2870177450012600261);

/* To access unaligned uint{32,64}_t variables, we put them in structs with a packing of 1. This
   forces the compiler to produce instructions that access the value at any position. However, it is
   possible that the compiler doesn't understand the pack pragma. Therefore, we check the alignof in
   the getuint{32,64} functions. If it is not 1 as requested, then we simply use memcpy. */
#pragma pack(push, 1)
struct unaligned32_t {
  uint32_t value;
};
struct unaligned64_t {
  uint64_t value;
};
#pragma pack(pop)

uint32_t getuint32(const uint8_t *ptr) {
  if (alignof(unaligned32_t) == 1) {
    return reinterpret_cast<const unaligned32_t *>(ptr)->value;
  } else {
    uint32_t result;
    std::memcpy(&result, ptr, sizeof(uint32_t));
    return result;
  }
}

uint64_t getuint64(const uint8_t *ptr) {
  if (alignof(unaligned64_t) == 1) {
    return reinterpret_cast<const unaligned64_t *>(ptr)->value;
  } else {
    uint64_t result;
    std::memcpy(&result, ptr, sizeof(uint64_t));
    return result;
  }
}

uint32_t rotate_left32(uint32_t value, unsigned steps) {
  return value << steps | value >> (32 - steps);
}

uint64_t rotate_left64(uint64_t value, unsigned steps) {
  return value << steps | value >> (64 - steps);
}

uint32_t round32(uint32_t accN, uint32_t laneN) {
  accN = accN + (laneN * PRIME32_2);
  accN = rotate_left32(accN, 13);
  return accN * PRIME32_1;
}

uint64_t round64(uint64_t accN, uint64_t laneN) {
  accN = accN + (laneN * PRIME64_2);
  accN = rotate_left64(accN, 31);
  return accN * PRIME64_1;
}

uint64_t merge_accumulator64(uint64_t acc, uint64_t accN) {
  acc = acc ^ round64(0, accN);
  acc = acc * PRIME64_1;
  return acc + PRIME64_4;
}
}  // namespace

namespace internal {
T3_WIDGET_LOCAL uint32_t ModifiedXXHash32(const void *data, size_t length, uint32_t seed) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);

  uint32_t acc;
  if (length >= 16) {
    uint32_t acc1 = seed + PRIME32_1 + PRIME32_2;
    uint32_t acc2 = seed + PRIME32_2;
    uint32_t acc3 = seed + 0;
    uint32_t acc4 = seed - PRIME32_1;
    for (size_t blocks = length / 16; blocks > 0; --blocks, bytes += 16) {
      acc1 = round32(acc1, getuint64(bytes));
      acc2 = round32(acc2, getuint64(bytes + 4));
      acc3 = round32(acc3, getuint64(bytes + 8));
      acc4 = round32(acc4, getuint64(bytes + 12));
    }
    acc = rotate_left32(acc1, 1) + rotate_left32(acc2, 7) + rotate_left32(acc3, 12) +
          rotate_left32(acc4, 18);
  } else {
    acc = seed + PRIME32_5;
  }
  acc += length;

  for (length &= 0xf; length >= 4; length -= 4, bytes += 4) {
    acc = acc + getuint32(bytes) * PRIME32_3;
    acc = rotate_left32(acc, 17) * PRIME32_4;
  }
  for (; length > 0; --length, ++bytes) {
    acc = acc + *bytes * PRIME32_5;
    acc = rotate_left32(acc, 11) * PRIME32_1;
  }
  acc = acc ^ (acc >> 15);
  acc = acc * PRIME32_2;
  acc = acc ^ (acc >> 13);
  acc = acc * PRIME32_3;
  acc = acc ^ (acc >> 16);
  return acc;
}

T3_WIDGET_LOCAL uint64_t ModifiedXXHash64(const void *data, size_t length, uint64_t seed) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);

  uint64_t acc;
  if (length >= 32) {
    uint64_t acc1 = seed + PRIME64_1 + PRIME64_2;
    uint64_t acc2 = seed + PRIME64_2;
    uint64_t acc3 = seed + 0;
    uint64_t acc4 = seed - PRIME64_1;
    for (size_t blocks = length / 32; blocks > 0; --blocks, bytes += 32) {
      acc1 = round64(acc1, getuint64(bytes));
      acc2 = round64(acc2, getuint64(bytes + 8));
      acc3 = round64(acc3, getuint64(bytes + 16));
      acc4 = round64(acc4, getuint64(bytes + 24));
    }
    acc = rotate_left64(acc1, 1) + rotate_left64(acc2, 7) + rotate_left64(acc3, 12) +
          rotate_left64(acc4, 18);
    acc = merge_accumulator64(acc, acc1);
    acc = merge_accumulator64(acc, acc2);
    acc = merge_accumulator64(acc, acc3);
    acc = merge_accumulator64(acc, acc4);
  } else {
    acc = seed + PRIME64_5;
  }
  acc += length;
  for (length &= 0x1f; length >= 8; length -= 8, bytes += 8) {
    acc = acc ^ round64(0, getuint64(bytes));
    acc = rotate_left64(acc, 27) * PRIME64_1;
    acc = acc + PRIME64_4;
  }
  if (length >= 4) {
    acc = acc ^ (getuint32(bytes) * PRIME64_1);
    acc = rotate_left64(acc, 23) * PRIME64_2;
    acc = acc + PRIME64_3;
    bytes += 4;
    length -= 4;
  }
  for (; length >= 1; --length, ++bytes) {
    acc = acc ^ (*bytes * PRIME64_5);
    acc = rotate_left64(acc, 11) * PRIME64_1;
  }
  acc = acc ^ (acc >> 33);
  acc = acc * PRIME64_2;
  acc = acc ^ (acc >> 29);
  acc = acc * PRIME64_3;
  acc = acc ^ (acc >> 32);
  return acc;
}

}  // namespace internal

size_t ModifiedXXHash(const void *data, size_t length, size_t seed) {
  if (sizeof(size_t) <= 4) {
    return internal::ModifiedXXHash32(data, length, seed);
  } else {
    return internal::ModifiedXXHash64(data, length, seed);
  }
}

}  // namespace t3widget
