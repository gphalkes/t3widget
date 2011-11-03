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
#ifndef T3_WIDGET_FINDCONTEXT_H
#define T3_WIDGET_FINDCONTEXT_H

#include <string>
#include <pcre.h>

#include <t3widget/widget_api.h>
#include <t3widget/stringmatcher.h>
#include <t3widget/util.h>

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

namespace t3_widget {

/** @internal A struct holding the result of a find operation.
    In the current situation, find operations work on a single line. Therefore
    this struct does not contain line references.
*/
struct find_result_t {
	int start, end;
};

class text_line_t;

/** @internal Class holding the context of a find operation. */
class T3_WIDGET_LOCAL finder_t {
	friend class text_line_t;
	//FIXME: many members are necessary in other classes
	private:
		static void call_pcre_free(pcre *);

		/** Flags indicating what type of search was requested. */
		int flags;

		/** Pointer to a string_matcher_t, if a non-regex search was requested. */
		cleanup_obj_ptr<string_matcher_t> matcher;

		/* PCRE context and data */
		/** Pointer to a compiled regex. */
		cleanup_ptr<pcre, pcre, call_pcre_free> regex;
		/** Array to hold sub-matches information. */
		int ovector[30];
		/** The number of sub-matches captured. */
		int captures;
		bool found; /**< Boolean indicating whether the regex match was successful. */

		/** Replacement string. */
		cleanup_obj_ptr<std::string> replacement;

		/** Space to store the case-folded representation of a single character. */
		cleanup_ptr<char> folded;
		/** Size of the finder_t::folded buffer. */
		size_t folded_size;

		/** Get the next position of a UTF-8 character. */
		static int adjust_position(const std::string *str, int pos, int adjust);
		/** Check if the start and end of a match are on word boundaries.
		    @param str The string to check.
		    @param match_start The position of the start of the match in @p str.
		    @param match_end The position of the end of the match in @p str.
		*/
		bool check_boundaries(const std::string *str, int match_start, int match_end);
		/** Get the character class at postion @p pos in the string. */
		static int get_class(const std::string *str, int pos);

	public:
		/** Create a new empty finder_t. */
		finder_t(void);
		/** Create a new finder_t for a specific search.
		    May throw a @c const @c char pointer holding an error message. Caller
		    of this constructor remains owner of passed objects.
		*/
		finder_t(const std::string *needle, int flags, const std::string *replacement = NULL);
		/** Destroy a finder_t instance. */
		virtual ~finder_t(void);
		/** Assign the value of another finder_t to this finder_t.
		    Assignment using this operator is destructive to @p other. I.e. this
		    finder_t instance will take ownership of all objects allocated by
		    @p other, and set @p other's object pointers to NULL.
		*/
		finder_t &operator=(finder_t& other);

		/** Set the search parameters.
		    May throw a @c const @c char pointer holding an error message. Caller
		    of this function remains owner of passed objects.
		*/
		void set_context(const std::string *needle, int flags, const std::string *replacement = NULL);
		/** Try to find the previously set @c needle in a string. */
		bool match(const std::string *haystack, find_result_t *result, bool reverse);
		/** Retrieve the flags set when setting the search context. */
		int get_flags(void);
		/** Retrieve the replacement string.
		    Returns a newly allocated string, for which the caller will have
		    ownership.
		*/
		std::string *get_replacement(const std::string *haystack);
};

}; // namespace
#endif
