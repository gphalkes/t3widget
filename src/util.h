/* Copyright (C) 2011-2013,2018 G.P. Halkes
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
#ifndef T3_WIDGET_UTIL_H
#define T3_WIDGET_UTIL_H
#include <cstdlib>
#include <memory>
#include <string>
#include <t3window/window.h>
#include <unistd.h>

#include <t3widget/signals.h>
#include <t3widget/widget_api.h>

namespace t3_widget {

#define T3_WIDGET_DISALLOW_COPY(c)  \
  c(const c &) = delete;            \
  c(c &&) = delete;                 \
  c &operator=(const c &) = delete; \
  c &operator=(c &&) = delete;

struct nullopt_t {
  // Constructor to allow pre Defect 253 compilers to compile the code as well.
  constexpr nullopt_t() {}
};
T3_WIDGET_API extern const nullopt_t nullopt;

/** Class defining values with a separate validity check. */
template <class T>
class T3_WIDGET_API optional {
  // FIXME: this should use placement new/delete to only initialize the value if it is actually used
  // FIXME: it would be good if this would be compatible with C++17 std::optional.
  // FIXME: this should provide a move constructor for the original type.
  // FIXME: this should either delete or provide copy and move constructors
 private:
  T value;          /**< Value, if #initialized is @c true. */
  bool initialized; /**< Boolean indicating whether #value has been initialized. */

 public:
  optional() : initialized(false) {}
  optional(nullopt_t) : initialized(false) {}
  optional(T _value) : value(_value), initialized(true) {}
  bool is_valid() const { return initialized; }
  void unset() { initialized = false; }
  operator T(void) const {
    if (!initialized) {
      throw(0);
    }
    return value;
  }
  const T &operator()() const {
    if (!initialized) {
      throw(0);
    }
    return value;
  }
  T &operator()() {
    if (!initialized) {
      throw(0);
    }
    return value;
  }
  const T *operator->() const {
    if (!initialized) {
      throw(0);
    }
    return &value;
  }
  T *operator->() {
    if (!initialized) {
      throw(0);
    }
    return &value;
  }
  optional &operator=(const optional &other) {
    initialized = other.initialized;
    value = other.value;
    return *this;
  }
  optional &operator=(const T other) {
    initialized = true;
    value = other;
    return *this;
  }
  T value_or_default(T dflt) { return initialized ? value : dflt; }
};

typedef optional<int> optint;
/** Standard uninitialized @ref optint value. */
T3_WIDGET_API extern const optint None;

struct T3_WIDGET_API text_coordinate_t {
  text_coordinate_t() {}
  text_coordinate_t(int _line, int _pos) : line(_line), pos(_pos) {}
  bool operator==(const text_coordinate_t &other) const {
    return line == other.line && pos == other.pos;
  }
  bool operator!=(const text_coordinate_t &other) const {
    return line != other.line || pos != other.pos;
  }
  bool operator>(const text_coordinate_t &other) const {
    return line > other.line || (line == other.line && pos > other.pos);
  }
  bool operator>=(const text_coordinate_t &other) const {
    return line > other.line || (line == other.line && pos >= other.pos);
  }
  bool operator<(const text_coordinate_t &other) const {
    return line < other.line || (line == other.line && pos < other.pos);
  }
  bool operator<=(const text_coordinate_t &other) const {
    return line < other.line || (line == other.line && pos <= other.pos);
  }
  int line;
  int pos;
};

#define T3_WIDGET_SIGNAL(_name, ...)                                     \
 protected:                                                              \
  signal_t<__VA_ARGS__> _name;                                           \
                                                                         \
 public:                                                                 \
  connection_t connect_##_name(std::function<void(__VA_ARGS__)> _slot) { \
    return _name.connect(_slot);                                         \
  }

#define _T3_WIDGET_ENUM(_name, ...)                                            \
  class T3_WIDGET_API _name {                                                  \
   public:                                                                     \
    enum _values { __VA_ARGS__ };                                              \
    _name() {}                                                                 \
    _name(_values _value_arg) : _value(_value_arg) {}                          \
    _values operator=(_values _value_arg) {                                    \
      _value = _value_arg;                                                     \
      return _value;                                                           \
    }                                                                          \
    operator int(void) const { return (int)_value; }                           \
    bool operator==(_values _value_arg) const { return _value == _value_arg; } \
                                                                               \
   private:                                                                    \
    _values _value;                                                            \
  }

_T3_WIDGET_ENUM(selection_mode_t, NONE, SHIFT, MARK, ALL);

_T3_WIDGET_ENUM(find_flags_t, BACKWARD = (1 << 0), ICASE = (1 << 1), REGEX = (1 << 2),
                WRAP = (1 << 3), TRANSFROM_BACKSLASH = (1 << 4), WHOLE_WORD = (1 << 5) | (1 << 6),
                ANCHOR_WORD_LEFT = (1 << 5), ANCHOR_WORD_RIGHT = (1 << 6), VALID = (1 << 7),
                REPLACEMENT_VALID = (1 << 8), );

_T3_WIDGET_ENUM(find_action_t, FIND, SKIP, REPLACE, REPLACE_ALL, REPLACE_IN_SELECTION);

/** Constants for indicating which attribute to change/retrieve. */
_T3_WIDGET_ENUM(attribute_t, NON_PRINT, TEXT_SELECTION_CURSOR, TEXT_SELECTION_CURSOR2, BAD_DRAW,
                TEXT_CURSOR, TEXT, TEXT_SELECTED, HOTKEY_HIGHLIGHT, DIALOG, DIALOG_SELECTED,
                BUTTON_SELECTED, SCROLLBAR, MENUBAR, MENUBAR_SELECTED, BACKGROUND, SHADOW,
                META_TEXT);
/** @var attribute_t::NON_PRINT
    Attribute specifier for non-printable characters. */
/** @var attribute_t::SELECTION_CURSOR
    Attribute specifier for cursor when selecting text. */
/** @var attribute_t::SELECTION_CURSOR2
    Attribute specifier for cursor when selecting text in reverse direction. */
/** @var attribute_t::BAD_DRAW
    Attribute specifier for text which the terminal is not able to draw correctly. */
// FIXME: list other attributes

_T3_WIDGET_ENUM(rewrap_type_t, REWRAP_ALL, REWRAP_LINE, REWRAP_LINE_LOCAL, INSERT_LINES,
                DELETE_LINES);

_T3_WIDGET_ENUM(wrap_type_t, NONE, WORD, CHARACTER);

#undef _T3_WIDGET_ENUM

struct free_deleter {
  void operator()(void *val) { free(val); }
};

template <typename T, typename... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type make_unique(
    Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

T3_WIDGET_API ssize_t nosig_write(int fd, const char *buffer, size_t bytes);
T3_WIDGET_API ssize_t nosig_read(int fd, char *buffer, size_t bytes);

T3_WIDGET_API std::string get_working_directory();
T3_WIDGET_API std::string get_directory(const char *directory);
T3_WIDGET_API void sanitize_dir(std::string *directory);
T3_WIDGET_API bool is_dir(const std::string *current_dir, const char *name);

T3_WIDGET_API void convert_lang_codeset(const char *str, size_t len, std::string *result,
                                        bool from);
T3_WIDGET_API void convert_lang_codeset(const char *str, std::string *result, bool from);
T3_WIDGET_API void convert_lang_codeset(const std::string *str, std::string *result, bool from);

namespace internal {
/* Template which forwards the type it is instantiated with. This prevents the type from
   participating in type deduction, allowing multiple types in the template to use the same template
   paramter, even though one is a base class of the other. */
template <typename C>
struct identity {
  typedef C type;
};
}  // namespace internal

template <typename R, typename A, typename... Args>
std::function<R(Args...)> bind_front(R (*f)(A, Args...), typename internal::identity<A>::type a) {
  return [=](Args... args) { return f(a, args...); };
}

template <typename R, typename A, typename B, typename... Args>
std::function<R(Args...)> bind_front(R (*f)(A, B, Args...), typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b) {
  return [=](Args... args) { return f(a, b, args...); };
}

template <typename R, typename A, typename B, typename C, typename... Args>
std::function<R(Args...)> bind_front(R (*f)(A, B, C, Args...),
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c) {
  return [=](Args... args) { return f(a, b, c, args...); };
}

template <typename R, typename A, typename B, typename C, typename D, typename... Args>
std::function<R(Args...)> bind_front(R (*f)(A, B, C, D, Args...),
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c,
                                     typename internal::identity<D>::type d) {
  return [=](Args... args) { return f(a, b, c, d, args...); };
}

template <typename R, typename A, typename... Args>
std::function<R(Args...)> bind_front(std::function<R(A, Args...)> f,
                                     typename internal::identity<A>::type a) {
  return [=](Args... args) { return f(a, args...); };
}

template <typename R, typename A, typename B, typename... Args>
std::function<R(Args...)> bind_front(std::function<R(A, B, Args...)> f,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b) {
  return [=](Args... args) { return f(a, b, args...); };
}

template <typename R, typename A, typename B, typename C, typename... Args>
std::function<R(Args...)> bind_front(std::function<R(A, B, C, Args...)> f,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c) {
  return [=](Args... args) { return f(a, b, c, args...); };
}

template <typename R, typename A, typename B, typename C, typename D, typename... Args>
std::function<R(Args...)> bind_front(std::function<R(A, B, C, D, Args...)> f,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c,
                                     typename internal::identity<D>::type d) {
  return [=](Args... args) { return f(a, b, c, d, args...); };
}

template <typename R, typename T, typename... Args>
std::function<R(Args...)> bind_front(R (T::*func)(Args...),
                                     typename internal::identity<T>::type *instance) {
  return [=](Args... args) { return (instance->*func)(args...); };
}

template <typename R, typename T, typename A, typename... Args>
std::function<R(Args...)> bind_front(R (T::*func)(A, Args...),
                                     typename internal::identity<T>::type *instance,
                                     typename internal::identity<A>::type a) {
  return [=](Args... args) { return (instance->*func)(a, args...); };
}

template <typename R, typename T, typename A, typename B, typename... Args>
std::function<R(Args...)> bind_front(R (T::*func)(A, B, Args...),
                                     typename internal::identity<T>::type *instance,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b) {
  return [=](Args... args) { return (instance->*func)(a, b, args...); };
}

template <typename R, typename T, typename A, typename B, typename C, typename... Args>
std::function<R(Args...)> bind_front(R (T::*func)(A, B, C, Args...),
                                     typename internal::identity<T>::type *instance,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c) {
  return [=](Args... args) { return (instance->*func)(a, b, c, args...); };
}

template <typename R, typename T, typename A, typename B, typename C, typename D, typename... Args>
std::function<R(Args...)> bind_front(R (T::*func)(A, B, C, D, Args...),
                                     typename internal::identity<T>::type *instance,
                                     typename internal::identity<A>::type a,
                                     typename internal::identity<B>::type b,
                                     typename internal::identity<C>::type c,
                                     typename internal::identity<D>::type d) {
  return [=](Args... args) { return (instance->*func)(a, b, c, d, args...); };
}

}  // namespace
#endif
