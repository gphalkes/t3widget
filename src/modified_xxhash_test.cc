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

// Test the xxHash implementation against the reference implementation. Only works on little endian
// machines, as the local implementation doesn't do byte order correction.

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#define _T3_WIDGET_INTERNAL
#include "widget_api.h"
#include "xxhash.h"

namespace t3_widget {
namespace internal {

T3_WIDGET_LOCAL uint32_t ModifiedXXHash32(const void *data, size_t length, uint32_t seed);
T3_WIDGET_LOCAL uint64_t ModifiedXXHash64(const void *data, size_t length, uint64_t seed);
}  // namespace internal
}  // namespace t3_widget

void check(uint64_t orig, uint64_t own, int bits, size_t i, size_t j, uint64_t seed) {
  if (orig != own) {
    std::cout << "Different values: " << std::hex << orig << " vs. " << own << " on " << std::dec
              << bits << "," << i << "," << j << "," << seed << "\n";
  }
}

int main(int, char **) {
  char data[] =
      "9o8uqcwrmpucouns o9m098 w opyhasfdnioyuaciynixoinyarwity"
      "O*^YHKREOCUJKLmDSO*UIjkerfdo9uijseklrfzpo7lyuuakewrd]0puoqiahfkR&E*OUIWRFK";
  for (int k = 0; k < 20; ++k) {
    uint64_t seed = std::rand();
    for (size_t i = 0; i < sizeof(data); ++i) {
      for (size_t j = 1; j + i <= sizeof(data); ++j) {
        check(XXH64(data + i, j, seed), t3_widget::internal::ModifiedXXHash64(data + i, j, seed),
              64, i, j, seed);
        check(XXH32(data + i, j, seed), t3_widget::internal::ModifiedXXHash32(data + i, j, seed),
              32, i, j, seed);
      }
    }
  }
}
