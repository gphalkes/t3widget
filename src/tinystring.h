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
#ifndef T3_WIDGET_TINYSTRING_H_
#define T3_WIDGET_TINYSTRING_H_

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <utility>

#include <t3widget/string_view.h>

namespace t3_widget {

// A string class with a tiny footprint. The size of the class itself is the size of a pointer. For
// very small strings, the data is stored in the bytes where the pointer is normally stored. In this
// case the lower bit of the pointer value is set. This also means this class only works if pointers
// of allocated data are at least even (which they are on pretty much all platforms).
// Contrary to the normal std::string class, this class does not maintain a nul byte at the end of
// the allocated data, and does not provide the c_str method.
class T3_WIDGET_API tiny_string_t {
 public:
  using value_type = char;
  using size_type = size_t;
  using iterator = char *;
  using const_iterator = const char *;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  tiny_string_t();
  tiny_string_t(string_view str);
  tiny_string_t(const char *str) : tiny_string_t(string_view(str)) {}
  tiny_string_t(const tiny_string_t &other);

  tiny_string_t(tiny_string_t &&other);

  ~tiny_string_t();

  tiny_string_t &operator=(const tiny_string_t &other) { return assign(other); }
  tiny_string_t &operator=(tiny_string_t &&other) { return assign(std::move(other)); }
  tiny_string_t &operator=(string_view str) { return assign(str); }
  tiny_string_t &operator=(char c) { return assign(1, c); }
  tiny_string_t &operator+=(string_view str) { return append(str); }
  tiny_string_t &operator+=(char c) { return append(1, c); }

  char &operator[](size_t idx);
  char operator[](size_t idx) const;

  explicit operator std::string() const { return std::string(data(), size()); }
  operator string_view() const { return string_view(data(), size()); }

  char &at(size_t idx);
  char at(size_t idx) const;

  tiny_string_t &insert(size_t index, size_t count, char ch);
  tiny_string_t &insert(size_t index, string_view str);

  tiny_string_t &append(size_t count, char ch);
  tiny_string_t &append(string_view str);

  tiny_string_t &assign(size_t count, char ch);
  tiny_string_t &assign(const tiny_string_t &str);
  tiny_string_t &assign(tiny_string_t &&str);
  tiny_string_t &assign(string_view str);

  tiny_string_t &replace(size_t pos, size_t count, size_t count2, char ch);
  tiny_string_t &replace(size_t pos, size_t count, string_view str);
  tiny_string_t &replace(const_iterator first, const_iterator last, string_view str);

  size_t find(char c, size_t pos = 0) const;
  size_t find(string_view str, size_t pos = 0) const;

  size_t rfind(char c, size_t pos = npos) const;
  size_t rfind(string_view str, size_t pos = npos) const;

  size_t find_first_of(char c, size_t pos = 0) const;
  size_t find_first_of(string_view str, size_t pos = 0) const;

  size_t find_first_not_of(char c, size_t pos = 0) const;
  size_t find_first_not_of(string_view str, size_t pos = 0) const;

  size_t find_last_of(char c, size_t pos = npos) const;
  size_t find_last_of(string_view str, size_t pos = npos) const;

  size_t find_last_not_of(char c, size_t pos = npos) const;
  size_t find_last_not_of(string_view str, size_t pos = npos) const;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;
  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;

  const char *data() const;
  char *data();

  void clear();
  size_t size() const;
  size_t length() const { return size(); }

  bool empty() const;

  void reserve(size_t reserved_size);

  void shrink_to_fit();

  // FIXME: implement the following functions:
  // front, back, max_size, capacity, erase, push_back, pop_back, starts_with, ends_with,
  // substr, copy, compare, resize, swap, comparison operators, operator>>, operator<<, hash

  static constexpr size_t npos = std::numeric_limits<size_t>::max();

 private:
  struct allocated_string_t {
    size_t allocated;
    size_t size;
    char data;
  };
  static_assert(!(alignof(allocated_string_t) & 1),
                "alignment requirement of size_t needs to be even");

  bool is_short() const;
  size_t short_size() const;

  char &mutable_signal_byte();
  char signal_byte() const;

  char *short_data_bytes();
  const char *short_data_bytes() const;
  char *mutable_data();

  /* Gets the pointer to the location where an append of size count must take place. Allocates
     extra memory if necessary. Updates size to indicate the new size. */
  char *get_append_dest(size_t count);

  /* Switches from short to allocated string. Does not change the size of the string. count
     must be at least the original size. */
  void switch_to_allocated(size_t count);
  void set_size_zero();
  char *prepare_replace(size_t pos, size_t count, size_t count2);
  void malloc_ptr(size_t size);
  void realloc_ptr(size_t size);

  static size_t max_short_size();

  union {
    char bytes[sizeof(allocated_string_t *)];
    allocated_string_t *ptr;
  };
};

T3_WIDGET_API inline bool operator==(const tiny_string_t &a, string_view b) {
  return string_view(a) == b;
}
T3_WIDGET_API inline bool operator!=(const tiny_string_t &a, string_view b) {
  return string_view(a) != b;
}
T3_WIDGET_API inline bool operator>(const tiny_string_t &a, string_view b) {
  return string_view(a) > b;
}
T3_WIDGET_API inline bool operator>=(const tiny_string_t &a, string_view b) {
  return string_view(a) >= b;
}
T3_WIDGET_API inline bool operator<(const tiny_string_t &a, string_view b) {
  return string_view(a) < b;
}
T3_WIDGET_API inline bool operator<=(const tiny_string_t &a, string_view b) {
  return string_view(a) <= b;
}

}  // namespace t3_widget
#endif
