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
/* Pointer wrappers which automatically de-allocate their objects when going out
   of scope. The difference with the Boost scoped_ptr is that the objects are not
   deallocated when assigning a different value. Their main use is to ensure
   deallocation during exception handling, and storing of temporary values.
*/
template <typename T>
class cleanup_abstract {
	public:
		T* operator-> (void) const { return p_; }
		T& operator* (void) const { return *p_; }
		T** operator& (void) { return &p_; }
		T* operator() (void) const { return p_; }
		cleanup_abstract(void) : p_(NULL) {}
		cleanup_abstract(T *p) : p_(p) {}
		T* operator= (T *p) { return p_ = p; }
		operator T* (void) { return p_; }
		T* get(void) { return p_; }
	protected:
		cleanup_abstract& operator= (const cleanup_abstract &p) { (void) p; return *this; }
		cleanup_abstract(const cleanup_abstract &p) { (void) p; }
		T* p_;
};

template <typename T>
class cleanup_obj_ptr : public cleanup_abstract<T> {
	public:
		~cleanup_obj_ptr(void) { delete cleanup_abstract<T>::p_; }
		cleanup_obj_ptr(void) : cleanup_abstract<T>(NULL) {}
		cleanup_obj_ptr(T *p) : cleanup_abstract<T>(p) {}
		T* operator= (T *p) { return cleanup_abstract<T>::p_ = p; }
	protected:
		cleanup_obj_ptr& operator= (const cleanup_obj_ptr &p) { (void) p; return *this; }
		cleanup_obj_ptr(const cleanup_obj_ptr &p) { (void) p; }
};

template <typename T>
class cleanup_objarr_ptr : public cleanup_abstract<T> {
	public:
		~cleanup_objarr_ptr(void) { delete [] cleanup_abstract<T>::p_; }
		cleanup_objarr_ptr(void) : cleanup_abstract<T>(NULL) {}
		cleanup_objarr_ptr(T *p) : cleanup_abstract<T>(p) {}
		T* operator= (T *p) { return cleanup_abstract<T>::p_ = p; }
	protected:
		cleanup_objarr_ptr& operator= (const cleanup_objarr_ptr &p) { (void) p; return *this; }
		cleanup_objarr_ptr(const cleanup_objarr_ptr &p) { (void) p; }
};

template <typename T, typename U = void, void (*f)(U *) = free>
class cleanup_ptr : public cleanup_abstract<T> {
	public:
		~cleanup_ptr(void) { f((U *) cleanup_abstract<T>::p_); }
		cleanup_ptr(void) : cleanup_abstract<T>(NULL) {}
		cleanup_ptr(T *p) : cleanup_abstract<T>(p) {}
		T* operator= (T *p) { return cleanup_abstract<T>::p_ = p; }
	private:
		cleanup_ptr& operator= (const cleanup_ptr &p) { (void) p; return *this; }
		cleanup_ptr(const cleanup_ptr &p) { (void) p; }
};

template <typename T>
class linked_ptr {
	public:
		T* operator-> (void) const { return p_; }
		T& operator* (void) const { return *p_; }
		T** operator& (void) { return &p_; }
		T* operator() (void) const { return p_; }

		linked_ptr(void) : p_(NULL), next(this), prev(this) {}
		linked_ptr(T *p) : p_(NULL) {
			set_p(p);
		}
		linked_ptr(const linked_ptr &other) : p_(NULL) { link_p(other); }
		linked_ptr& operator= (const linked_ptr &other) { link_p(other); return *this; }
		T* operator= (T *p) { set_p(p); return p_; }
		~linked_ptr(void) { set_p(NULL); }

		operator T* (void) { return p_; }
		T* get(void) { return p_; }
	protected:
		void set_p(T *p) {
			if (p_ != NULL) {
				if (next == this) {
					delete p_;
				} else {
					next->prev = prev;
					prev->next = next;
				}
			}
			p_ = p;
			next = this;
			prev = this;
		}
		void link_p(const linked_ptr &other) {
			set_p(other.p_);
			next = other.next;
			prev = other.prev;
			prev->next = this;
			next->prev = this;
		}

		T* p_;
		linked_ptr *next, *prev;
};

}; // namespace
#endif
