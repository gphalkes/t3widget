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

#ifndef T3_WIDGET_STRING_VIEW_H
#define T3_WIDGET_STRING_VIEW_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <limits>
#include <stddef.h>
#include <stdexcept>
#include <string>

#if defined(_T3_WIDGET_TEST) && __cplusplus >= 201703L
#define _T3_WIDGET_TEST_CONSTEXPR constexpr
#else
#define _T3_WIDGET_TEST_CONSTEXPR
#endif

namespace t3_widget {

// Simple basic_string_view implementation, which should be compatible with the C++17 standard
// version for most purposes. Once the minimum standard version is increased to C++17, it should
// be easy enough to replace this basic_string_view with std::basic_string_view.
template <class CharT, class Traits = std::char_traits<CharT>>
class basic_string_view {
 public:
  using traits_type = Traits;
  using value_type = CharT;
  using pointer = CharT *;
  using const_pointer = const CharT *;
  using const_iterator = const CharT *;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  constexpr basic_string_view() noexcept : data_(nullptr), size_(0) {}
  constexpr basic_string_view(const CharT *data) : data_(data), size_(Traits::length(data)) {}
  constexpr basic_string_view(const CharT *data, size_type size) : data_(data), size_(size) {}
  // Extra constructor necessary as std::string doesn't have the operator
  // version.
  basic_string_view(const std::basic_string<CharT, Traits> &str) noexcept
      : data_(str.data()), size_(str.size()) {}
  constexpr basic_string_view(const basic_string_view &view) noexcept = default;

  basic_string_view &operator=(const basic_string_view &view) noexcept = default;

  constexpr const_iterator begin() const noexcept { return data_; }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator end() const noexcept { return data_ + size_; }
  constexpr const_iterator cend() const noexcept { return end(); }

  constexpr const CharT &operator[](size_type pos) const { return data_[pos]; }

  _T3_WIDGET_TEST_CONSTEXPR const CharT &at(size_type pos) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    return data_[pos];
  }
  constexpr const CharT &front() const { return data_[0]; }
  constexpr const CharT &back() const { return data_[size_ - 1]; }

  constexpr const CharT *data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type length() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept { return npos - 1; }
  constexpr bool empty() const noexcept { return size_ == 0; }

  void clear() {
    data_ = "";
    size_ = 0;
  }
  void remove_prefix(size_type n) {
    data_ += n;
    size_ -= n;
  }
  void remove_suffix(size_type n) { size_ -= n; }
  void swap(basic_string_view &v) noexcept {
    std::swap(data_, v.data_);
    std::swap(size_, v.size_);
  }

  explicit operator std::basic_string<CharT>() const {
    return std::basic_string<CharT>(data_, size_);
  }

  size_type copy(CharT *dest, size_type count, size_type pos = 0) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    size_type to_copy = std::min(count, size_ - pos);
    Traits::copy(dest, data_ + pos, to_copy);
    return to_copy;
  }

  basic_string_view substr(size_type pos = 0, size_type count = npos) const {
    if (pos >= size_) throw std::out_of_range("Index out of range");
    return basic_string_view(data_ + pos, std::min(count, size_ - pos));
  }

  _T3_WIDGET_TEST_CONSTEXPR int compare(basic_string_view v) const noexcept {
    int cmp = Traits::compare(data_, v.data_, std::min(size_, v.size_));
    if (cmp == 0) {
      cmp = size_ < v.size_ ? -1 : (size_ > v.size_ ? 1 : 0);
    }
    return cmp;
  }
  constexpr int compare(size_type pos1, size_type count1, basic_string_view v) const {
    return substr(pos1, count1).compare(v);
  }
  constexpr int compare(size_type pos1, size_type count1, basic_string_view v, size_type pos2,
                        size_type count2) const {
    return substr(pos1, count1).compare(v.substr(pos2, count2));
  }
  constexpr int compare(const CharT *s) const { return compare(basic_string_view(s)); }
  constexpr int compare(size_type pos1, size_type count1, const CharT *s) const {
    return substr(pos1, count1).compare(basic_string_view(s));
  }
  constexpr int compare(size_type pos1, size_type count1, const CharT *s, size_type count2) const {
    return substr(pos1, count1).compare(basic_string_view(s, count2));
  }

  // FIXME: this needs reimplementing using Traits
  _T3_WIDGET_TEST_CONSTEXPR size_type find(basic_string_view v, size_type pos = 0) const noexcept {
    const_iterator result = std::search(begin() + std::min(size(), pos), end(), v.begin(), v.end());
    return result == end() ? npos : result - begin();
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find(CharT c, size_type pos = 0) const noexcept {
    const_iterator stop = end();
    for (const_iterator ptr = begin() + std::min(pos, size()); ptr != stop; ++ptr) {
      if (Traits::eq(*ptr, c)) {
        return ptr - begin();
      }
    }
    return npos;
  }
  constexpr size_type find(const CharT *s, size_type pos, size_type count) const {
    return find(basic_string_view(s, count), pos);
  }
  constexpr size_type find(const CharT *s, size_type pos = 0) const {
    return find(basic_string_view(s), pos);
  }

  // FIXME: this needs reimplementing using Traits
  _T3_WIDGET_TEST_CONSTEXPR size_type rfind(basic_string_view v, size_type pos = 0) const noexcept {
    const_iterator end = begin() + std::min(size_, pos);
    const_iterator result = std::find_end(begin(), end, v.begin(), v.end());
    return result == end ? npos : result - begin();
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type rfind(CharT c, size_type pos = 0) const noexcept {
    const_iterator stop = begin() + pos;
    for (const_iterator ptr = end(); ptr != stop;) {
      --ptr;
      if (Traits::eq(*ptr, c)) {
        return ptr - begin();
      }
    }
    return npos;
  }
  constexpr size_type rfind(const CharT *s, size_type pos, size_type count) const {
    return rfind(basic_string_view(s, count), pos);
  }
  constexpr size_type rfind(const CharT *s, size_type pos = 0) const {
    return rfind(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_first_of(basic_string_view v, size_type pos = 0) const
      noexcept {
    for (size_type x = pos; x < size_; x++) {
      if (v.find_first_of(data_[x]) != npos) return x;
    }
    return npos;
  }
  constexpr size_type find_first_of(CharT c, size_type pos = 0) const noexcept {
    return find(c, pos);
  }
  constexpr size_type find_first_of(const CharT *s, size_type pos, size_type count) const {
    return find_first_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_first_of(const CharT *s, size_type pos = 0) const {
    return find_first_of(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_first_not_of(basic_string_view v,
                                                        size_type pos = 0) const noexcept {
    for (size_type x = pos; x < size_; x++) {
      if (v.find_first_of(data_[x]) == npos) return x;
    }
    return npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept {
    for (size_type x = pos; x < size_; x++) {
      if (!Traits::eq(data_[x], c)) return x;
    }
    return npos;
  }
  constexpr size_type find_first_not_of(const CharT *s, size_type pos, size_type count) const {
    return find_first_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_first_not_of(const CharT *s, size_type pos = 0) const {
    return find_first_not_of(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_of(basic_string_view v, size_type pos = 0) const
      noexcept {
    if (size_ == 0) return npos;
    size_type x = size_ - 1;
    for (; x > pos; x--) {
      if (v.find_first_of(data_[x]) != npos) return x;
    }
    return x == pos ? (v.find_first_of(data_[x]) != npos ? x : npos) : npos;
  }
  constexpr size_type find_last_of(CharT c, size_type pos = 0) const noexcept {
    return rfind(c, pos);
  }
  constexpr size_type find_last_of(const CharT *s, size_type pos, size_type count) const {
    return find_last_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_last_of(const CharT *s, size_type pos = 0) const {
    return find_last_of(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_not_of(basic_string_view v, size_type pos = 0) const
      noexcept {
    if (size_ == 0) return npos;
    size_type x = size_ - 1;
    for (; x > pos; x--) {
      if (v.find_first_of(data_[x]) == npos) return x;
    }
    return x == pos ? (v.find_first_of(data_[x]) == npos ? x : npos) : npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_not_of(CharT c, size_type pos = 0) const noexcept {
    if (size_ == 0) return npos;
    size_type x = size_ - 1;
    for (; x < size_; x--) {
      if (!Traits::eq(data_[x], c)) return x;
    }
    return x == pos && !Traits::eq(data_[x], c) ? x : npos;
  }
  constexpr size_type find_last_not_of(const CharT *s, size_type pos, size_type count) const {
    return find_last_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_last_not_of(const CharT *s, size_type pos = 0) const {
    return find_last_not_of(basic_string_view(s), pos);
  }

  // FIXME: add the following functions:
  // constexpr bool starts_with(basic_string_view x) const noexcept;
  // constexpr bool starts_with(CharT x) const noexcept;
  // constexpr bool starts_with(const CharT* x) const;
  // constexpr bool ends_with(basic_string_view x) const noexcept;
  // constexpr bool ends_with(CharT x) const noexcept;
  // constexpr bool ends_with(const CharT* x) const;

  static constexpr size_type npos = std::numeric_limits<size_type>::max();

 private:
  const CharT *data_;
  size_type size_;
};

template <class CharT, class Traits>
constexpr
    typename basic_string_view<CharT, Traits>::size_type basic_string_view<CharT, Traits>::npos;

template <class CharT, class Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                         basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                         basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs, const CharT *rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
constexpr bool operator==(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) == 0;
}

template <class CharT, class Traits>
constexpr bool operator!=(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) != 0;
}

template <class CharT, class Traits>
constexpr bool operator<(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

template <class CharT, class Traits>
constexpr bool operator<=(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) >= 0;
}

template <class CharT, class Traits>
constexpr bool operator>(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) < 0;
}

template <class CharT, class Traits>
constexpr bool operator>=(const CharT *lhs, basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) <= 0;
}

template <class CharT, class Traits>
constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                          const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                          const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                         const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                          const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                         const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                          const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
constexpr bool operator==(const std::basic_string<CharT, Traits> &lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) == 0;
}

template <class CharT, class Traits>
constexpr bool operator!=(const std::basic_string<CharT, Traits> &lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) != 0;
}

template <class CharT, class Traits>
constexpr bool operator<(const std::basic_string<CharT, Traits> &lhs,
                         basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

template <class CharT, class Traits>
constexpr bool operator<=(const std::basic_string<CharT, Traits> &lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) >= 0;
}

template <class CharT, class Traits>
constexpr bool operator>(const std::basic_string<CharT, Traits> &lhs,
                         basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) < 0;
}

template <class CharT, class Traits>
constexpr bool operator>=(const std::basic_string<CharT, Traits> &lhs,
                          basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) <= 0;
}

using string_view = basic_string_view<char>;

}  // namespace t3_widget

#endif
