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

#include <unicode/unicode.h>
#include <cstring>
#include <climits>

#include "findcontext.h"
#include "util.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

find_context_t::find_context_t(void) : flags(0), matcher(NULL), regex(NULL), valid(false),
		replacement(NULL), folded(NULL), folded_size(0)
{}

find_context_t::find_context_t(const string *needle, int _flags, const string *_replacement) :
		flags(_flags), matcher(NULL), regex(NULL), valid(true), replacement(NULL), folded(NULL), folded_size(0)
{
	const char *error_message;
	if (flags & find_flags_t::REGEX) {
		int error_offset;
		int pcre_flags = PCRE_UTF8;

		string pattern = flags & find_flags_t::WHOLE_WORD ? "(?:\\b" : "(?:";
		pattern += *needle;
		pattern += flags & find_flags_t::WHOLE_WORD ? "\\b)(?C0)" : ")(?C0)";

		if (flags & find_flags_t::ICASE)
			pcre_flags |= PCRE_CASELESS;

		regex = pcre_compile(pattern.c_str(), pcre_flags, &error_message, &error_offset, NULL);
		if (regex == NULL)
			//FIXME: error offset should be added for clarity
			throw error_message;
		pattern_length = pattern.size();
	} else {
		/* Create a copy of needle, for transformation purposes. */
		string search_for(*needle);

		if (flags & find_flags_t::TRANSFROM_BACKSLASH) {
			if (!parse_escapes(search_for, &error_message))
				throw error_message;
		}

		if (flags & find_flags_t::ICASE) {
			size_t folded_needle_size;
			char *folded_needle;

			folded_needle_size = t3_unicode_casefold(search_for.data(), search_for.size(), &folded_needle, NULL, t3_false);
			/* When passing a const char * to string_matcher_t, it takes responsibility for
			   de-allocation. */
			matcher = new string_matcher_t(folded_needle, folded_needle_size);
		} else {
			matcher = new string_matcher_t(search_for);
		}
	}

	if (_replacement != NULL) {
		replacement = new string(*_replacement);
		if ((flags & find_flags_t::TRANSFROM_BACKSLASH) || (flags & find_flags_t::REGEX)) {
			if (!parse_escapes(*replacement, &error_message, (flags & find_flags_t::REGEX) != 0))
				throw error_message;
		}
	}
}

find_context_t::~find_context_t(void) {
	delete matcher;
	delete replacement;
	if (regex != NULL)
		pcre_free(regex);
	free(folded);
}

void find_context_t::set_context(const string *needle, int _flags, const string *_replacement) {
	/* If this initialization fails, it will throw a message pointer. */
	find_context_t new_context(needle, _flags, _replacement);

	/* Reset the search context. */
	delete matcher;
	delete replacement;
	if (regex != NULL)
		pcre_free(regex);
	/* Copy this instance's folded buffer to the new_context object, so we can do
	   a simple bit copy as initialization next. */
	new_context.folded = folded;
	new_context.folded_size = folded_size;

	*this = new_context;

	/* Make sure the destruction of the new_context object doesn't delete the things
	   we point to! */
	new_context.matcher = NULL;
	new_context.regex = NULL;
	new_context.replacement = NULL;
}

bool find_context_t::match(const string *haystack, find_result_t *result, bool backward) {
	int match_result;
	int start, end;

	if (flags & find_flags_t::REGEX) {
		pcre_extra extra;
		int pcre_flags = PCRE_NOTEMPTY;
		/* FIXME: shouldn't we be using the ovector instead of local_ovector? The main question is
		   whether the 0 and 1 (the complete match found) is correct. This is most important for
		   backward matches. */
		int local_ovector[30];

		ovector[0] = -1;
		ovector[1] = -1;
		found = false;

		if (backward) {
			start = result->end;
			end = result->start;
		} else {
			start = result->start;
			end = result->end;
		}

		extra.flags = PCRE_EXTRA_CALLOUT_DATA;
		extra.callout_data = this;
		//FIXME: doesn't work for backward!
		if ((size_t) end >= haystack->size())
			end = haystack->size();
		else
			pcre_flags |= PCRE_NOTEOL;

		if (start != 0)
			pcre_flags |= PCRE_NOTBOL;

		pcre_callout = callout;
		match_result = pcre_exec(regex, &extra, haystack->data(), end, start, pcre_flags,
			(int *) local_ovector, sizeof(local_ovector) / sizeof(local_ovector[0]));
		if (!found)
			return false;
		result->start = ovector[0];
		result->end = ovector[1];
		//FIXME: save local_ovector results, if we don't use ovector directly
		return true;
	} else {
		string substr;
		int curr_char, next_char;
		size_t c_size;
		const char *c;

		start = result->start >= 0 && (size_t) result->start > haystack->size() ? haystack->size() : result->start;
		curr_char = start;

		if (backward) {
			matcher->reset();
			while((size_t) curr_char > 0) {
				next_char = adjust_position(haystack, curr_char, -1);
				substr.clear();
				substr = haystack->substr(next_char, curr_char - next_char);
				if (flags & find_flags_t::ICASE) {
					c_size = t3_unicode_casefold(substr.data(), substr.size(), &folded, &folded_size, t3_false);
					c = folded;
				} else {
					c = substr.data();
					c_size = substr.size();
				}
				match_result = matcher->previous_char(c, c_size);
				if (match_result >= 0 &&
						(!(flags & find_flags_t::WHOLE_WORD) ||
						check_boundaries(haystack, next_char, adjust_position(haystack, start, -match_result))))
				{
					result->end = adjust_position(haystack, start, -match_result);
					result->start = next_char;
					return true;
				}
				curr_char = next_char;
			}
			return false;
		} else {
			matcher->reset();
			while((size_t) curr_char < haystack->size()) {
				next_char = adjust_position(haystack, curr_char, 1);
				substr.clear();
				substr = haystack->substr(curr_char, next_char - curr_char);
				if (flags & find_flags_t::ICASE) {
					c_size = t3_unicode_casefold(substr.data(), substr.size(), &folded, &folded_size, t3_false);
					c = folded;
				} else {
					c = substr.data();
					c_size = substr.size();
				}
				match_result = matcher->next_char(c, c_size);
				if (match_result >= 0 &&
						(!(flags & find_flags_t::WHOLE_WORD) ||
						check_boundaries(haystack, adjust_position(haystack, start, match_result), next_char)))
				{
					result->start = adjust_position(haystack, start, match_result);
					result->end = next_char;
					return true;
				}
				curr_char = next_char;
			}
			return false;
		}
	}
}

int find_context_t::callout(pcre_callout_block *block) {
	find_context_t *context = (find_context_t *) block->callout_data;
	if (block->pattern_position != context->pattern_length)
		return 0;

	if (context->flags & find_flags_t::BACKWARD) {
		if (block->start_match > context->ovector[0] ||
				(block->start_match == context->ovector[0] && block->current_position > context->ovector[1])) {
			memcpy(context->ovector, block->offset_vector, block->capture_top * sizeof(int) * 2);
			context->ovector[0] = block->start_match;
			context->ovector[1] = block->current_position;
			context->captures = block->capture_top;
			context->found = true;
		}
		return 1;
	} else {
		memcpy(context->ovector, block->offset_vector, block->capture_top * sizeof(int) * 2);
		context->ovector[0] = block->start_match;
		context->ovector[1] = block->current_position;
		context->captures = block->capture_top;
		context->found = true;
		return 0;
	}
}

static inline int is_start_char(int c) { return (c & 0x80) == 0 || (c & 0xc0) == 0x80; }

int find_context_t::adjust_position(const string *str, int pos, int adjust) {
	if (adjust > 0) {
		for (; adjust > 0 && (size_t) pos < str->size(); adjust -= is_start_char((*str)[pos]))
			pos++;
	} else {
		for (; adjust < 0 && pos > 0; adjust += is_start_char((*str)[pos])) {
			pos--;
			while (pos > 0 && !is_start_char((*str)[pos]))
				pos--;
		}
	}
	return pos;
}

bool find_context_t::check_boundaries(const string *str, int match_start, int match_end) {
	return (match_start == 0 || get_class(str, match_start) != get_class(str, adjust_position(str, match_start, -1))) &&
							(match_end == (int) str->size() || get_class(str, match_end) != get_class(str, adjust_position(str, match_end, 1)));
}

int find_context_t::get_class(const string *str, int pos) {
	size_t data_len = str->size() - pos;
	return t3_unicode_get_info(t3_unicode_get(str->data() + pos, &data_len), INT_MAX) &
		(T3_UNICODE_ALNUM_BIT | T3_UNICODE_GRAPH_BIT | T3_UNICODE_SPACE_BIT);
}

}; // namespace
