/* Copyright (C) 2010 G.P. Halkes
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

#include "stringmatcher.h"

/* Adaptation of the Knuth-Morris-Pratt string searching algorithm, to work
   with UTF-8 and allow tracking of the start of the match in number of
   UTF-8 characters instead of bytes. The latter is required to allow the
   case-insensitive matching in UTF-8 to match the fully case-folded
   version. */

string_matcher_t::string_matcher_t(const string &_needle) : needle(_needle) {
	size_t pos, cnd;

	partial_match_table = new int[needle.size() + 1];
	partial_match_table[0] = -1;
	partial_match_table[1] = 0;
	reverse_partial_match_table = new int[needle.size() + 1];
	reverse_partial_match_table[0] = -1;
	reverse_partial_match_table[1] = 0;
	index_table = new int[needle.size() + 1];

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

string_matcher_t::~string_matcher_t(void) {
	delete partial_match_table;
	delete reverse_partial_match_table;
	delete index_table;
}

void string_matcher_t::reset(void) {
	i = 0;
	index_table[0] = 0;
}

int string_matcher_t::next_char(const string *c) {
	while (1) {
		if (needle.compare(i, c->size(), *c) == 0) {
			index_table[i + c->size()] = index_table[i] + 1;
			i += c->size();
			if ((size_t) i == needle.size())
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

int string_matcher_t::previous_char(const string *c) {
	while (1) {
		if (i + c->size() <= needle.size() && needle.compare(needle.size() - i - c->size(), c->size(), *c) == 0) {
			index_table[i + c->size()] = index_table[i] + 1;
			i += c->size();
			if ((size_t) i == needle.size())
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


#if 0
#include <cstdio>
#include <cstdlib>
int main(int argc, char *argv[]) {
										/* 012345678901234567890123 */
	string needle = "abcdabde", haystack = "abc abcdab  abcdabcdabdefgcdabdef";
	string_matcher_t matcher(&needle);
	int start = 0, i;
#if 0
	for (i = start; (size_t) i < haystack.size(); i += 2) {
		string substr = haystack.substr(i, 2);
		int result = matcher.next_char(&substr);
		if (result >= 0) {
			printf("Found substring at %d, %d\n", result + start, i);
			exit(1);
		}
	}
	matcher.reset();
#endif
	for (i = haystack.size() - start - 2; i >= 0; i -= 2) {
		string substr = haystack.substr(i, 2);
		int result = matcher.previous_char(&substr);
		if (result >= 0) {
			printf("Found substring at %d, %d\n", result + start, i);
			exit(1);
		}
	}
}
#endif
