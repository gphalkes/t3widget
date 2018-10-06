/* Copyright (C) 2011,2018 G.P. Halkes
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
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "t3widget/string_view.h"
#include "t3widget/stringmatcher.h"

namespace t3widget {

/* Adaptation of the Knuth-Morris-Pratt string searching algorithm, to work
   with UTF-8 and allow tracking of the start of the match in number of
   UTF-8 characters instead of bytes. The latter is required to allow the
   case-insensitive matching in UTF-8 to match the fully case-folded
   version. */

string_matcher_t::string_matcher_t(string_view _needle) : needle(_needle.data(), _needle.size()) {
  init();
}

void string_matcher_t::init() {
  size_t pos, cnd;

  partial_match_table.resize(needle.size() + 1);
  partial_match_table[0] = -1;
  partial_match_table[1] = 0;
  reverse_partial_match_table.resize(needle.size() + 1);
  reverse_partial_match_table[0] = -1;
  reverse_partial_match_table[1] = 0;
  index_table.resize(needle.size() + 1);

  pos = 2;
  cnd = 0;
  while (pos <= needle.size()) {
    if (needle[pos - 1] == needle[cnd]) {
      partial_match_table[pos] = cnd + 1;
      pos++;
      cnd++;
    } else if (cnd > 0) {
      cnd = partial_match_table[cnd];
    } else {
      partial_match_table[pos] = 0;
      pos++;
    }
  }

  pos = 2;
  cnd = 0;
  while (pos <= needle.size()) {
    if (needle[needle.size() - 1 - (pos - 1)] == needle[needle.size() - 1 - cnd]) {
      reverse_partial_match_table[pos] = cnd + 1;
      pos++;
      cnd++;
    } else if (cnd > 0) {
      cnd = reverse_partial_match_table[cnd];
    } else {
      reverse_partial_match_table[pos] = 0;
      pos++;
    }
  }

  reset();
}

string_matcher_t::~string_matcher_t() {}

void string_matcher_t::reset() {
  i = 0;
  index_table[0] = 0;
}

int string_matcher_t::next_char(string_view c) {
  while (true) {
    if (i + c.size() <= needle.size() && memcmp(needle.c_str() + i, c.data(), c.size()) == 0) {
      index_table[i + c.size()] = index_table[i] + 1;
      i += c.size();
      if (static_cast<size_t>(i) == needle.size()) {
        return index_table[0];
      }
      return -1;
    } else {
      int new_i = partial_match_table[i];
      if (new_i >= 0) {
        memmove(index_table.data(), index_table.data() + i - new_i, sizeof(int) * (new_i + 1));
        i = new_i;
      } else {
        index_table[0]++;
        return -1;
      }
    }
  }
}

int string_matcher_t::previous_char(string_view c) {
  while (true) {
    if (i + c.size() <= needle.size() &&
        memcmp(needle.c_str() + needle.size() - i - c.size(), c.data(), c.size()) == 0) {
      index_table[i + c.size()] = index_table[i] + 1;
      i += c.size();
      if (static_cast<size_t>(i) == needle.size()) {
        return index_table[0];
      }
      return -1;
    } else {
      int new_i = reverse_partial_match_table[i];
      if (new_i >= 0) {
        memmove(index_table.data(), index_table.data() + i - new_i, sizeof(int) * (new_i + 1));
        i = new_i;
      } else {
        index_table[0]++;
        return -1;
      }
    }
  }
}

}  // namespace t3widget
