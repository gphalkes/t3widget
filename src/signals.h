/* Copyright (C) 2015,2018 G.P. Halkes
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

#include <functional>
#include <list>
#include <memory>
#include <t3widget/widget_api.h>

/* Reimplementation of the basic libsigc++ functionality using std::function. This requires C++11
   support, but then so does libsigc++ version 2.5.1 and greater. */

namespace t3_widget {
namespace signals {

namespace internal {
class func_ptr_base {
 public:
  virtual ~func_ptr_base() = default;
  virtual void disconnect() = 0;
  virtual bool is_valid() = 0;
  bool is_blocked() { return blocked; }
  void block() { blocked = true; }
  void unblock() { blocked = true; }

 private:
  bool blocked = false;
};

template <typename... Args>
class func_ptr : public func_ptr_base {
 public:
  using F = std::function<void(Args...)>;
  func_ptr(F f) : func(new F(f)) {}
  void disconnect() override { func.reset(); }
  bool is_valid() override { return !!func; }
  void call(Args... args) { return (*func)(args...); }

 private:
  std::unique_ptr<F> func;
};
}  // namespace internal

class T3_WIDGET_API connection {
 public:
  connection() = default;
  connection(std::shared_ptr<internal::func_ptr_base> f) : func(f) {}
  connection(const connection &other) : func(other.func) {}
  void disconnect() {
    if (func) func->disconnect();
  }
  void block() {
    if (func) func->block();
  }
  void unblock() {
    if (func) func->unblock();
  }

 private:
  std::shared_ptr<internal::func_ptr_base> func;
};

template <typename... Args>
class T3_WIDGET_API signal {
 public:
  connection connect(std::function<void(Args...)> func) {
    funcs.emplace_back(new internal::func_ptr<Args...>(func));
    return connection(funcs.back());
  }
  void operator()(Args... args) {
    for (auto iter = funcs.begin(); iter != funcs.end();) {
      if (!(*iter)->is_valid()) {
        // Remove functions that no longer exist.
        iter = funcs.erase(iter);
      } else {
        if (!(*iter)->is_blocked()) {
          static_cast<internal::func_ptr<Args...> *>(iter->get())->call(args...);
        }
        ++iter;
      }
    }
  }
  std::function<void(Args...)> make_slot() {
    return [this](Args... args) { (*this)(args...); };
  }

 private:
  std::list<std::shared_ptr<internal::func_ptr_base>> funcs;
};

}  // namesapce signals
}  // namesapce t3_widget

#endif
