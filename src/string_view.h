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

#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <t3widget/widget_api.h>
#include <type_traits>

#if defined(_T3_WIDGET_TEST) && __cplusplus >= 201703L
#define _T3_WIDGET_TEST_CONSTEXPR constexpr
#else
#define _T3_WIDGET_TEST_CONSTEXPR
#endif

namespace t3widget {

/* Simple basic_string_view implementation, which should be compatible with the C++17 standard
   version for most purposes. Once the minimum standard version is increased to C++17, it should
   be easy enough to replace this basic_string_view with std::basic_string_view. Caveat: this also
   implements the starts_with_* and ends_with_* methods from the C++20 standard. */
template <class CharT, class Traits = std::char_traits<CharT>>
class T3_WIDGET_API basic_string_view {
 public:
  using traits_type = Traits;
  using value_type = CharT;
  using pointer = CharT *;
  using const_pointer = const CharT *;
  using reference = CharT &;
  using const_reference = const CharT &;
  using const_iterator = const CharT *;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  static_assert(std::is_same<CharT, typename Traits::char_type>::value,
                "Traits character type must match CharT.");
  static_assert(std::is_trivial<CharT>::value, "CharT must be trivial.");
  static_assert(!std::is_array<CharT>::value, "CharT may not be an array.");

  constexpr basic_string_view() noexcept : data_(nullptr), size_(0) {}
  constexpr basic_string_view(const CharT *data) : data_(data), size_(Traits::length(data)) {}
  constexpr basic_string_view(const CharT *data, size_type size) : data_(data), size_(size) {}
  // Extra constructor necessary as std::string doesn't have the operator version.
  basic_string_view(const std::basic_string<CharT, Traits> &str) noexcept
      : data_(str.data()), size_(str.size()) {}
  constexpr basic_string_view(const basic_string_view &view) noexcept = default;

  basic_string_view &operator=(const basic_string_view &view) noexcept = default;

  constexpr const_iterator begin() const noexcept { return data_; }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator end() const noexcept { return data_ + size_; }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator rbegin() const noexcept {
    return std::reverse_iterator<const_iterator>(end());
  }
  constexpr const_reverse_iterator rend() const noexcept {
    return std::reverse_iterator<const_iterator>(begin());
  }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

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

  _T3_WIDGET_TEST_CONSTEXPR void remove_prefix(size_type n) {
    data_ += n;
    size_ -= n;
  }
  _T3_WIDGET_TEST_CONSTEXPR void remove_suffix(size_type n) { size_ -= n; }
  _T3_WIDGET_TEST_CONSTEXPR void swap(basic_string_view &v) noexcept {
    {
      const CharT *tmp = data_;
      data_ = v.data_;
      v.data_ = tmp;
    }
    {
      size_type tmp = size_;
      size_ = v.size_;
      v.size_ = tmp;
    }
  }

  explicit operator std::basic_string<CharT, Traits>() const {
    return data_ ? std::basic_string<CharT, Traits>(data_, size_)
                 : std::basic_string<CharT, Traits>();
  }

  size_type copy(CharT *dest, size_type count, size_type pos = 0) const {
    if (pos > size_) throw std::out_of_range("Index out of range");
    size_type to_copy = std::min(count, size_ - pos);
    Traits::copy(dest, data_ + pos, to_copy);
    return to_copy;
  }

  _T3_WIDGET_TEST_CONSTEXPR basic_string_view substr(size_type pos = 0,
                                                     size_type count = npos) const {
    if (pos > size_) throw std::out_of_range("Index out of range");
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

  _T3_WIDGET_TEST_CONSTEXPR size_type find(basic_string_view v, size_type pos = 0) const noexcept {
    if (size_ == 0) {
      return v.size_ + pos == 0 ? 0 : npos;
    }
    for (const_iterator it = begin() + pos; it + v.size_ <= end(); ++it) {
      const_iterator left = it;
      for (const_iterator right = v.begin(); right != v.end() && Traits::eq(*right, *left);
           ++right, ++left) {
      }
      if (left == it + v.size_) {
        return it - begin();
      }
    }
    return npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find(CharT c, size_type pos = 0) const noexcept {
    if (size_ == 0) {
      return npos;
    }
    for (const_iterator ptr = begin() + pos; ptr < end(); ++ptr) {
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

  _T3_WIDGET_TEST_CONSTEXPR size_type rfind(basic_string_view v, size_type pos = npos) const
      noexcept {
    if (size_ == 0) {
      return v.size_ == 0 ? 0 : npos;
    }
    for (const_iterator it = begin() + std::min(size_ - v.size_, pos); it >= begin(); --it) {
      const_iterator left = it;
      for (const_iterator right = v.begin(); right != v.end() && Traits::eq(*right, *left);
           ++right, ++left) {
      }
      if (left == it + v.size_) {
        return it - begin();
      }
    }
    return npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type rfind(CharT c, size_type pos = npos) const noexcept {
    if (size_ == 0) return npos;
    for (const_iterator ptr = begin() + std::min(size_ - 1, pos) + 1; ptr != begin();) {
      --ptr;
      if (Traits::eq(*ptr, c)) return ptr - begin();
    }
    return npos;
  }
  constexpr size_type rfind(const CharT *s, size_type pos, size_type count) const {
    return rfind(basic_string_view(s, count), pos);
  }
  constexpr size_type rfind(const CharT *s, size_type pos = npos) const {
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
    for (const_iterator ptr = begin() + std::min(size_, pos); ptr != end(); ++ptr) {
      if (v.find_first_of(*ptr) == npos) return ptr - begin();
    }
    return npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept {
    for (const_iterator ptr = begin() + std::min(size_, pos); ptr != end(); ++ptr) {
      if (!Traits::eq(*ptr, c)) return ptr - begin();
    }
    return npos;
  }
  constexpr size_type find_first_not_of(const CharT *s, size_type pos, size_type count) const {
    return find_first_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_first_not_of(const CharT *s, size_type pos = 0) const {
    return find_first_not_of(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_of(basic_string_view v, size_type pos = npos) const
      noexcept {
    if (size_ == 0) return npos;
    for (const_iterator ptr = begin() + std::min(size_ - 1, pos) + 1; ptr != begin();) {
      --ptr;
      if (v.find_first_of(*ptr) != npos) return ptr - begin();
    }
    return npos;
  }
  constexpr size_type find_last_of(CharT c, size_type pos = npos) const noexcept {
    return rfind(c, pos);
  }
  constexpr size_type find_last_of(const CharT *s, size_type pos, size_type count) const {
    return find_last_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_last_of(const CharT *s, size_type pos = npos) const {
    return find_last_of(basic_string_view(s), pos);
  }

  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_not_of(basic_string_view v,
                                                       size_type pos = npos) const noexcept {
    if (size_ == 0) return npos;
    for (const_iterator ptr = begin() + std::min(size_ - 1, pos) + 1; ptr != begin();) {
      --ptr;
      if (v.find_first_of(*ptr) == npos) return ptr - begin();
    }
    return npos;
  }
  _T3_WIDGET_TEST_CONSTEXPR size_type find_last_not_of(CharT c, size_type pos = npos) const
      noexcept {
    if (size_ == 0) return npos;
    for (const_iterator ptr = begin() + std::min(size_ - 1, pos) + 1; ptr != begin();) {
      --ptr;
      if (!Traits::eq(*ptr, c)) return ptr - begin();
    }
    return npos;
  }
  constexpr size_type find_last_not_of(const CharT *s, size_type pos, size_type count) const {
    return find_last_not_of(basic_string_view(s, count), pos);
  }
  constexpr size_type find_last_not_of(const CharT *s, size_type pos = npos) const {
    return find_last_not_of(basic_string_view(s), pos);
  }

  constexpr bool starts_with(basic_string_view v) const noexcept {
    return size_ >= v.size_ && Traits::compare(data_, v.data_, v.size_) == 0;
  }
  constexpr bool starts_with(CharT c) const noexcept { return size_ >= 1 && Traits::eq(*data_, c); }
  constexpr bool starts_with(const CharT *s) const { return starts_with(basic_string_view(s)); }
  constexpr bool ends_with(basic_string_view v) const noexcept {
    return size_ >= v.size_ && Traits::compare(data_ + size_ - v.size_, v.data_, v.size_) == 0;
  }
  constexpr bool ends_with(CharT c) const noexcept {
    return size_ >= 1 && Traits::eq(data_[size_ - 1], c);
  }
  constexpr bool ends_with(const CharT *s) const { return ends_with(basic_string_view(s)); }

  static constexpr size_type npos = std::numeric_limits<size_type>::max();

 private:
  const CharT *data_;
  size_type size_;
};

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                        const CharT *rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                        const CharT *rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                       const CharT *rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                        const CharT *rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                       const CharT *rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                        const CharT *rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator==(const CharT *lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) == 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator!=(const CharT *lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) != 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<(const CharT *lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<=(const CharT *lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) >= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>(const CharT *lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) < 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>=(const CharT *lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) <= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                        const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                        const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                       const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                        const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                       const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                        const std::basic_string<CharT, Traits> &rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator==(const std::basic_string<CharT, Traits> &lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) == 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator!=(const std::basic_string<CharT, Traits> &lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) != 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<(const std::basic_string<CharT, Traits> &lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator<=(const std::basic_string<CharT, Traits> &lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) >= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>(const std::basic_string<CharT, Traits> &lhs,
                                       basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) < 0;
}

template <class CharT, class Traits>
T3_WIDGET_API constexpr bool operator>=(const std::basic_string<CharT, Traits> &lhs,
                                        basic_string_view<CharT, Traits> rhs) noexcept {
  return rhs.compare(lhs) <= 0;
}

template <class CharT, class Traits>
T3_WIDGET_API std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os,
                                                            basic_string_view<CharT, Traits> v) {
  return os.write(v.data(), v.size());
}

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

}  // namespace t3widget

namespace std {
template <>
struct T3_WIDGET_API hash<t3widget::string_view> {
  size_t operator()(t3widget::string_view v) const noexcept;
};
template <>
struct T3_WIDGET_API hash<t3widget::wstring_view> {
  size_t operator()(t3widget::wstring_view v) const noexcept;
};
template <>
struct T3_WIDGET_API hash<t3widget::u16string_view> {
  size_t operator()(t3widget::u16string_view v) const noexcept;
};
template <>
struct T3_WIDGET_API hash<t3widget::u32string_view> {
  size_t operator()(t3widget::u32string_view v) const noexcept;
};
}  // namespace std

#endif
