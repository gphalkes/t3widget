/* Copyright (C) 2011 G.P. Halkes
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
#include <cstring>
#include <cstdlib>

#include "stringmatcher.h"

namespace t3_widget {

/* Adaptation of the Knuth-Morris-Pratt string searching algorithm, to work
   with UTF-8 and allow tracking of the start of the match in number of
   UTF-8 characters instead of bytes. The latter is required to allow the
   case-insensitive matching in UTF-8 to match the fully case-folded
   version. */

string_matcher_t::string_matcher_t(const std::string &_needle) {
	needle_size = _needle.size();
	if ((needle = (char *) malloc(needle_size)) == nullptr)
		throw std::bad_alloc();
	memcpy(needle, _needle.data(), needle_size);
	init();
}

string_matcher_t::string_matcher_t(char *_needle, size_t _needle_size) : needle(_needle), needle_size(_needle_size) {
	init();
}

void string_matcher_t::init() {
	size_t pos, cnd;

	partial_match_table = new int[needle_size + 1];
	partial_match_table[0] = -1;
	partial_match_table[1] = 0;
	reverse_partial_match_table = new int[needle_size + 1];
	reverse_partial_match_table[0] = -1;
	reverse_partial_match_table[1] = 0;
	index_table = new int[needle_size + 1];

	pos = 2;
	cnd = 0;
	while (pos <= needle_size) {
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
	while (pos <= needle_size) {
		if (needle[needle_size - 1 - (pos - 1)] == needle[needle_size - 1 - cnd]) {
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

int string_matcher_t::next_char(const std::string *c) { return next_char(c->data(), c->size()); }
int string_matcher_t::previous_char(const std::string *c) { return previous_char(c->data(), c->size()); }


int string_matcher_t::next_char(const char *c, size_t c_size) {
	while (1) {
		if (i + c_size <= needle_size && memcmp(needle + i, c, c_size) == 0) {
			index_table[i + c_size] = index_table[i] + 1;
			i += c_size;
			if ((size_t) i == needle_size)
				return index_table[0];
			return -1;
		} else {
			int new_i = partial_match_table[i];
			if (new_i >= 0) {
				memmove(index_table, index_table + i - new_i, sizeof(int) * (new_i + 1));
				i = new_i;
			} else {
				index_table[0]++;
				return -1;
			}
		}
	}
}

int string_matcher_t::previous_char(const char *c, size_t c_size) {
	while (1) {
		if (i + c_size <= needle_size && memcmp(needle + needle_size - i - c_size, c, c_size) == 0) {
			index_table[i + c_size] = index_table[i] + 1;
			i += c_size;
			if ((size_t) i == needle_size)
				return index_table[0];
			return -1;
		} else {
			int new_i = reverse_partial_match_table[i];
			if (new_i >= 0) {
				memmove(index_table, index_table + i - new_i, sizeof(int) * (new_i + 1));
				i = new_i;
			} else {
				index_table[0]++;
				return -1;
			}
		}
	}
}

}; // namespace
