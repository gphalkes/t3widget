/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include <climits>
#include <cstring>
#include <t3window/utf8.h>
#include <unicase.h>

#include "t3widget/findcontext.h"
#include "t3widget/internal.h"
#include "t3widget/log.h"
#include "t3widget/stringmatcher.h"
#include "t3widget/util.h"

namespace t3widget {

class T3_WIDGET_LOCAL finder_base_t : public finder_t {
 public:
  /** Set the needle.
      @returns Whether the operation was successful. If unsuccessful, error_message will contain a
     description of the error. */
  virtual bool set_needle(const std::string &needle, std::string *error_message) = 0;

 protected:
  /** Create a new empty finder_t. */
  finder_base_t(int flags, const std::string *replacement) : flags_(flags) {
    if (replacement) {
      replacement_.reset(new std::string(*replacement));
    }
  }

  /** Flags indicating what type of search was requested. */
  int flags_;

  /** Replacement string. */
  std::unique_ptr<std::string> replacement_;

 private:
  int get_flags() const override { return flags_; }
};

class T3_WIDGET_LOCAL plain_finder_t : public finder_base_t {
 public:
  /** Create a new full_finder_t for a specific search.
      May throw a @c const @c char pointer holding an error message. Caller
      of this constructor remains owner of passed objects. */
  plain_finder_t(int flags, const std::string *replacement);

  bool set_needle(const std::string &needle, std::string *error_message) override;

  /** Try to find the previously set @c needle in a string. */
  bool match(const std::string &haystack, find_result_t *result, bool reverse) override;
  /** Retrieve the replacement string. */
  std::string get_replacement(const std::string &haystack) const override;

 private:
  /** Pointer to a string_matcher_t, if a non-regex search was requested. */
  std::unique_ptr<string_matcher_t> matcher;

  /** Space to store the case-folded representation of a single character. Allocation is handled by
      the unistring library, hence we can not use string or vector. */
  std::unique_ptr<char, free_deleter> folded_;
  /** Size of the full_finder_t::folded buffer. */
  size_t folded_size_;

  /** Get the next position of a UTF-8 character. */
  static int adjust_position(const std::string &str, int pos, int adjust);
  /** Check if the start and end of a match are on word boundaries.
      @param str The string to check.
      @param match_start The position of the start of the match in @p str.
      @param match_end The position of the end of the match in @p str.
  */
  bool check_boundaries(const std::string &str, int match_start, int match_end);
};

/** Implementation of the finder_t interface for regular expression based searches. */
class T3_WIDGET_LOCAL regex_finder_t : public finder_base_t {
 public:
  /** Create a new full_finder_t for a specific search.
      May throw a @c const @c char pointer holding an error message. Caller
      of this constructor remains owner of passed objects. */
  regex_finder_t(int flags, const std::string *replacement);

  bool set_needle(const std::string &needle, std::string *error_message) override;

  /** Try to find the previously set @c needle in a string. */
  bool match(const std::string &haystack, find_result_t *result, bool reverse) override;
  /** Retrieve the replacement string. */
  std::string get_replacement(const std::string &) const override;

 private:
  struct pcre_free_deleter {
    void operator()(pcre *p) { pcre_free(p); }
  };
  using unique_pcre_ptr = std::unique_ptr<pcre, pcre_free_deleter>;

  /* PCRE context and data */
  /** Pointer to a compiled regex. */
  unique_pcre_ptr regex_;
  /** Array to hold sub-matches information. */
  int ovector_[30];
  /** The number of sub-matches captured. */
  int captures_;
  bool found_; /**< Boolean indicating whether the regex match was successful. */
};
//================================= finder_t implementation ========================================
finder_t::~finder_t() {}

std::unique_ptr<finder_t> finder_t::create(const std::string &needle, int flags,
                                           std::string *error_message,
                                           const std::string *replacement) {
  std::unique_ptr<finder_base_t> result;
  if (flags & find_flags_t::REGEX) {
    result = make_unique<regex_finder_t>(flags, replacement);
  } else {
    result = make_unique<plain_finder_t>(flags, replacement);
  }
  if (!result->set_needle(needle, error_message)) {
    return nullptr;
  }
  return result;
}

//================================= plain_finder_t implementation ==================================
plain_finder_t::plain_finder_t(int flags, const std::string *replacement)
    : finder_base_t(flags, replacement), folded_size_(0) {}

bool plain_finder_t::set_needle(const std::string &needle, std::string *error_message) {
  /* Create a copy of needle, for transformation purposes. */
  std::string search_for(needle);

  if (flags_ & find_flags_t::TRANSFROM_BACKSLASH) {
    if (!parse_escapes(search_for, error_message)) {
      return false;
    }
  }

  if (flags_ & find_flags_t::ICASE) {
    size_t folded_needle_size;
    std::unique_ptr<char, free_deleter> folded_needle;

    folded_needle.reset(reinterpret_cast<char *>(
        u8_casefold(reinterpret_cast<const uint8_t *>(search_for.data()), search_for.size(),
                    nullptr, nullptr, nullptr, &folded_needle_size)));
    matcher.reset(new string_matcher_t(string_view(folded_needle.get(), folded_needle_size)));
  } else {
    matcher.reset(new string_matcher_t(search_for));
  }

  if (replacement_ != nullptr) {
    if ((flags_ & find_flags_t::TRANSFROM_BACKSLASH)) {
      if (!parse_escapes(*replacement_, error_message, false)) {
        return false;
      }
    }
    flags_ |= find_flags_t::REPLACEMENT_VALID;
  }
  flags_ |= find_flags_t::VALID;
  return true;
}

bool plain_finder_t::match(const std::string &haystack, find_result_t *result, bool reverse) {
  int match_result;
  int start;

  if (!(flags_ & find_flags_t::VALID)) {
    return false;
  }

  std::string substr;
  int curr_char, next_char;
  size_t c_size;
  const char *c;

  if (reverse) {
    start = result->end.pos >= 0 && static_cast<size_t>(result->end.pos) > haystack.size()
                ? haystack.size()
                : static_cast<size_t>(result->end.pos);
  } else {
    start = result->start.pos >= 0 && static_cast<size_t>(result->start.pos) > haystack.size()
                ? haystack.size()
                : static_cast<size_t>(result->start.pos);
  }
  curr_char = start;

  if (reverse) {
    matcher->reset();
    while (curr_char > 0) {
      next_char = adjust_position(haystack, curr_char, -1);

      if (next_char < result->start.pos) {
        return false;
      }

      substr.clear();
      substr = haystack.substr(next_char, curr_char - next_char);
      if (flags_ & find_flags_t::ICASE) {
        c_size = folded_size_;
        char *c_data = reinterpret_cast<char *>(
            u8_casefold(reinterpret_cast<const uint8_t *>(substr.data()), substr.size(), nullptr,
                        nullptr, reinterpret_cast<uint8_t *>(folded_.get()), &c_size));
        c = c_data;
        if (c_data != folded_.get()) {
          // Previous value of folded will be automatically deleted.
          folded_.reset(c_data);
          folded_size_ = c_size;
        }
      } else {
        c = substr.data();
        c_size = substr.size();
      }
      match_result = matcher->previous_char(c, c_size);
      if (match_result >= 0 &&
          (!(flags_ & find_flags_t::WHOLE_WORD) ||
           check_boundaries(haystack, next_char,
                            adjust_position(haystack, start, -match_result)))) {
        result->end.pos = adjust_position(haystack, start, -match_result);
        result->start.pos = next_char;
        return true;
      }
      curr_char = next_char;
    }
    return false;
  } else {
    matcher->reset();
    while (static_cast<size_t>(curr_char) < haystack.size()) {
      next_char = adjust_position(haystack, curr_char, 1);

      if (next_char > result->end.pos) {
        return false;
      }

      substr.clear();
      substr = haystack.substr(curr_char, next_char - curr_char);
      if (flags_ & find_flags_t::ICASE) {
        c_size = folded_size_;
        char *c_data = reinterpret_cast<char *>(
            u8_casefold(reinterpret_cast<const uint8_t *>(substr.data()), substr.size(), nullptr,
                        nullptr, reinterpret_cast<uint8_t *>(folded_.get()), &c_size));
        c = c_data;
        if (c_data != folded_.get()) {
          // Previous value of folded will be automatically deleted.
          folded_.reset(c_data);
          folded_size_ = c_size;
        }
      } else {
        c = substr.data();
        c_size = substr.size();
      }
      match_result = matcher->next_char(c, c_size);
      if (match_result >= 0 &&
          (!(flags_ & find_flags_t::WHOLE_WORD) ||
           check_boundaries(haystack, adjust_position(haystack, start, match_result), next_char))) {
        result->start.pos = adjust_position(haystack, start, match_result);
        result->end.pos = next_char;
        return true;
      }
      curr_char = next_char;
    }
    return false;
  }
}

static inline int is_start_char(int c) { return (c & 0xc0) != 0x80; }

int plain_finder_t::adjust_position(const std::string &str, int pos, int adjust) {
  if (adjust > 0) {
    for (; adjust > 0 && static_cast<size_t>(pos) < str.size(); adjust -= is_start_char(str[pos])) {
      pos++;
    }
  } else {
    for (; adjust < 0 && pos > 0; adjust += is_start_char(str[pos])) {
      pos--;
      while (pos > 0 && !is_start_char(str[pos])) {
        pos--;
      }
    }
  }
  return pos;
}

bool plain_finder_t::check_boundaries(const std::string &str, int match_start, int match_end) {
  if ((flags_ & find_flags_t::ANCHOR_WORD_LEFT) &&
      !(match_start == 0 ||
        get_class(str, match_start) != get_class(str, adjust_position(str, match_start, -1)))) {
    return false;
  }

  if ((flags_ & find_flags_t::ANCHOR_WORD_RIGHT) &&
      !(match_end == static_cast<int>(str.size()) ||
        get_class(str, match_end) != get_class(str, adjust_position(str, match_end, -1)))) {
    return false;
  }
  return true;
}

std::string plain_finder_t::get_replacement(const std::string &) const { return *replacement_; }

//================================= regex_finder_t implementation ==================================
regex_finder_t::regex_finder_t(int flags, const std::string *replacement)
    : finder_base_t(flags, replacement) {}

bool regex_finder_t::set_needle(const std::string &needle, std::string *error_message) {
  const char *char_error_message;
  int error_offset;
  int pcre_flags = PCRE_UTF8;

  std::string pattern = flags_ & find_flags_t::ANCHOR_WORD_LEFT ? "(?:\\b" : "(?:";
  pattern += needle;
  pattern += flags_ & find_flags_t::ANCHOR_WORD_RIGHT ? "\\b)" : ")";

  if (flags_ & find_flags_t::ICASE) {
    pcre_flags |= PCRE_CASELESS;
  }

  regex_.reset(
      pcre_compile(pattern.c_str(), pcre_flags, &char_error_message, &error_offset, nullptr));
  if (regex_ == nullptr) {
    // FIXME: this should include the offset.
    *error_message = char_error_message;
    return false;
  }

  if (replacement_ != nullptr) {
    if (!parse_escapes(*replacement_, error_message, true)) {
      return false;
    }
    flags_ |= find_flags_t::REPLACEMENT_VALID;
  }
  flags_ |= find_flags_t::VALID;
  return true;
}

bool regex_finder_t::match(const std::string &haystack, find_result_t *result, bool reverse) {
  int match_result;
  int start, end;

  if (!(flags_ & find_flags_t::VALID)) {
    return false;
  }

  int pcre_flags = PCRE_NOTEMPTY | PCRE_NO_UTF8_CHECK;
  /* Because of the way we match backwards, we can't directly use the ovector
     in context. That gets destroyed by trying to find a further match if none
     exists. */
  int local_ovector[30];

  ovector_[0] = -1;
  ovector_[1] = -1;
  found_ = false;

  start = result->start.pos;
  end = result->end.pos;

  if (static_cast<size_t>(end) >= haystack.size()) {
    end = haystack.size();
  } else {
    pcre_flags |= PCRE_NOTEOL;
  }

  if (reverse) {
    do {
      match_result = pcre_exec(regex_.get(), nullptr, haystack.data(), end, start, pcre_flags,
                               local_ovector, sizeof(local_ovector) / sizeof(local_ovector[0]));
      if (match_result >= 0) {
        found_ = true;
        memcpy(ovector_, local_ovector, sizeof(ovector_));
        captures_ = match_result;
        start = ovector_[1];
      }
    } while (match_result >= 0);
  } else {
    match_result = pcre_exec(regex_.get(), nullptr, haystack.data(), end, start, pcre_flags,
                             ovector_, sizeof(ovector_) / sizeof(ovector_[0]));
    captures_ = match_result;
    found_ = match_result >= 0;
  }
  if (!found_) {
    return false;
  }
  result->start.pos = ovector_[0];
  result->end.pos = ovector_[1];
  return true;
}

#define REPLACE_CAPTURE(x)                                      \
  case x:                                                       \
    if (captures_ > x)                                          \
      retval.replace(pos, 3, haystack.data() + ovector_[2 * x], \
                     ovector_[2 * x + 1] - ovector_[2 * x]);    \
    else                                                        \
      retval.erase(pos, 3);                                     \
    break;

std::string regex_finder_t::get_replacement(const std::string &haystack) const {
  std::string retval(*replacement_);
  /* Replace the following strings with the matched items:
     EDA481 - EDA489. */
  size_t pos = 0;

  while ((pos = retval.find("\xed\xa4", pos)) != std::string::npos) {
    if (pos + 3 > retval.size()) {
      retval.erase(pos, 2);
      break;
    }
    int capture_nr = retval[pos + 2] & 0x7f;
    if (captures_ > capture_nr) {
      retval.replace(pos, 3, haystack.data() + ovector_[2 * capture_nr],
                     ovector_[2 * capture_nr + 1] - ovector_[2 * capture_nr]);
    } else {
      retval.erase(pos, 3);
    }
  }
  return retval;
}

}  // namespace t3widget
