/* Copyright (C) 2011-2012,2018 G.P. Halkes
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
#include <algorithm>
#include <deque>
#include <memory>
#include <type_traits>
#include <utility>

#include "t3widget/interfaces.h"
#include "t3widget/key.h"
#include "t3widget/key_binding.h"
#include "t3widget/util.h"
#include "t3widget/widgets/split.h"
#include "t3widget/widgets/widget.h"
#include "t3window/window.h"

namespace t3widget {

#define _T3_ACTION_FILE "t3widget/widgets/split.actions.h"
#define _T3_ACTION_TYPE split_t
#include "t3widget/key_binding_def.h"

struct split_t::implementation_t {
  widgets_t widgets;           /**< The list of widgets contained by this split_t. */
  widgets_t::iterator current; /**< The currently active widget. */
  /** Boolean indicating whether to divide the space horizontally or vertically. */
  bool horizontal = true;
  /** Boolean indicating whether this split_t (or rather, one of its children) has the input focus.
   */
  bool focus = false;
};

split_t::split_t(std::unique_ptr<widget_t> widget)
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()) {
  init_unbacked_window(3, 3);
  set_widget_parent(widget.get());
  widget->set_anchor(this, 0);
  widget->show();
  impl->widgets.push_back(std::move(widget));
  impl->current = impl->widgets.begin();
}

split_t::~split_t() {}

bool split_t::process_key(key_t key) {
  if (impl->widgets.empty()) {
    return false;
  }

  optional<Action> action = key_bindings.find_action(key);
  if (action.is_valid()) {
    switch (action.value()) {
      case ACTION_NEXT_SPLIT:
        next();
        break;
      case ACTION_PREVIOUS_SPLIT:
        previous();
        break;
      default:
        break;
    }
  } else {
    return (*impl->current)->process_key(key);
  }
  return true;
}

bool split_t::set_size(optint height, optint width) {
  bool result;

  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  result = window.resize(height.value(), width.value());

  if (impl->horizontal) {
    int idx = 0;
    int step = height.value() / impl->widgets.size();
    int left_over = height.value() % impl->widgets.size();

    for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
      result &= widget->set_size(step + (idx < left_over), width);
      widget->set_position(idx * step + std::min(idx, left_over), 0);
      ++idx;
    }
  } else {
    int idx = 0;
    int step = width.value() / impl->widgets.size();
    int left_over = width.value() % impl->widgets.size();

    for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
      result &= widget->set_size(height, step + (idx < left_over));
      widget->set_position(0, idx * step + std::min(idx, left_over));
      ++idx;
    }
  }
  return result;
}

void split_t::update_contents() {
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    widget->update_contents();
  }
}

void split_t::set_focus(focus_t _focus) {
  impl->focus = _focus;
  (*impl->current)->set_focus(_focus);
}

void split_t::force_redraw() {
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    widget->force_redraw();
  }
}

void split_t::set_child_focus(window_component_t *target) {
  widgets_t::iterator &current = impl->current;
  for (widgets_t::iterator iter = impl->widgets.begin(); iter != impl->widgets.end(); iter++) {
    if (iter->get() == target) {
      if (*current != *iter) {
        (*current)->set_focus(window_component_t::FOCUS_OUT);
        current = iter;
        (*current)->set_focus(window_component_t::FOCUS_SET);
      }
      return;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter->get());
      if (container != nullptr && container->is_child(target)) {
        if (*current != *iter) {
          (*current)->set_focus(window_component_t::FOCUS_OUT);
          current = iter;
        }
        container->set_child_focus(target);
        return;
      }
    }
  }
}

bool split_t::is_child(const window_component_t *widget) const {
  for (const std::unique_ptr<widget_t> &iter : impl->widgets) {
    if (iter.get() == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter.get());
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

void split_t::split(std::unique_ptr<widget_t> widget, bool _horizontal) {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());

  if (current_window != nullptr) {
    current_window->split(std::move(widget), _horizontal);
  } else if (impl->widgets.size() == 1 || _horizontal == impl->horizontal) {
    impl->horizontal = _horizontal;
    container_t::set_widget_parent(widget.get());
    widget->set_anchor(this, 0);
    widget->show();
    if (impl->focus) {
      (*current)->set_focus(window_component_t::FOCUS_OUT);
    }
    current++;
    current = impl->widgets.insert(current, std::move(widget));
    set_size(None, None);
    if (impl->focus) {
      (*current)->set_focus(window_component_t::FOCUS_SET);
    }
  } else {
    /* Create a new split_t with the current widget as its contents. Then
       add split that split_t to splice in the requested widget. */
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current_window = new split_t(std::move(*current));
    container_t::set_widget_parent(current_window);
    current_window->set_focus(window_component_t::FOCUS_SET);
    current_window->split(std::move(widget), _horizontal);
    current->reset(current_window);
    set_size(None, None);
  }
}

bool split_t::unsplit(std::unique_ptr<widget_t> *widget) {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());

  if (current_window == nullptr) {
    /* This should not happen for previously split windows. However, for
       the first split_t instance this may be the case, so we have to
       handle it. */
    if (impl->widgets.size() == 1) {
      return true;
    }
    *widget = std::move(*current);
    current = impl->widgets.erase(current);
    if (current == impl->widgets.end()) {
      current--;
      if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
        current_window->set_to_end();
      }
    } else {
      if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
        current_window->set_to_begin();
      }
    }
    if (impl->focus) {
      (*current)->set_focus(window_component_t::FOCUS_SET);
    }
    set_size(None, None);
    if (impl->widgets.size() == 1) {
      return true;
    }
  } else {
    if (current_window->unsplit(widget)) {
      *current = std::move(current_window->impl->widgets.front());
      set_widget_parent(current->get());
      (*current)->set_anchor(this, 0);
      current_window->impl->widgets.clear();
      delete current_window;
      if (impl->focus) {
        (*current)->set_focus(window_component_t::FOCUS_SET);
      }
      set_size(None, None);
    }
  }
  return false;
}

std::unique_ptr<widget_t> split_t::unsplit() {
  std::unique_ptr<widget_t> result;
  unsplit(&result);
  return result;
}

bool split_t::next_recurse() {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());
  if (current_window == nullptr || !current_window->next_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current++;
    if (current != impl->widgets.end()) {
      if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
        current_window->set_to_begin();
      }
      if (impl->focus) {
        (*current)->set_focus(window_component_t::FOCUS_IN_FWD);
      }
      return true;
    } else {
      current--;
      return false;
    }
  }
  return true;
}

bool split_t::previous_recurse() {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());
  if (current_window == nullptr || !current_window->previous_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    if (current == impl->widgets.begin()) {
      return false;
    }
    current--;

    if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
      current_window->set_to_end();
    }
    (*current)->set_focus(window_component_t::FOCUS_IN_BCK);
  }
  return true;
}

void split_t::next() {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());
  if (current_window == nullptr || !current_window->next_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current++;
    if (current == impl->widgets.end()) {
      current = impl->widgets.begin();
    }

    if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
      current_window->set_to_begin();
    }
    if (impl->focus) {
      (*current)->set_focus(window_component_t::FOCUS_IN_FWD);
    }
  }
}

void split_t::previous() {
  widgets_t::iterator &current = impl->current;
  split_t *current_window = dynamic_cast<split_t *>(current->get());
  if (current_window == nullptr || !current_window->previous_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    if (current == impl->widgets.begin()) {
      current = impl->widgets.end();
    }
    current--;

    if ((current_window = dynamic_cast<split_t *>(current->get())) != nullptr) {
      current_window->set_to_end();
    }
    if (impl->focus) {
      (*current)->set_focus(window_component_t::FOCUS_IN_BCK);
    }
  }
}

widget_t *split_t::get_current() const {
  split_t *current_window = dynamic_cast<split_t *>(impl->current->get());
  if (current_window == nullptr) {
    return impl->current->get();
  } else {
    return current_window->get_current();
  }
}

void split_t::set_to_begin() {
  split_t *current_window;

  impl->current = impl->widgets.begin();
  if ((current_window = dynamic_cast<split_t *>(impl->current->get())) != nullptr) {
    current_window->set_to_begin();
  }
}

void split_t::set_to_end() {
  split_t *current_window;

  impl->current = impl->widgets.end();
  impl->current--;
  if ((current_window = dynamic_cast<split_t *>(impl->current->get())) != nullptr) {
    current_window->set_to_end();
  }
}

}  // namespace t3widget
