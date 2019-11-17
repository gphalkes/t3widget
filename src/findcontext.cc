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

#define PCRE2_CODE_UNIT_WIDTH 8

#include <cstdint>
#include <cstring>
#include <memory>
#ifdef PCRE_COMPAT
#include "t3widget/pcre_compat.h"
#else
#include <pcre2.h>
#endif
#include <string>
#include <unicase.h>

#include "t3widget/findcontext.h"
#include "t3widget/internal.h"
#include "t3widget/string_view.h"
#include "t3widget/stringmatcher.h"
#include "t3widget/util.h"
#include "widget_api.h"

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
  static text_pos_t adjust_position(const std::string &str, text_pos_t pos, int adjust);
  /** Check if the start and end of a match are on word boundaries.
      @param str The string to check.
      @param match_start The position of the start of the match in @p str.
      @param match_end The position of the end of the match in @p str.
  */
  bool check_boundaries(const std::string &str, text_pos_t match_start, text_pos_t match_end);
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
  struct pcre_code_free_deleter {
    void operator()(pcre2_code_8 *p) { pcre2_code_free_8(p); }
  };
  using unique_pcre_ptr = std::unique_ptr<pcre2_code_8, pcre_code_free_deleter>;
  struct pcre_match_data_free_deleter {
    void operator()(pcre2_match_data_8 *p) { pcre2_match_data_free_8(p); }
  };
  using unique_pcre_match_data_ptr =
      std::unique_ptr<pcre2_match_data_8, pcre_match_data_free_deleter>;

  /* PCRE context and data */
  /** Pointer to a compiled regex. */
  unique_pcre_ptr regex_;
  /** Structure to hold sub-matches information. */
  unique_pcre_match_data_ptr match_data_;
  /** Structure to hold sub-matches information when searching in reverse. */
  unique_pcre_match_data_ptr local_match_data_;

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
    result = t3widget::make_unique<regex_finder_t>(flags, replacement);
  } else {
    result = t3widget::make_unique<plain_finder_t>(flags, replacement);
  }
  if (!result->set_needle(needle, error_message)) {
    return nullptr;
  }
  // Using std::move here because some older C++11 compilers didn't correctly treat this as a move.
  return std::move(result);
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

  if (!(flags_ & find_flags_t::VALID)) {
    return false;
  }

  size_t c_size;

  text_pos_t start = std::max<text_pos_t>(0, result->start.pos);
  if (static_cast<size_t>(start) > haystack.size()) {
    start = static_cast<text_pos_t>(haystack.size());
  }
  text_pos_t end = result->end.pos < 0 || static_cast<size_t>(result->end.pos) > haystack.size()
                       ? static_cast<text_pos_t>(haystack.size())
                       : result->end.pos;
  if (reverse) {
    std::swap(start, end);
  }
  text_pos_t curr_char = start;

  if (reverse) {
    matcher->reset();
    while (curr_char > 0) {
      text_pos_t next_char = adjust_position(haystack, curr_char, -1);

      if (next_char < end) {
        return false;
      }

      string_view substr = string_view(haystack).substr(next_char, (curr_char - next_char));
      if (flags_ & find_flags_t::ICASE) {
        c_size = folded_size_;
        char *c_data = reinterpret_cast<char *>(
            u8_casefold(reinterpret_cast<const uint8_t *>(substr.data()), substr.size(), nullptr,
                        nullptr, reinterpret_cast<uint8_t *>(folded_.get()), &c_size));
        if (c_data != folded_.get()) {
          // Previous value of folded will be automatically deleted.
          folded_.reset(c_data);
          folded_size_ = c_size;
        }
        substr = string_view(c_data, c_size);
      }
      match_result = matcher->previous_char(substr);
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
      text_pos_t next_char = adjust_position(haystack, curr_char, 1);

      if (next_char > end) {
        return false;
      }

      string_view substr = string_view(haystack).substr(curr_char, (next_char - curr_char));
      if (flags_ & find_flags_t::ICASE) {
        c_size = folded_size_;
        char *c_data = reinterpret_cast<char *>(
            u8_casefold(reinterpret_cast<const uint8_t *>(substr.data()), substr.size(), nullptr,
                        nullptr, reinterpret_cast<uint8_t *>(folded_.get()), &c_size));
        if (c_data != folded_.get()) {
          // Previous value of folded will be automatically deleted.
          folded_.reset(c_data);
          folded_size_ = c_size;
        }
        substr = string_view(c_data, c_size);
      }
      match_result = matcher->next_char(substr);
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

text_pos_t plain_finder_t::adjust_position(const std::string &str, text_pos_t pos, int adjust) {
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

bool plain_finder_t::check_boundaries(const std::string &str, text_pos_t match_start,
                                      text_pos_t match_end) {
  if ((flags_ & find_flags_t::ANCHOR_WORD_LEFT) &&
      !(match_start == 0 ||
        get_class(str, match_start) != get_class(str, adjust_position(str, match_start, -1)))) {
    return false;
  }

  if ((flags_ & find_flags_t::ANCHOR_WORD_RIGHT) &&
      !(static_cast<size_t>(match_end) == str.size() ||
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
  int error_code;
  PCRE2_SIZE error_offset;
  int pcre_flags = PCRE2_UTF;

  std::string pattern = flags_ & find_flags_t::ANCHOR_WORD_LEFT ? "(?:\\b" : "(?:";
  pattern += needle;
  pattern += flags_ & find_flags_t::ANCHOR_WORD_RIGHT ? "\\b)" : ")";

  if (flags_ & find_flags_t::ICASE) {
    pcre_flags |= PCRE2_CASELESS;
  }

  regex_.reset(pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(pattern.c_str()), pattern.size(),
                               pcre_flags, &error_code, &error_offset, nullptr));
  if (regex_ == nullptr) {
    char buffer[256];
    pcre2_get_error_message_8(error_code, reinterpret_cast<PCRE2_UCHAR8 *>(buffer), sizeof(buffer));
    // FIXME: this should include the offset.
    *error_message = buffer;
    return false;
  }

  if (replacement_ != nullptr) {
    if (!parse_escapes(*replacement_, error_message, true)) {
      return false;
    }
    flags_ |= find_flags_t::REPLACEMENT_VALID;
  }
  flags_ |= find_flags_t::VALID;
  match_data_.reset(pcre2_match_data_create_from_pattern_8(regex_.get(), nullptr));
  if (match_data_ == nullptr) {
    *error_message = "Out of memory";
    return false;
  }
  pcre2_jit_compile_8(regex_.get(), PCRE2_JIT_COMPLETE);
  return true;
}

bool regex_finder_t::match(const std::string &haystack, find_result_t *result, bool reverse) {
  int match_result;

  if (!(flags_ & find_flags_t::VALID)) {
    return false;
  }

  int pcre_flags = PCRE2_NO_UTF_CHECK;
  found_ = false;

  PCRE2_SIZE start;
  PCRE2_SIZE end;
  bool may_not_match_end = true;
  bool may_not_match_start = true;

  if (static_cast<size_t>(result->end.pos) > haystack.size() || result->end.pos < 0) {
    end = haystack.size();
    may_not_match_end = false;
  } else if (static_cast<size_t>(result->end.pos) == haystack.size()) {
    end = haystack.size();
  } else {
    end = result->end.pos;
    pcre_flags |= PCRE2_NOTEOL;
  }

  if (result->start.pos < 0) {
    may_not_match_start = false;
    start = 0;
  } else {
    start = result->start.pos;
  }
  if (start != 0) {
    pcre_flags |= PCRE2_NOTBOL;
  }

  if (reverse) {
    if (local_match_data_ == nullptr) {
      local_match_data_.reset(pcre2_match_data_create_from_pattern_8(regex_.get(), nullptr));
      if (local_match_data_ == nullptr) {
        return false;
      }
    }
    while (start <= end) {
      match_result = pcre2_match_8(regex_.get(), reinterpret_cast<PCRE2_SPTR8>(haystack.data()),
                                   end, start, pcre_flags, local_match_data_.get(), nullptr);
      if (match_result < 0) {
        break;
      }
      const PCRE2_SIZE *local_ovector = pcre2_get_ovector_pointer_8(local_match_data_.get());
      const PCRE2_SIZE match_end = local_ovector[1];
      const bool found_this_iteration = may_not_match_end ? local_ovector[0] != end : true;
      found_ |= found_this_iteration;
      if (found_this_iteration) {
        std::swap(match_data_, local_match_data_);
        captures_ = match_result;
      }
      if (start == match_end) {
        PCRE2_SIZE new_start = text_line_t::adjust_position(haystack, start, 1);
        if (new_start == start) {
          break;
        }
        start = new_start;
      } else {
        start = text_pos_t(match_end);
      }
      pcre_flags |= PCRE2_NOTBOL;
    }
  } else {
    while (!found_ && start <= end) {
      match_result = pcre2_match_8(regex_.get(), reinterpret_cast<PCRE2_SPTR8>(haystack.data()),
                                   end, start, pcre_flags, match_data_.get(), nullptr);
      captures_ = match_result;
      if (match_result < 0) {
        break;
      }
      if (may_not_match_start) {
        may_not_match_start = false;
        found_ = pcre2_get_ovector_pointer_8(match_data_.get())[0] != start;
        if (!found_) {
          PCRE2_SIZE new_start = text_line_t::adjust_position(haystack, start, 1);
          if (new_start == start) {
            break;
          }
          start = new_start;
        }
      } else {
        found_ = true;
      }
      pcre_flags |= PCRE2_NOTBOL;
    }
  }
  if (!found_) {
    return false;
  }
  result->start.pos = text_pos_t(pcre2_get_ovector_pointer_8(match_data_.get())[0]);
  result->end.pos = text_pos_t(pcre2_get_ovector_pointer_8(match_data_.get())[1]);
  return true;
}

std::string regex_finder_t::get_replacement(const std::string &haystack) const {
  std::string retval(*replacement_);
  /* Replace the following strings with the matched items:
     EDA481 - EDA489. */
  size_t pos = 0;

  const PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_8(match_data_.get());
  while ((pos = retval.find("\xed\xa4", pos)) != std::string::npos) {
    if (pos + 3 > retval.size()) {
      retval.erase(pos, 2);
      break;
    }
    int capture_nr = retval[pos + 2] & 0x7f;
    if (captures_ > capture_nr) {
      retval.replace(pos, 3, haystack.data() + ovector[2 * capture_nr],
                     ovector[2 * capture_nr + 1] - ovector[2 * capture_nr]);
    } else {
      retval.erase(pos, 3);
    }
  }
  return retval;
}

}  // namespace t3widget
