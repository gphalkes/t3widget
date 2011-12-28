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
#include <climits>
#include <t3window/utf8.h>
#include <unicase.h>

#include "findcontext.h"
#include "util.h"
#include "internal.h"
#include "log.h"

using namespace std;
namespace t3_widget {

/* This function exists as a wrapper around pcre_free, because pcre_free is not
   a function but a variable, and therefore can not be passed as a template
   parameter. */
void finder_t::call_pcre_free(pcre *p) {
	pcre_free(p);
}

finder_t::finder_t(void) : flags(0), folded_size(0) {}

finder_t::finder_t(const string *needle, int _flags, const string *_replacement) :
		flags(_flags), folded_size(0)
{
	const char *error_message;
	if (flags & find_flags_t::REGEX) {
		int error_offset;
		int pcre_flags = PCRE_UTF8;

		string pattern = flags & find_flags_t::ANCHOR_WORD_LEFT ? "(?:\\b" : "(?:";
		pattern += *needle;
		pattern += flags & find_flags_t::ANCHOR_WORD_RIGHT ? "\\b)" : ")";

		if (flags & find_flags_t::ICASE)
			pcre_flags |= PCRE_CASELESS;

		if ((regex = pcre_compile(pattern.c_str(), pcre_flags, &error_message, &error_offset, NULL)) == NULL)
			//FIXME: error offset should be added for clarity
			throw error_message;
	} else {
		/* Create a copy of needle, for transformation purposes. */
		string search_for(*needle);

		if (flags & find_flags_t::TRANSFROM_BACKSLASH) {
			if (!parse_escapes(search_for, &error_message))
				throw error_message;
		}

		if (flags & find_flags_t::ICASE) {
			size_t folded_needle_size;
			cleanup_free_ptr<char>::t folded_needle;

			folded_needle = (char *) u8_casefold((const uint8_t *) search_for.data(), search_for.size(),
				NULL, NULL, NULL, &folded_needle_size);
			/* When passing a const char * to string_matcher_t, it takes responsibility for
			   de-allocation. */
			matcher = new string_matcher_t(folded_needle, folded_needle_size);
			/* Avoid de-allocation of folded_needle if creating the string matcher succeeds. */
			folded_needle = NULL;
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
		flags |= find_flags_t::REPLACEMENT_VALID;
	}
	flags |= find_flags_t::VALID;
}

finder_t::~finder_t(void) {}

finder_t &finder_t::operator=(finder_t& other) {
	if (&other == this)
		return *this;

	delete matcher;
	delete replacement;
	if (regex != NULL)
		pcre_free(regex);

	flags = other.flags;
	matcher = other.matcher();
	regex = other.regex();
	memcpy(ovector, other.ovector, sizeof(ovector));
	captures = other.captures;
	found = other.found;
	replacement = other.replacement();

	other.matcher = NULL;
	other.regex = NULL;
	other.replacement = NULL;
	return *this;
}

void finder_t::set_context(const string *needle, int _flags, const string *_replacement) {
	/* If this initialization fails, it will throw a message pointer. */
	finder_t new_context(needle, _flags, _replacement);
	*this = new_context;
}

bool finder_t::match(const string *haystack, find_result_t *result, bool reverse) {
	int match_result;
	int start, end;

	if (!(flags & find_flags_t::VALID))
		return false;

	if (flags & find_flags_t::REGEX) {
		int pcre_flags = PCRE_NOTEMPTY | PCRE_NO_UTF8_CHECK;
		/* Because of the way we match backwards, we can't directly use the ovector
		   in context. That gets destroyed by trying to find a further match if none
		   exists. */
		int local_ovector[30];

		ovector[0] = -1;
		ovector[1] = -1;
		found = false;

		start = result->start;
		end = result->end;

		if ((size_t) end >= haystack->size())
			end = haystack->size();
		else
			pcre_flags |= PCRE_NOTEOL;

		if (reverse) {
			do {
				match_result = pcre_exec(regex, NULL, haystack->data(), end, start, pcre_flags,
					local_ovector, sizeof(local_ovector) / sizeof(local_ovector[0]));
				if (match_result >= 0) {
					found = true;
					memcpy(ovector, local_ovector, sizeof(ovector));
					captures = match_result;
					start = ovector[1];
				}
			} while (match_result >= 0);
		} else {
			match_result = pcre_exec(regex, NULL, haystack->data(), end, start, pcre_flags,
				ovector, sizeof(ovector) / sizeof(ovector[0]));
			captures = match_result;
			found = match_result >= 0;
		}
		if (!found)
			return false;
		result->start = ovector[0];
		result->end = ovector[1];
		return true;
	} else {
		string substr;
		int curr_char, next_char;
		size_t c_size;
		const char *c;

		if (reverse)
			start = result->end >= 0 && (size_t) result->end > haystack->size() ? haystack->size() : (size_t) result->end;
		else
			start = result->start >= 0 && (size_t) result->start > haystack->size() ? haystack->size() : (size_t) result->start;
		curr_char = start;

		if (reverse) {
			matcher->reset();
			while((size_t) curr_char > 0) {
				next_char = adjust_position(haystack, curr_char, -1);

				if (next_char < result->start)
					return false;

				substr.clear();
				substr = haystack->substr(next_char, curr_char - next_char);
				if (flags & find_flags_t::ICASE) {
					c_size = folded_size;
					c = (char *) u8_casefold((const uint8_t *) substr.data(), substr.size(), NULL, NULL, (uint8_t *) (char *) folded, &c_size);
					if (c != folded) {
						lprintf("folded: %p, c: %p\n", (char *) folded, c);
						free(folded);
						folded = const_cast<char *>(c);
						folded_size = c_size;
					}
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

				if (next_char > result->end)
					return false;

				substr.clear();
				substr = haystack->substr(curr_char, next_char - curr_char);
				if (flags & find_flags_t::ICASE) {
					c_size = folded_size;
					c = (char *) u8_casefold((const uint8_t *) substr.data(), substr.size(), NULL, NULL, (uint8_t *) (char *) folded, &c_size);
					if (c != folded) {
						lprintf("folded: %p, c: %p\n", (char *) folded, c);
						free(folded);
						folded = const_cast<char *>(c);
						folded_size = c_size;
					}
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

static inline int is_start_char(int c) { return (c & 0x80) == 0 || (c & 0xc0) == 0x80; }

int finder_t::adjust_position(const string *str, int pos, int adjust) {
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

bool finder_t::check_boundaries(const string *str, int match_start, int match_end) {
	if ((flags & find_flags_t::ANCHOR_WORD_LEFT) &&
			!(match_start == 0 || get_class(str, match_start) !=
			get_class(str, adjust_position(str, match_start, -1))))
		return false;

	if ((flags & find_flags_t::ANCHOR_WORD_RIGHT) &&
			!(match_end == (int) str->size() || get_class(str, match_end) !=
			get_class(str, adjust_position(str, match_end, -1))))
		return false;
	return true;
}

int finder_t::get_flags(void) {
	return flags;
}

#define REPLACE_CAPTURE(x) \
	case x: \
		if (captures > x) \
			retval->replace(pos, 3, haystack->data() + ovector[2 * x], ovector[2 * x + 1] - ovector[2 * x]); \
		else \
			retval->erase(pos, 3); \
		break;

string *finder_t::get_replacement(const string *haystack) {
	string *retval = new string(*replacement);
	if (flags & find_flags_t::REGEX) {
		/* Replace the following strings with the matched items:
		   EDA481 - EDA489. */
		size_t pos = 0;

		while ((pos = retval->find("\xed\xa4", pos)) != string::npos) {
			if (pos + 3 > retval->size()) {
				retval->erase(pos, 2);
				break;
			}
			switch ((*retval)[pos + 2] & 0x7f) {
				REPLACE_CAPTURE(1)
				REPLACE_CAPTURE(2)
				REPLACE_CAPTURE(3)
				REPLACE_CAPTURE(4)
				REPLACE_CAPTURE(5)
				REPLACE_CAPTURE(6)
				REPLACE_CAPTURE(7)
				REPLACE_CAPTURE(8)
				REPLACE_CAPTURE(9)
				default:
					retval->erase(pos, 3);
					break;
			}
		}
	}
	return retval;
}

}; // namespace
