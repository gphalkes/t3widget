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

#ifndef T3_WIDGET_SIGNAL_H
#define T3_WIDGET_SIGNAL_H

#if __cplusplus >= 201103L
#include <functional>
// Reimplementation of the basic libsigc++ functionality using std::function.
// This requires C++11 support, but then so does libsigc++ version 2.5.1 and
// greater.

namespace signal {
// mem_fun
template<typename R, typename C, typename... Args>
std::function<R(Args...)> mem_fun(C* instance, R (C::* func)(Args...)) {
    return [=](Args... args){ return (instance->*func)(args...); };
}

// Extra templates for disambiguation.
template<typename R, typename C>
std::function<R()> mem_fun0(C* instance, R (C::* func)()) {
    return [=](){ return (instance->*func)(); };
}

template<typename R, typename C, typename A>
std::function<R(A)> mem_fun1(C* instance, R (C::* func)(A)) {
    return [=](A a){ return (instance->*func)(a); };
}

// ptr_fun
template<typename R, typename... Args>
std::function<R(Args...)> ptr_fun(R (*ptr)(Args...)) {
	return ptr;
}

// bind
template<typename R, typename A, typename... Args>
std::function<R(Args...)> bind(std::function<R(A, Args...)> f, A a) {
  return [=](Args... args){ f(a, args...); };
}

// FIXME: implement slot (or rather typedef it), signal and connection

}  // namesapce signal

#else
#include <sigc++/sigc++.h>
namespace signal = sigc;
#endif

#endif
