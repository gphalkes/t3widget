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
#ifndef T3_WIDGET_FINDCONTEXT_H
#define T3_WIDGET_FINDCONTEXT_H

#include <string>
#include <pcre.h>

#include "stringmatcher.h"

namespace t3_widget {

struct find_result_t {
	int start, end;
};

class finder_t {
	//FIXME: many members are necessary in other classes
	private:
		int flags;

		/* Standard string matcher. */
		string_matcher_t *matcher;

		/* PCRE context and data */
		pcre *regex;
		int ovector[30];
		int captures;
		int pattern_length;
		bool found, valid;

		/* Replacement string. */
		std::string *replacement;

		/* Space to store the case-folded representation of a single character. */
		char *folded;
		size_t folded_size;

		static int callout(pcre_callout_block *block);
		static int adjust_position(const std::string *str, int pos, int adjust);
		static bool check_boundaries(const std::string *str, int match_start, int match_end);
		static int get_class(const std::string *str, int pos);

	public:
		finder_t(void);
		finder_t(const std::string *needle, int flags, const std::string *replacement = NULL);
		~finder_t(void);
		finder_t &operator=(finder_t& other);

		void set_context(const std::string *needle, int flags, const std::string *replacement = NULL);
		bool match(const std::string *haystack, find_result_t *result, bool reverse);
		int get_flags(void);
		std::string *get_replacement(const std::string *haystack);
};

}; // namespace

#endif
