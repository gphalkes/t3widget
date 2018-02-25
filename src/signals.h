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

namespace t3_widget {
namespace signals {

namespace internal {
/* Base class for function wrappers. This provides the part of the functionality that is not
   dependent on actual function argument types, and defines the generic functionality in terms of
   abstract functions. Having this base class allows the connection class to be fully generic. */
class func_ptr_base {
 public:
  virtual ~func_ptr_base() = default;
  virtual void disconnect() = 0;
  virtual bool is_valid() = 0;
  // Blocked signals don't get called.
  bool is_blocked() { return blocked; }
  void block() { blocked = true; }
  void unblock() { blocked = true; }

 private:
  bool blocked = false;
};

/* Templated subclass of func_base_ptr which implements the actual call functionality and holds the
   pointer to the actual std::function. */
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

/** A connection is the handle for a callback that is associated with a signal.

    Using the @c connection, a callback can be removed from the signal, or it can be blocked and
    unblocked to temporarily stop the callback from being called. The @c connection object does not
    have to be alive for the callback to be activated. I.e., if the lifetime of the callback is no
    bound to the @c connection object.
*/
class T3_WIDGET_API connection {
 public:
  connection() = default;
  connection(std::shared_ptr<internal::func_ptr_base> f) : func(f) {}
  connection(const connection &other) : func(other.func) {}
  /// Disconnect the callback from the signal.
  void disconnect() {
    if (func) {
      func->disconnect();
      func.reset();
    }
  }
  /// Prevent signal activation from calling the callback.
  void block() {
    if (func) func->block();
  }
  /** Allow signal activation to call the callback. This is the default state. */
  void unblock() {
    if (func) func->unblock();
  }

 private:
  std::shared_ptr<internal::func_ptr_base> func;
};

/** A signal object allows a set of callbacks to be called on activation.

    The signal object holds zero or more callbacks, which get called when operator() is called. The
    purpose of this is to allow an object to provide a callback interface, which may be hooked into
    by multiple other objects. Through the returned @c connection object, registered callbacks can
    be controlled or removed. Note that actual removal of the callback object (std::function) only
    happens on activation of the signal.
*/
template <typename... Args>
class T3_WIDGET_API signal {
 public:
  /// Add a callback to be called on activation.
  connection connect(std::function<void(Args...)> func) {
    funcs.emplace_back(new internal::func_ptr<Args...>(func));
    return connection(funcs.back());
  }

  /// Activate the signal, i.e. call all the registered active callbacks.
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

  /** Get a callback which, when called, activates the signal.

      This callback allows chaining of signals, by passing the returned callback to the @c connect
      method of another @c signal. */
  std::function<void(Args...)> get_trigger() {
    return [this](Args... args) { (*this)(args...); };
  }

 private:
  std::list<std::shared_ptr<internal::func_ptr_base>> funcs;
};

}  // namesapce signals
}  // namesapce t3_widget

#endif
