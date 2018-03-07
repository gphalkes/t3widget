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
#ifndef T3_WIDGET_FINDCONTEXT_H
#define T3_WIDGET_FINDCONTEXT_H

#include <memory>
#include <pcre.h>
#include <string>

#include <t3widget/string_view.h>
#include <t3widget/stringmatcher.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>

namespace t3_widget {

/** A struct holding the result of a find operation.
    In the current situation, find operations work on a single line. Therefore
    this struct does not contain line references.
*/
struct T3_WIDGET_API find_result_t {
  text_coordinate_t start, end;
};

/** Class holding the context of a find operation. */
class T3_WIDGET_API finder_t {
 public:
  finder_t() = default;
  T3_WIDGET_DISALLOW_COPY(finder_t)

  /** Destroy a finder_t instance. */
  virtual ~finder_t();

  /** Try to find the previously set @c needle in a string. */
  virtual bool match(const std::string &haystack, find_result_t *result, bool reverse) = 0;
  /** Retrieve the flags set when setting the search context. */
  virtual int get_flags() const = 0;
  /** Retrieve the replacement string. */
  virtual std::string get_replacement(const std::string &haystack) const = 0;

  /** Creates a finder_t (or rather a subclass) with the given parameters.
      @param needle The string to search for.
      @param flags A logical or of flags from find_flags_t.
      @param replacement The optional replacement string.

      For regular expression searches, the replacement string may contain references of the form \0
      .. \9. */
  static std::unique_ptr<finder_t> create(const std::string &needle, int flags,
                                          const std::string *replacement = nullptr);
};

}  // namespace
#endif
