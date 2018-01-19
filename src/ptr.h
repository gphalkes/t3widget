/* Copyright (C) 2011-2012 G.P. Halkes
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
#ifndef T3_WIDGET_PTR_H
#define T3_WIDGET_PTR_H

// FIXME: pretty much everything in here should be replacable by std::unique_ptr and std::shared_ptr

#include <cstdlib>

#include <t3widget/widget_api.h>

namespace t3_widget {

template <typename T>
struct delete_functor {
  void operator()(T *p) { delete p; }
};

template <typename T>
struct delete_functor<T[]> {
  void operator()(T *p) { delete[] p; }
};

template <typename T = void, void (*f)(T *) = free>
struct free_func {
  void operator()(T *p) { f((T *)p); }
};

template <typename T, typename U, U (*f)(T *)>
struct free_func2 {
  void operator()(T *p) { f((T *)p); }
};

template <typename T>
class T3_WIDGET_API smartptr_base {
 public:
  T *operator->(void)const { return p_; }
  T &operator*(void)const { return *p_; }
  T *operator()(void) const { return p_; }
  operator T *(void) { return p_; }
  T *get() { return p_; }

 protected:
  smartptr_base() : p_(NULL) {}
  T *p_;
};

/* To allow typedef of the templates, we use the typedef in template class
   trick. For readability we include a macro for this purpose here. This will
   define a type t in the class name such that name<T>::t will be the type. This
   is sort of in line with the _t suffix we use for types.

   Because the template arguments that we want to pass as the last parameter will
   be considered separate macro parameters, we use a simple trick: we use a
   variadic macro, and use __VA_ARGS__ where we want the template. */
#define _T3_WIDGET_TYPEDEF(name, ...) \
  class T3_WIDGET_API name {          \
   public:                            \
    typedef __VA_ARGS__ t;            \
                                      \
   private:                           \
    name() {}                         \
  }

/* Pointer wrapper which automatically de-allocate its objects when going out
   of scope. The difference with the Boost scoped_ptr is that the objects can
   be released. Their main use is to ensure deallocation during exception
   handling, and storing of temporary values.

   Furthermore, these pointer wrappers are more generic than the std::auto_ptr,
   in that a functor can be passed to perform the clean-up, instead of
   requiring delete.
*/
#define _T3_WIDGET_DEFINE_CLEANUP_PTR                      \
 public:                                                   \
  ~cleanup_ptr_base() {                                    \
    if (smartptr_base<T>::p_ != NULL) {                    \
      D d;                                                 \
      d(smartptr_base<T>::p_);                             \
    }                                                      \
  }                                                        \
  cleanup_ptr_base() {}                                    \
  cleanup_ptr_base(T *p) { smartptr_base<T>::p_ = p; }     \
  T **operator&(void) { return &smartptr_base<T>::p_; }    \
  T *operator=(T *p) {                                     \
    if (smartptr_base<T>::p_ == p) return p;               \
    if (smartptr_base<T>::p_ != NULL) {                    \
      D d;                                                 \
      d(smartptr_base<T>::p_);                             \
    }                                                      \
    return smartptr_base<T>::p_ = p;                       \
  }                                                        \
  T *release() {                                           \
    T *p = smartptr_base<T>::p_;                           \
    smartptr_base<T>::p_ = NULL;                           \
    return p;                                              \
  }                                                        \
                                                           \
 private:                                                  \
  cleanup_ptr_base &operator=(const cleanup_ptr_base &p) { \
    (void)p;                                               \
    return *this;                                          \
  }                                                        \
  cleanup_ptr_base(const cleanup_ptr_base &p) { (void)p; }

template <typename T, typename D>
class T3_WIDGET_API cleanup_ptr_base : public smartptr_base<T> {
  _T3_WIDGET_DEFINE_CLEANUP_PTR
};

template <typename T, typename D>
class T3_WIDGET_API cleanup_ptr_base<T[], D> : public smartptr_base<T> {
  _T3_WIDGET_DEFINE_CLEANUP_PTR
};
#undef _T3_WIDGET_DEFINE_CLEANUP_PTR

/* We also typedef what would be the basic variant, such that all type names
   use the same pattern. */
template <typename T, typename D = delete_functor<T> >
_T3_WIDGET_TYPEDEF(cleanup_ptr, cleanup_ptr_base<T, D>);
template <typename T, void (*f)(T *)>
_T3_WIDGET_TYPEDEF(cleanup_func_ptr, cleanup_ptr_base<T, free_func<T, f> >);
template <typename T, typename U, U (*f)(T *)>
_T3_WIDGET_TYPEDEF(cleanup_func2_ptr, cleanup_ptr_base<T, free_func2<T, U, f> >);
template <typename T>
_T3_WIDGET_TYPEDEF(cleanup_free_ptr, cleanup_ptr_base<T, free_func<> >);

/* Pointer wrapper using reference linking. These can be allocated on the stack
   in their entirety, in contrast to reference counted pointers which always
   have a heap allocated part. These will never throw a bad_alloc exception.

   Note that in link_p we use several dirty tricks: we need a pointer to other.
   However, taking the address of a reference doesn't work. So we use
   other.next->prev, which points to other. Furthermore, we go through our own
   pointer to other to modify it, because simply trying to modify other is not
   allowed because it is const. Not having it const causes problems elsewhere.
*/
#define _T3_WIDGET_DEFINE_LINKED_PTR                               \
 public:                                                           \
  linked_ptr_base() : next(this), prev(this) {}                    \
  linked_ptr_base(T *p) { set_p(p); }                              \
  linked_ptr_base(const linked_ptr_base &other) { link_p(other); } \
  ~linked_ptr_base() { set_p(NULL); }                              \
  linked_ptr_base &operator=(const linked_ptr_base &other) {       \
    link_p(other);                                                 \
    return *this;                                                  \
  }                                                                \
  T *operator=(T *p) {                                             \
    set_p(p);                                                      \
    return smartptr_base<T>::p_;                                   \
  }                                                                \
                                                                   \
 private:                                                          \
  void set_p(T *p) {                                               \
    if (smartptr_base<T>::p_ == p) return;                         \
    if (smartptr_base<T>::p_ != NULL) {                            \
      if (next == this && prev == this) {                          \
        D d;                                                       \
        d(smartptr_base<T>::p_);                                   \
      } else {                                                     \
        next->prev = prev;                                         \
        prev->next = next;                                         \
      }                                                            \
    }                                                              \
    smartptr_base<T>::p_ = p;                                      \
    next = this;                                                   \
    prev = this;                                                   \
  }                                                                \
  void link_p(const linked_ptr_base &other) {                      \
    set_p(other.p_);                                               \
    next = other.next->prev;                                       \
    prev = other.prev;                                             \
    prev->next = this;                                             \
    next->prev = this;                                             \
  }                                                                \
  linked_ptr_base *next, *prev;

template <typename T, typename D>
class T3_WIDGET_API linked_ptr_base : public smartptr_base<T> {
  _T3_WIDGET_DEFINE_LINKED_PTR
};

template <typename T, typename D>
class T3_WIDGET_API linked_ptr_base<T[], D> : public smartptr_base<T> {
  _T3_WIDGET_DEFINE_LINKED_PTR
};

/* We also typedef what would be the basic variant, such that all type names
   use the same pattern. */
template <typename T, typename D = delete_functor<T> >
_T3_WIDGET_TYPEDEF(linked_ptr, linked_ptr_base<T, D>);

/* The following pointers are meant only for private implemenation purposes,
   therfore no assignment is possible, and only the constructor with argument
   is available. Other than that, they are normal auto-pointers.
*/
template <typename T>
class T3_WIDGET_API pimpl_ptr_base : public smartptr_base<T> {
 public:
  ~pimpl_ptr_base() {
    if (smartptr_base<T>::p_ != NULL) delete smartptr_base<T>::p_;
  }
  pimpl_ptr_base(T *p) { smartptr_base<T>::p_ = p; }

 private:
  pimpl_ptr_base &operator=(const pimpl_ptr_base &p) {
    (void)p;
    return *this;
  }
  pimpl_ptr_base(const pimpl_ptr_base &p) { (void)p; }
};
/* We also typedef what would be the basic variant, such that all type names
   use the same pattern. */
template <typename T>
_T3_WIDGET_TYPEDEF(pimpl_ptr, pimpl_ptr_base<T>);

#undef _T3_WIDGET_TYPEDEF
};  // namespace
#endif
