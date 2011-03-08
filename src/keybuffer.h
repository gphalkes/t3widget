/* Copyright (C) 2010 G.P. Halkes
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

/* Buffer for storing keys that ensures thread-safe operation by means of a
   mutex. It is implemented by means of a double ended queue.  */

#include <pthread.h>
#include <deque>

#include "keys.h"
namespace t3_widget {

typedef std::deque<key_t> keys_t;

class KeyBuffer {
	private:
		keys_t keys;
		pthread_mutex_t lock;
		pthread_cond_t cond;

	public:
		KeyBuffer(void) {
			pthread_mutex_init(&lock, NULL);
			pthread_cond_init(&cond, NULL);
		}

		void push_back(key_t key) {
			pthread_mutex_lock(&lock);
			// FIXME: Catch appropriate exceptions. For now just catch all to
			// enusre that unlocking of the mutex is performed.
			try {
				keys.push_back(key);
			} catch (...) {
			}
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&lock);
		}

		key_t pop_front(void) {
			key_t result;
			pthread_mutex_lock(&lock);
			while (keys.empty())
				pthread_cond_wait(&cond, &lock);
			result = keys.front();
			keys.pop_front();
			pthread_mutex_unlock(&lock);
			return result;
		}

		~KeyBuffer(void) {
			pthread_mutex_destroy(&lock);
			pthread_cond_destroy(&cond);
		}
};

}; // namespace
#endif
