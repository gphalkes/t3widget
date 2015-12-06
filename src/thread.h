/* Copyright (C) 2015 G.P. Halkes
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

#ifndef T3_WIDGET_THREAD_H
#define T3_WIDGET_THREAD_H

/* This header file provides a set of types in the t3_widget::thread namespace which
   use interfaces sufficiently similar to the C++11 versions of the types with the
   same name. The underlying idea is to remove the need for a pthread library, using
   only the types available in C++11. Only the parts of the std:: interface that is
   actually used is implemented. In one case (std::condition_variable::wait_until)
   the choice was made not to reimplement part of the standard. This does mean that
   we can not use "namespace thread = std" in the C++11 case, which would otherwise
   be possible.

   Note that some parts of the interface may actually have different semantics for
   some use cases that are not used in libt3widget.
*/

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#if __cplusplus >= 201103L
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace t3_widget {
namespace thread {
using mutex = std::mutex;
using thread = std::thread;
namespace this_thread = std::this_thread;
template <typename T> using unique_lock = std::unique_lock<T>;
using defer_lock_t = std::defer_lock_t;
using condition_variable = std::condition_variable;
using cv_status = std::cv_status;
using timeout_t = std::chrono::time_point<std::chrono::system_clock>;
inline timeout_t timeout_time(int microseconds) {
	return std::chrono::system_clock::now() + std::chrono::microseconds(microseconds);
}
}  // namespace thread
}  // namespace t3_widget
#else
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
namespace t3_widget {
namespace thread {
class thread {
public:
	thread() {}
	thread(void (*f)()) { pthread_create(&handle, NULL, wrap_function, reinterpret_cast<void *>(f)); }
	void join() { void *result; pthread_join(handle, &result); }
	const thread &operator=(const thread &other) { handle = other.handle; return *this; }

private:
	static void *wrap_function(void *f) { reinterpret_cast<void (*)()>(f)(); return NULL; }
	pthread_t handle;
};

namespace this_thread {
inline void yield() { pthread_yield(); }
}  // namespace this_thread

typedef struct timespec timeout_t;
inline timeout_t timeout_time(int microseconds) {
	struct timeval timeval;
	struct timespec result;

	gettimeofday(&timeval, NULL);
	timeval.tv_sec += microseconds / 1000000;
	timeval.tv_usec += microseconds % 1000000;
	if (timeval.tv_usec > 1000000) {
		timeval.tv_sec++;
		timeval.tv_usec -= 1000000;
	}
	result.tv_sec = timeval.tv_sec;
	result.tv_nsec = (long) timeval.tv_usec * 1000;
	return result;
}

class mutex {
public:
	mutex() { pthread_mutex_init(&mtx, NULL); }
	~mutex() { pthread_mutex_destroy(&mtx); }

	void lock() { pthread_mutex_lock(&mtx); }
	void unlock() { pthread_mutex_unlock(&mtx); }

private:
	friend class condition_variable;
	pthread_mutex_t mtx;
};

struct defer_lock_t { };

template <class T>
class unique_lock {
public:
	unique_lock(T &m) : locked(true), mtx(m) { mtx.lock(); }
	unique_lock(T &m, defer_lock_t t) : locked(false), mtx(m) { (void) t; }
	~unique_lock() { if (locked) mtx.unlock(); }
	void lock() { mtx.lock(); locked = true; }
	void unlock() { mtx.unlock(); locked = false; }
private:
	friend class condition_variable;
	bool locked;
	T &mtx;
};

struct cv_status {
	enum {
		no_timeout,
		timeout
	};
};

class condition_variable {
public:
	condition_variable() { pthread_cond_init(&cond, NULL); }
	~condition_variable() { pthread_cond_destroy(&cond); }

	void notify_one() { pthread_cond_signal(&cond); }
	void notify_all() { pthread_cond_broadcast(&cond); }
	void wait(unique_lock<mutex> &lock) { pthread_cond_wait(&cond, &lock.mtx.mtx); }

	// This function has a different signature, because reimplementing all the
	// time related stuff in the C++11 standard is too much. Instead, we define
	// a new type (timeout_t) and a function to calculate those.
	int wait_until(unique_lock<mutex> &lock, const timeout_t &timeout) {
		return pthread_cond_timedwait(&cond, &lock.mtx.mtx, &timeout) == ETIMEDOUT ? cv_status::timeout : cv_status::no_timeout;
	}

private:
	pthread_cond_t cond;
};
}  // namespace thread
}  // namespace t3_widget
#endif

#endif

