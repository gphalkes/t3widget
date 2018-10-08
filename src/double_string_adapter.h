/* Copyright (C) 2018 G.P. Halkes
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
#ifndef T3_WIDGET_DOUBLE_STRING_ADAPTER_H
#define T3_WIDGET_DOUBLE_STRING_ADAPTER_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <t3widget/string_view.h>
#include <t3widget/tinystring.h>
#include <t3window/utf8.h>

namespace t3widget {

/* Class which allows storing two separate strings in a single tiny_string_t. This is used by the
   undo system to store the information for OVERWRITE actions.
   The layout of the data is as follows:
   - A UTF-8 encoded length of the first string.
   - The first string.
   - The second string.
   Note that this doesn't optimize in any way, and an append on the first string will result in
   moving bytes around. */
class double_string_adapter_t {
 public:
  double_string_adapter_t(tiny_string_t *str) : str_(str) {
    if (str_->empty()) {
      str_->assign(string_view("\0", 1));
      first_size_ = 0;
      first_start_ = 1;
    } else {
      size_t utf8_size = str_->size();
      first_size_ = t3_utf8_get(str_->data(), &utf8_size);
      first_start_ = utf8_size;
    }
  }

  string_view first() const { return string_view(str_->data() + first_start_, first_size_); }
  string_view second() const {
    const size_t offset = second_start();
    return string_view(str_->data() + offset, str_->size() - offset);
  }

  double_string_adapter_t &append_first(string_view str) {
    str_->insert(second_start(), str);
    first_size_ += str.size();

    char size_buffer[4];
    const size_t new_first_start_ = t3_utf8_put(first_size_, size_buffer);
    str_->replace(0, first_start_, string_view(size_buffer, new_first_start_));
    first_start_ = new_first_start_;
    return *this;
  }

  double_string_adapter_t &append_second(string_view str) {
    str_->append(str);
    return *this;
  }

 private:
  size_t second_start() const { return first_start_ + first_size_; }

  tiny_string_t *str_;
  size_t first_size_;
  size_t first_start_;
};

}  // namespace t3widget

#endif  // T3_WIDGET_DOUBLE_STRING_ADAPTER_H
