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
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <t3window/window.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

#include <t3widget/signals.h>
#include <t3widget/string_view.h>
#include <t3widget/widget_api.h>

namespace t3widget {

#define T3_WIDGET_DISALLOW_COPY(c)  \
  c(const c &) = delete;            \
  c(c &&) = delete;                 \
  c &operator=(const c &) = delete; \
  c &operator=(c &&) = delete;

#if defined(__GNUC__) || defined(__clang__)
#define T3_WIDGET_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define T3_WIDGET_UNLIKELY(x) (bool)(x)
#endif

struct nullopt_t {
  // Constructor to allow pre Defect 253 compilers to compile the code as well.
  constexpr nullopt_t() {}
};
T3_WIDGET_API extern const nullopt_t nullopt;

namespace internal {
/* As it is impossible to simply disable certain functions, an indirect way needs to be used. In
   this case, we want to disable the copy/move constructors and assignment operators. The way this
   can be achieved is by deriving from a base class that explicitly deletes these functions, and not
   defining them ourselves. This also means that all the functionality of the class itself must be
   in a a separate base class. */
template <bool copy_construct, bool move_construct, bool copy_assign, bool move_assign>
struct enable_copy_assign;

#define _T3_WIDGET_ENABLE_COPY_ASSIGN(cc, mc, ca, ma, cc_en, mc_en, ca_en, ma_en) \
  template <>                                                                     \
  struct enable_copy_assign<cc, mc, ca, ma> {                                     \
    enable_copy_assign() = default;                                               \
    enable_copy_assign(const enable_copy_assign &) = cc_en;                       \
    enable_copy_assign(enable_copy_assign &&) = mc_en;                            \
    enable_copy_assign &operator=(const enable_copy_assign &) = ca_en;            \
    enable_copy_assign &operator=(enable_copy_assign &&) = ma_en;                 \
  }

_T3_WIDGET_ENABLE_COPY_ASSIGN(false, false, false, false, delete, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, false, false, true, delete, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, false, true, false, delete, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, false, true, true, delete, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, true, false, false, delete, default, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, true, false, true, delete, default, delete, default);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, true, true, false, delete, default, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(false, true, true, true, delete, default, delete, default);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, false, false, false, default, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, false, false, true, default, delete, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, false, true, false, default, delete, default, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, false, true, true, default, delete, default, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, true, false, false, default, default, delete, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, true, false, true, default, default, delete, default);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, true, true, false, default, default, default, delete);
_T3_WIDGET_ENABLE_COPY_ASSIGN(true, true, true, true, default, default, default, default);
#undef _T3_WIDGET_ENABLE_COPY_ASSIGN

/* Base class for the optional class, which defines all the functionality. This is separate from the
   actual optional class, because we want to disable copy/move constructors and assignment
   operators, depending on the template parameter. See the description of enable_copy_assign for
   more details.

   The interface of the class is meant to be a subset of the std::optional class, such that at some
   point in the future this may be replaced by std::optional. It only implements the checked value
   access though, to prevent accidental access to invalid values.
*/
template <typename T>
class optional_base {
 private:
  char value_[sizeof(T)]; /**< Value, if #initialized is @c true. */
  bool initialized_;      /**< Boolean indicating whether #value_ has been initialized. */

  T &&as_xvalue() { return std::move(*reinterpret_cast<T *>(value_)); }
  T &as_value() { return *reinterpret_cast<T *>(value_); }
  const T &as_const_value() const { return *reinterpret_cast<const T *>(value_); }

 public:
  constexpr optional_base() noexcept : initialized_(false) {}
  constexpr optional_base(nullopt_t) noexcept : initialized_(false) {}
  optional_base(const optional_base &other) : initialized_(other.initialized_) {
    if (initialized_) {
      new (value_) T(other.as_const_value());
    }
  }
  optional_base(optional_base &&other) : initialized_(other.initialized_) {
    if (initialized_) {
      new (value_) T(std::forward<T>(other.as_xvalue()));
    }
  }
  template <typename X = T,
            typename = typename std::enable_if<
                std::is_constructible<T, X &&>::value &&
                !std::is_same<optional_base<T>, typename std::decay<X>::type>::value>::type>
  optional_base(X &&value) : initialized_(true) {
    new (value_) T(std::forward<X>(value));
  }
  ~optional_base() {
    if (initialized_) as_value().~T();
  }
  bool is_valid() const { return initialized_; }
  void reset() {
    if (initialized_) as_value().~T();
    initialized_ = false;
  }
  T &value() {
    if (T3_WIDGET_UNLIKELY(!initialized_)) {
      throw(0);
    }
    return as_value();
  }
  const T &value() const {
    if (T3_WIDGET_UNLIKELY(!initialized_)) {
      throw(0);
    }
    return as_const_value();
  }
  optional_base &operator=(const optional_base &other) {
    if (initialized_) {
      if (other.initialized_) {
        as_value() = other.as_const_value();
      } else {
        as_value().~T();
        initialized_ = false;
      }
    } else if (other.initialized_) {
      new (value_) T(other.as_const_value());
      initialized_ = true;
    }
    return *this;
  }
  optional_base &operator=(optional_base &&other) {
    if (initialized_) {
      if (other.initialized_) {
        as_value() = std::forward<T>(other.as_xvalue());
      } else {
        as_value().~T();
        initialized_ = false;
      }
    } else if (other.initialized_) {
      new (value_) T(std::forward<T>(other.as_xvalue()));
      initialized_ = true;
    }
    return *this;
  }
  template <class X = T,
            typename = typename std::enable_if<
                std::is_constructible<T, X &&>::value && std::is_assignable<T, X &&>::value &&
                !std::is_same<optional_base<T>, typename std::decay<X>::type>::value>::type>
  optional_base &operator=(X &&value) {
    if (initialized_) {
      as_value() = std::forward<T>(value);
    } else {
      new (value_) T(std::forward<T>(value));
      initialized_ = true;
    }
  }
  template <typename X>
  T value_or(X &&dflt) const {
    if (initialized_) {
      return as_const_value();
    } else {
      return static_cast<T>(std::forward<X>(dflt));
    }
  }
};
}  // namespace internal

/** Class defining values with a separate validity check. */
template <typename T>
class T3_WIDGET_API optional
    : public internal::optional_base<T>,
      public internal::enable_copy_assign<
          std::is_copy_constructible<T>::value, std::is_move_constructible<T>::value,
          std::is_copy_assignable<T>::value, std::is_move_assignable<T>::value> {
 public:
  using internal::optional_base<T>::optional_base;
};

using optint = optional<int>;
/** Standard uninitialized @ref optint value. */
T3_WIDGET_API extern const optint None;

/* Wrapper class which prevents accessing members in the pointed-to object when the object itself
   is const. This simulates the behavior of const pointers for objects like std::unique. Should be
   compatible with the std::experimental::propagate_const class from library fundamentals TSv2,
   although more limited in scope. */
template <typename T>
class propagate_const {
 public:
  using element_type = typename std::remove_reference<decltype(*std::declval<T &>())>::type;

  propagate_const() = default;
  propagate_const(propagate_const &&) = default;
  template <typename U>
  propagate_const(U &&u) : t_(std::forward<U>(u)) {}
  propagate_const(const propagate_const &) = delete;

  element_type *get() { t_.get(); }
  const element_type *get() const { t_.get(); }
  explicit operator bool() const { return static_cast<bool>(t_); }

  element_type &operator*() { return *t_.get(); }
  const element_type &operator*() const { return *t_.get(); }
  element_type *operator->() { return t_.get(); }
  const element_type *operator->() const { return t_.get(); }

  operator element_type *() { return t_.get(); }
  operator const element_type *() const { return t_.get(); }

 private:
  T t_;
};

/** Class to be used with unique_ptr etc. when only the destructor should be called, but no
    deallocation should be performed. This is used by the single-alloc multi-pimpl design in this
    library, where the implementations are allocated in a pool which is deallocated separately. */
struct no_dealloc_deleter {
  template <typename T>
  void operator()(T *t) {
    t->~T();
  }
};

/** Template for holding a pointer to an implementation object. This is intended for use with the
    impl_allocator_t, and only calls the destructor on the object it holds, but does not delete the
    object. */
template <typename T>
using single_alloc_pimpl_t = propagate_const<const std::unique_ptr<T, no_dealloc_deleter>>;

template <typename T>
using pimpl_t = propagate_const<const std::unique_ptr<T>>;

using text_pos_t = std::ptrdiff_t;

struct T3_WIDGET_API text_coordinate_t {
  text_coordinate_t() {}
  text_coordinate_t(text_pos_t _line, text_pos_t _pos) : line(_line), pos(_pos) {}
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
  text_pos_t line;
  text_pos_t pos;
};

#define T3_WIDGET_DECLARE_SIGNAL(_name, ...) \
  connection_t connect_##_name(std::function<void(__VA_ARGS__)> cb)

enum class selection_mode_t { NONE, SHIFT, MARK, ALL };

/* This uses a namespace like a type, to ensure that the flags don't end up in the default
   namespace. As they are used as integer constants, it is not practical to use an enum class. */
namespace find_flags_t {
enum {
  BACKWARD = (1 << 0),
  ICASE = (1 << 1),
  REGEX = (1 << 2),
  WRAP = (1 << 3),
  TRANSFROM_BACKSLASH = (1 << 4),
  WHOLE_WORD = (1 << 5) | (1 << 6),
  ANCHOR_WORD_LEFT = (1 << 5),
  ANCHOR_WORD_RIGHT = (1 << 6),
  VALID = (1 << 7),
  REPLACEMENT_VALID = (1 << 8)
};
}  // namespace find_flags_t

enum class find_action_t { FIND, SKIP, REPLACE, REPLACE_ALL, REPLACE_IN_SELECTION };

/** Constants for indicating which attribute to change/retrieve. */
enum class attribute_t {
  /** Attribute specifier for non-printable characters. */
  NON_PRINT,
  /** Attribute specifier for cursor when selecting text. */
  TEXT_SELECTION_CURSOR,
  /** Attribute specifier for cursor when selecting text in reverse direction. */
  TEXT_SELECTION_CURSOR2,
  /** Attribute specifier for text which the terminal is not able to draw correctly. */
  BAD_DRAW,
  // FIXME: document other attributes
  TEXT_CURSOR,
  TEXT,
  TEXT_SELECTED,
  HOTKEY_HIGHLIGHT,
  DIALOG,
  DIALOG_SELECTED,
  BUTTON_SELECTED,
  SCROLLBAR,
  MENUBAR,
  MENUBAR_SELECTED,
  BACKGROUND,
  SHADOW,
  META_TEXT
};

enum class rewrap_type_t { REWRAP_ALL, REWRAP_LINE, REWRAP_LINE_LOCAL, INSERT_LINES, DELETE_LINES };

enum class wrap_type_t { NONE, WORD, CHARACTER };

#undef _T3_WIDGET_ENUM

struct free_deleter {
  void operator()(void *val) { free(val); }
};

template <typename T, typename... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type make_unique(
    Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type wrap_unique(T *t) {
  return std::unique_ptr<T>(t);
}

T3_WIDGET_API ssize_t nosig_write(int fd, const char *buffer, size_t bytes);
T3_WIDGET_API ssize_t nosig_read(int fd, char *buffer, size_t bytes);

T3_WIDGET_API std::string get_working_directory();
T3_WIDGET_API std::string get_directory(string_view directory);
T3_WIDGET_API void sanitize_dir(std::string *directory);
T3_WIDGET_API bool is_dir(string_view current_dir, string_view name);

/** Utility function to convert a string to or from the LANG codeset. Does not return errors, but
    simply provides the best possible conversion. */
T3_WIDGET_API std::string convert_lang_codeset(string_view str, bool from);

namespace internal {
/** Template which forwards the type it is instantiated with. This prevents the type from
    participating in type deduction, allowing multiple types in the template to use the same
    template paramter, even though one is a base class of the other. */
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

class call_on_return_t {
 public:
  call_on_return_t(std::function<void()> cb) : cb_(std::move(cb)) {}

  ~call_on_return_t() {
    if (cb_) {
      cb_();
    }
  }

  std::function<void()> release() {
    std::function<void()> result = std::move(cb_);
    cb_ = std::function<void()>();
    return result;
  }

 private:
  std::function<void()> cb_;
};

}  // namespace t3widget

#endif
