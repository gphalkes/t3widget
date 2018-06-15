/* Copyright (C) 2017-2018 G.P. Halkes
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
#ifndef T3_WIDGET_KEY_BINDING_H
#define T3_WIDGET_KEY_BINDING_H

#include <algorithm>
#include <map>
#include <vector>

#include "t3widget/util.h"

namespace t3_widget {

class T3_WIDGET_API key_bindings_base_t {
 public:
  virtual ~key_bindings_base_t();
  virtual size_t names_size() const = 0;
  virtual const std::string &names(size_t idx) const = 0;
  virtual bool bind_key(key_t key, const std::string &name) = 0;
};

template <typename T>
class T3_WIDGET_API key_bindings_t : public key_bindings_base_t {
 public:
  struct param_t {
    T action;
    std::string name;
    std::initializer_list<key_t> bound_keys;
  };

  key_bindings_t(std::initializer_list<param_t> actions) {
    for (const param_t &action : actions) {
      name_mapping[action.name] = action.action;
      for (key_t key : action.bound_keys) {
        if (key >= 0) {
          key_bindings[key] = action.action;
        }
      }
    }
  }
  key_bindings_t() {}

  optional<T> find_action(key_t key) const {
    typename std::map<key_t, T>::const_iterator iter = key_bindings.find(key);
    if (iter == key_bindings.end()) {
      return nullopt;
    }
    return iter->second;
  }

  void bind_key(key_t key, optional<T> action) {
    if (action.is_valid()) {
      key_bindings[key] = action;
    } else {
      key_bindings.erase(key);
    }
  }

  bool bind_key(key_t key, const std::string &name) override {
    if (name.empty()) {
      key_bindings.erase(key);
      return true;
    }
    optional<T> action = map_name(name);
    if (!action.is_valid()) {
      return false;
    }
    key_bindings[key] = action;
    return true;
  }

  optional<T> map_name(const std::string &name) const {
    typename std::map<std::string, T>::const_iterator iter = name_mapping.find(name);
    if (iter == name_mapping.end()) {
      return nullopt;
    }
    return iter->second;
  }

  size_t names_size() const override { return name_mapping.size(); }

  const std::string &names(size_t idx) const override {
    return std::next(name_mapping.begin(), idx)->first;
  }

 private:
  std::map<std::string, T> name_mapping;
  std::map<key_t, T> key_bindings;
};

}  // namespace t3_widget

#endif
