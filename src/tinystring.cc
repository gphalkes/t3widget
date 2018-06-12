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
#include "tinystring.h"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace t3widget {
namespace {

// This will be optimized to a constant by most compilers.
bool is_little_endian() {
  const union {
    unsigned u;
    unsigned char c;
  } one = {1};
  return one.c;
}

}  // namespace

tiny_string_t::tiny_string_t() { mutable_signal_byte() = 1; }

tiny_string_t::tiny_string_t(string_view str) {
  if (str.size() > max_short_size()) {
    malloc_ptr(str.size());
    ptr->allocated = str.size();
    ptr->size = str.size();
    std::memcpy(&ptr->data, str.data(), str.size());
  } else {
    mutable_signal_byte() = str.size() * 2 + 1;
    std::memcpy(short_data_bytes(), str.data(), str.size());
  }
}

tiny_string_t::tiny_string_t(const tiny_string_t &other) {
  if (other.is_short()) {
    std::memcpy(bytes, other.bytes, sizeof(bytes));
  } else {
    malloc_ptr(other.ptr->allocated);
    std::memcpy(ptr, other.ptr, other.ptr->size + sizeof(allocated_string_t) - 1);
  }
}

tiny_string_t::tiny_string_t(tiny_string_t &&other) {
  std::memcpy(bytes, other.bytes, sizeof(bytes));
  other.mutable_signal_byte() = 1;
}

tiny_string_t::~tiny_string_t() {
  if (!is_short()) {
    std::free(ptr);
  }
}

char &tiny_string_t::operator[](size_t idx) { return mutable_data()[idx]; }
char tiny_string_t::operator[](size_t idx) const { return data()[idx]; }

char &tiny_string_t::at(size_t idx) {
  if (idx >= size()) {
    throw std::out_of_range("Index out of range");
  }
  return mutable_data()[idx];
}

char tiny_string_t::at(size_t idx) const {
  if (idx >= size()) {
    throw std::out_of_range("Index out of range");
  }
  return data()[idx];
}

tiny_string_t &tiny_string_t::insert(size_t index, size_t count, char ch) {
  get_append_dest(count);
  char *insertion_point = mutable_data() + index;
  std::memmove(insertion_point + count, insertion_point, size() - index - count);
  for (; count > 0; --count, ++insertion_point) {
    *insertion_point = ch;
  }
  return *this;
}

tiny_string_t &tiny_string_t::insert(size_t index, string_view str) {
  get_append_dest(str.size());
  char *insertion_point = mutable_data() + index;
  std::memmove(insertion_point + str.size(), insertion_point, size() - index - str.size());
  std::memcpy(insertion_point, str.data(), str.size());
  return *this;
}

tiny_string_t &tiny_string_t::append(size_t count, char ch) {
  char *dest = get_append_dest(count);
  for (; count > 0; --count, ++dest) {
    *dest = ch;
  }
  return *this;
}

tiny_string_t &tiny_string_t::append(string_view str) {
  memcpy(get_append_dest(str.size()), str.data(), str.size());
  return *this;
}

tiny_string_t &tiny_string_t::assign(size_t count, char ch) {
  set_size_zero();
  return append(count, ch);
}

tiny_string_t &tiny_string_t::assign(string_view str) {
  set_size_zero();
  return append(str);
}

tiny_string_t &tiny_string_t::assign(const tiny_string_t &other) {
  if (other.is_short()) {
    if (!is_short()) {
      std::free(ptr);
    }
    std::memcpy(bytes, other.bytes, sizeof(bytes));
  } else {
    if (is_short()) {
      malloc_ptr(other.ptr->allocated);
    } else {
      realloc_ptr(other.ptr->allocated);
    }
    std::memcpy(ptr, other.ptr, other.ptr->size + sizeof(allocated_string_t) - 1);
  }
  return *this;
}

tiny_string_t &tiny_string_t::assign(tiny_string_t &&other) {
  if (!is_short()) {
    std::free(ptr);
  }
  std::memcpy(bytes, other.bytes, sizeof(bytes));
  return *this;
}

tiny_string_t &tiny_string_t::replace(size_t pos, size_t count, size_t count2, char ch) {
  char *dest = prepare_replace(pos, count, count2);
  for (; count2 > 0; --count2, ++dest) {
    *dest = ch;
  }
  return *this;
}

tiny_string_t &tiny_string_t::replace(size_t pos, size_t count, string_view str) {
  const size_t count2 = str.size();
  std::memcpy(prepare_replace(pos, count, count2), str.data(), count2);
  return *this;
}

tiny_string_t &tiny_string_t::replace(tiny_string_t::const_iterator first,
                                      tiny_string_t::const_iterator last, string_view str) {
  return replace(first - cbegin(), last - first, str);
}

tiny_string_t tiny_string_t::substr(size_t pos, size_t count) const {
  return tiny_string_t(string_view(*this).substr(pos, count));
}

int tiny_string_t::compare(const tiny_string_t &other) const {
  return string_view(*this).compare(other);
}

int tiny_string_t::compare(string_view other) const { return string_view(*this).compare(other); }

size_t tiny_string_t::find(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find(c, pos);
}

size_t tiny_string_t::find(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find(str, pos);
}

size_t tiny_string_t::rfind(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).rfind(c, pos);
}

size_t tiny_string_t::rfind(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).rfind(str, pos);
}

size_t tiny_string_t::find_first_of(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_first_of(c, pos);
}

size_t tiny_string_t::find_first_of(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_first_of(str, pos);
}

size_t tiny_string_t::find_first_not_of(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_first_not_of(c, pos);
}

size_t tiny_string_t::find_first_not_of(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_first_not_of(str, pos);
}

size_t tiny_string_t::find_last_of(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_last_of(c, pos);
}

size_t tiny_string_t::find_last_of(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_last_of(str, pos);
}

size_t tiny_string_t::find_last_not_of(char c, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_last_of(c, pos);
}

size_t tiny_string_t::find_last_not_of(string_view str, size_t pos) const {
  static_assert(npos == string_view::npos, "tiny_string_t::npos must match string_view::npos");
  return string_view(*this).find_last_of(str, pos);
}

tiny_string_t::iterator tiny_string_t::begin() { return mutable_data(); }
tiny_string_t::iterator tiny_string_t::end() { return begin() + size(); }
tiny_string_t::const_iterator tiny_string_t::begin() const { return data(); }
tiny_string_t::const_iterator tiny_string_t::end() const { return begin() + size(); }
tiny_string_t::const_iterator tiny_string_t::cbegin() const { return data(); }
tiny_string_t::const_iterator tiny_string_t::cend() const { return cbegin() + size(); }
tiny_string_t::reverse_iterator tiny_string_t::rbegin() {
  return std::reverse_iterator<iterator>(end());
}
tiny_string_t::reverse_iterator tiny_string_t::rend() {
  return std::reverse_iterator<iterator>(begin());
}
tiny_string_t::const_reverse_iterator tiny_string_t::rbegin() const {
  return std::reverse_iterator<const_iterator>(cend());
}
tiny_string_t::const_reverse_iterator tiny_string_t::rend() const {
  return std::reverse_iterator<const_iterator>(cbegin());
}
tiny_string_t::const_reverse_iterator tiny_string_t::crbegin() const {
  return std::reverse_iterator<const_iterator>(cend());
}
tiny_string_t::const_reverse_iterator tiny_string_t::crend() const {
  return std::reverse_iterator<const_iterator>(cbegin());
}

const char *tiny_string_t::data() const { return is_short() ? short_data_bytes() : &ptr->data; }
char *tiny_string_t::data() { return is_short() ? short_data_bytes() : &ptr->data; }

void tiny_string_t::clear() {
  if (is_short()) {
    mutable_signal_byte() = 1;
  } else {
    ptr->size = 0;
  }
}

size_t tiny_string_t::size() const {
  return is_short() ? static_cast<size_t>(signal_byte() / 2) : ptr->size;
}

bool tiny_string_t::empty() const { return signal_byte() == 1; }

void tiny_string_t::reserve(size_t reserved_size) {
  if (reserved_size <= std::numeric_limits<size_t>::max() - sizeof(allocated_string_t) &&
      reserved_size > max_short_size()) {
    if (is_short()) {
      switch_to_allocated(reserved_size);
    } else if (ptr->allocated < reserved_size) {
      realloc_ptr(reserved_size);
      ptr->allocated = reserved_size;
    }
  }
}

void tiny_string_t::shrink_to_fit() {
  if (!is_short()) {
    if (ptr->size > max_short_size()) {
      size_t original_size = ptr->size;
      char data[sizeof(allocated_string_t *)];
      memcpy(data, &ptr->data, sizeof(allocated_string_t *));
      std::free(ptr);
      memcpy(mutable_data(), data + static_cast<size_t>(is_little_endian()), original_size);
    } else {
      realloc_ptr(ptr->size);
    }
  }
}

bool tiny_string_t::is_short() const { return signal_byte() & 1; }

size_t tiny_string_t::short_size() const { return signal_byte() / 2; }

char &tiny_string_t::mutable_signal_byte() {
  return bytes[is_little_endian() ? 0 : max_short_size()];
}

char tiny_string_t::signal_byte() const { return bytes[is_little_endian() ? 0 : max_short_size()]; }

char *tiny_string_t::short_data_bytes() { return bytes + static_cast<size_t>(is_little_endian()); }

const char *tiny_string_t::short_data_bytes() const {
  return bytes + static_cast<size_t>(is_little_endian());
}

char *tiny_string_t::mutable_data() { return is_short() ? short_data_bytes() : &ptr->data; }

/* Gets the pointer to the location where an append of size count must take place. Allocates
   extra memory if necessary. Updates size to indicate the new size. */
char *tiny_string_t::get_append_dest(size_t count) {
  char *dest;
  if (is_short()) {
    size_t original_size = short_size();
    if (original_size + count < max_short_size()) {
      dest = short_data_bytes() + original_size;
      mutable_signal_byte() += count * 2;
    } else {
      switch_to_allocated(count + original_size);
      ptr->size += count;
      dest = &ptr->data + original_size;
    }
  } else {
    if (ptr->allocated < ptr->size + count) {
      do {
        if (std::numeric_limits<size_t>::max() / 2 > ptr->allocated) {
          ptr->allocated *= 2;
        } else {
          ptr->allocated = std::numeric_limits<size_t>::max() - sizeof(allocated_string_t);
          break;
        }
      } while (ptr->allocated < ptr->size + count);
      if (ptr->allocated < ptr->size + count) {
        throw std::length_error("tiny_string_t attempted to create too large string");
      }
      realloc_ptr(ptr->allocated);
    }
    dest = &ptr->data + ptr->size;
    ptr->size += count;
  }
  return dest;
}

/* Switches from short to allocated string. Does not change the size of the string. count
   must be at least the original size. */
void tiny_string_t::switch_to_allocated(size_t count) {
  size_t original_size = short_size();
  // This uses a buffer the size of the original string, as it allows faster copying and a
  // statically sized buffer.
  char data[sizeof(allocated_string_t *)];
  memcpy(data, bytes, sizeof(allocated_string_t *));
  malloc_ptr(count);
  memcpy(&ptr->data, data + static_cast<size_t>(is_little_endian()), original_size);
  ptr->size = original_size;
  ptr->allocated = count;
}

void tiny_string_t::set_size_zero() {
  if (is_short()) {
    mutable_signal_byte() = 1;
  } else {
    ptr->size = 0;
  }
}

char *tiny_string_t::prepare_replace(size_t pos, size_t count, size_t count2) {
  if (pos > size()) {
    throw std::out_of_range("Index out of range");
  }
  if (count2 > count) {
    get_append_dest(count2 - count);
  }
  if (count2 != count) {
    std::memmove(mutable_data() + pos + count2, mutable_data() + pos + count, size() - pos - count);
  }
  return mutable_data() + pos;
}

void tiny_string_t::malloc_ptr(size_t size) {
  ptr = static_cast<allocated_string_t *>(std::malloc(sizeof(allocated_string_t) - 1 + size));
}

void tiny_string_t::realloc_ptr(size_t size) {
  allocated_string_t *new_ptr =
      static_cast<allocated_string_t *>(std::realloc(ptr, sizeof(allocated_string_t) - 1 + size));
  if (new_ptr == nullptr) {
    throw std::bad_alloc();
  }
  ptr = new_ptr;
}

size_t tiny_string_t::max_short_size() { return sizeof(allocated_string_t *) - 1; }

}  // namespace t3widget

size_t std::hash<t3widget::tiny_string_t>::operator()(t3widget::tiny_string_t str) const noexcept {
  return std::hash<t3widget::string_view>()(str);
}
