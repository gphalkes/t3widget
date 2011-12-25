/* Copyright (C) 2011 G.P. Halkes
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

#include <cstdlib>

namespace t3_widget {

template <typename T>
struct delete_functor {
	void operator()(T* p) { delete p; }
};

template <typename T>
struct delete_functor<T []> {
	void operator()(T* p) { delete [] p; }
};

template <typename T = void, void (*f)(T *) = free>
struct free_func {
	void operator()(T *p) { f((T *) p); }
};

template <typename T, typename U, U (*f)(T *)>
struct free_func2 {
	void operator()(T *p) { f((T *) p); }
};

template <typename T>
class smartptr_base {
	public:
		T* operator-> (void) const { return p_; }
		T& operator* (void) const { return *p_; }
		T** operator& (void) { return &p_; }
		T* operator() (void) const { return p_; }
		operator T* (void) { return p_; }
		T* get(void) { return p_; }
	protected:
		smartptr_base(void) : p_(NULL) {}
		T* p_;
};

/* Pointer wrapper which automatically de-allocate its objects when going out
   of scope. The difference with the Boost scoped_ptr is that the objects are not
   deallocated when assigning a different value. Their main use is to ensure
   deallocation during exception handling, and storing of temporary values.
*/
#define _DEFINE_cleanup_ptr \
	public: \
		~cleanup_ptr(void) { if (smartptr_base<T>::p_ != NULL) { D d; d(smartptr_base<T>::p_); } } \
		cleanup_ptr(void) {} \
		cleanup_ptr(T *p) { smartptr_base<T>::p_ = p; } \
		T* operator= (T *p) { return smartptr_base<T>::p_ = p; } \
	private: \
		cleanup_ptr& operator= (const cleanup_ptr &p) { (void) p; return *this; } \
		cleanup_ptr(const cleanup_ptr &p) { (void) p; }

template <typename T, typename D = delete_functor<T> >
class cleanup_ptr : public smartptr_base<T> {
	_DEFINE_cleanup_ptr
};

template <typename T, typename D>
class cleanup_ptr<T[], D> : public smartptr_base<T> {
	_DEFINE_cleanup_ptr
};
#undef _DEFINE_cleanup_ptr
typedef cleanup_ptr<char, free_func<> > cleanup_ptr_char;

template <typename T, void (*f)(T *)>
class cleanup_func_ptr : public cleanup_ptr<T, free_func<T, f> > {
	public: \
		cleanup_func_ptr(void) {} \
		cleanup_func_ptr(T *p) { cleanup_ptr<T, free_func<T, f> >::p_ = p; } \
		T* operator= (T *p) { return cleanup_ptr<T, free_func<T, f> >::p_ = p; } \
	private: \
		cleanup_func_ptr& operator= (const cleanup_func_ptr &p) { (void) p; return *this; } \
		cleanup_func_ptr(const cleanup_func_ptr &p) { (void) p; }
};

/* Pointer wrapper using reference linking. */
#define _DEFINE_LINKED_PTR \
	public: \
		linked_ptr(void) : next(this), prev(this) {} \
		linked_ptr(T *p) { set_p(p); } \
		linked_ptr(const linked_ptr &other) { link_p(other); } \
		linked_ptr& operator= (const linked_ptr &other) { link_p(other); return *this; } \
		T* operator= (T *p) { set_p(p); return smartptr_base<T>::p_; } \
	private: \
		void set_p(T *p) { \
			if (smartptr_base<T>::p_ != NULL) { \
				if (next == this) { \
					D d; \
					d(smartptr_base<T>::p_); \
				} else { \
					next->prev = prev; \
					prev->next = next; \
				} \
			} \
			smartptr_base<T>::p_ = p; \
			next = this; \
			prev = this; \
		} \
		void link_p(const linked_ptr &other) { \
			set_p(other.p_); \
			next = other.next; \
			prev = other.prev; \
			prev->next = this; \
			next->prev = this; \
		} \
		linked_ptr *next, *prev; \

template <typename T, typename D = delete_functor<T> >
class linked_ptr : public smartptr_base<T> {
	_DEFINE_LINKED_PTR
};

template <typename T, typename D>
class linked_ptr<T[], D> : public smartptr_base<T> {
	_DEFINE_LINKED_PTR
};

}; // namespace
#endif
