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
#ifndef T3_WIDGET_KEYBUFFER_H
#define T3_WIDGET_KEYBUFFER_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

/* Buffer for storing keys that ensures thread-safe operation by means of a
   mutex. It is implemented by means of a double ended queue.  */

#include <pthread.h>
#include <algorithm>
#include <deque>

#include <t3widget/key.h>
namespace t3_widget {

/** Class implmementing a mutex-protected queue of items. */
template <class T>
class item_buffer_t {
	protected:
		/** The list of item symbols. */
		std::deque<T> items;
		/** The mutex used for the critical section. */
		pthread_mutex_t lock;
		/** The condition variable used to signal addition to the #keys list. */
		pthread_cond_t cond;

	public:
		/** Create a new key_buffer_t. */
		item_buffer_t(void) {
			pthread_mutex_init(&lock, NULL);
			pthread_cond_init(&cond, NULL);
		}

		/** Append an item to the list. */
		void push_back(T item) {
			pthread_mutex_lock(&lock);
			/* Catch all exceptions, to enusre that unlocking of the mutex is
			   performed. The only real exception that can occur here is bad_alloc,
			   and there is not much we can do about that anyway. */
			try {
				items.push_back(item);
			} catch (...) {
			}
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&lock);
		}

		/** Retrieve and remove the item at the front of the queue. */
		T pop_front(void) {
			T result;
			pthread_mutex_lock(&lock);
			while (items.empty())
				pthread_cond_wait(&cond, &lock);
			result = items.front();
			items.pop_front();
			pthread_mutex_unlock(&lock);
			return result;
		}

		/** Destroy the key_buffer_t. */
		~item_buffer_t(void) {
			pthread_mutex_destroy(&lock);
			pthread_cond_destroy(&cond);
		}
};

/** Class implmementing a mutex-protected queue of key symbols. */
class key_buffer_t : public item_buffer_t<key_t> {
	public:
		/** Append an item to the list, but only if it is not already in the queue. */
		void push_back_unique(key_t key) {
			pthread_mutex_lock(&lock);

			// Return without adding if the key is already queued
			if (find(items.begin(), items.end(), key) != items.end()) {
				pthread_mutex_unlock(&lock);
				return;
			}

			/* Catch all exceptions, to enusre that unlocking of the mutex is
			   performed. The only real exception that can occur here is bad_alloc,
			   and there is not much we can do about that anyway. */
			try {
				items.push_back(key);
			} catch (...) {
			}
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&lock);
		}
};

typedef item_buffer_t<mouse_event_t> mouse_event_buffer_t;

}; // namespace
#endif
