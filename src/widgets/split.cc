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
#include "widgets/split.h"
#include "internal.h"
#include <algorithm>
#include <cstring>

namespace t3_widget {

#define _T3_ACTION_FILE "widgets/split.actions.h"
#define _T3_ACTION_TYPE split_t
#include "key_binding_def.h"

split_t::split_t(widget_t *widget) : horizontal(true), focus(false) {
  init_unbacked_window(3, 3);
  set_widget_parent(widget);
  widget->set_anchor(this, 0);
  widget->show();
  widgets.push_back(widget);
  current = widgets.begin();
}

split_t::~split_t() {
  for (widget_t *widget : widgets) {
    delete widget;
  }
}

bool split_t::process_key(key_t key) {
  if (widgets.empty()) {
    return false;
  }

  optional<Action> action = key_bindings.find_action(key);
  if (action.is_valid()) {
    switch (action) {
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
    return (*current)->process_key(key);
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

  result = window.resize(height, width);

  if (horizontal) {
    int idx;
    int step = height / widgets.size();
    int left_over = height % widgets.size();
    widgets_t::iterator iter;

    for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
      result &= (*iter)->set_size(step + (idx < left_over), width);
      (*iter)->set_position(idx * step + std::min(idx, left_over), 0);
    }
  } else {
    int idx;
    int step = width / widgets.size();
    int left_over = width % widgets.size();
    widgets_t::iterator iter;

    for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
      result &= (*iter)->set_size(height, step + (idx < left_over));
      (*iter)->set_position(0, idx * step + std::min(idx, left_over));
    }
  }
  return result;
}

void split_t::update_contents() {
  for (widget_t *widget : widgets) {
    widget->update_contents();
  }
}

void split_t::set_focus(focus_t _focus) {
  focus = _focus;
  (*current)->set_focus(_focus);
}

void split_t::force_redraw() {
  for (widget_t *widget : widgets) {
    widget->force_redraw();
  }
}

void split_t::set_child_focus(window_component_t *target) {
  for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
    if (*iter == target) {
      if (*current != *iter) {
        (*current)->set_focus(window_component_t::FOCUS_OUT);
        current = iter;
        (*current)->set_focus(window_component_t::FOCUS_SET);
      }
      return;
    } else {
      container_t *container = dynamic_cast<container_t *>(*iter);
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

bool split_t::is_child(window_component_t *widget) {
  for (widget_t *iter : widgets) {
    if (iter == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter);
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

void split_t::split(widget_t *widget, bool _horizontal) {
  split_t *current_window = dynamic_cast<split_t *>(*current);

  if (current_window != nullptr) {
    current_window->split(widget, _horizontal);
  } else if (widgets.size() == 1 || _horizontal == horizontal) {
    horizontal = _horizontal;
    container_t::set_widget_parent(widget);
    widget->set_anchor(this, 0);
    widget->show();
    if (focus) {
      (*current)->set_focus(window_component_t::FOCUS_OUT);
    }
    current++;
    current = widgets.insert(current, widget);
    set_size(None, None);
    if (focus) {
      (*current)->set_focus(window_component_t::FOCUS_SET);
    }
  } else {
    /* Create a new split_t with the current widget as its contents. Then
       add split that split_t to splice in the requested widget. */
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current_window = new split_t(*current);
    container_t::set_widget_parent(current_window);
    current_window->set_focus(window_component_t::FOCUS_SET);
    current_window->split(widget, _horizontal);
    *current = current_window;
    set_size(None, None);
  }
}

bool split_t::unsplit(widget_t **widget) {
  split_t *current_window = dynamic_cast<split_t *>(*current);

  if (current_window == nullptr) {
    /* This should not happen for previously split windows. However, for
       the first split_t instance this may be the case, so we have to
       handle it. */
    if (widgets.size() == 1) {
      return true;
    }
    *widget = *current;
    current = widgets.erase(current);
    if (current == widgets.end()) {
      current--;
      if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
        current_window->set_to_end();
      }
    } else {
      if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
        current_window->set_to_begin();
      }
    }
    if (focus) {
      (*current)->set_focus(window_component_t::FOCUS_SET);
    }
    set_size(None, None);
    if (widgets.size() == 1) {
      return true;
    }
  } else {
    if (current_window->unsplit(widget)) {
      *current = current_window->widgets.front();
      set_widget_parent(*current);
      (*current)->set_anchor(this, 0);
      current_window->widgets.clear();
      delete current_window;
      if (focus) {
        (*current)->set_focus(window_component_t::FOCUS_SET);
      }
      set_size(None, None);
    }
  }
  return false;
}

widget_t *split_t::unsplit() {
  widget_t *result = nullptr;
  unsplit(&result);
  return result;
}

bool split_t::next_recurse() {
  split_t *current_window = dynamic_cast<split_t *>(*current);
  if (current_window == nullptr || !current_window->next_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current++;
    if (current != widgets.end()) {
      if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
        current_window->set_to_begin();
      }
      if (focus) {
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
  split_t *current_window = dynamic_cast<split_t *>(*current);
  if (current_window == nullptr || !current_window->previous_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    if (current == widgets.begin()) {
      return false;
    }
    current--;

    if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
      current_window->set_to_end();
    }
    (*current)->set_focus(window_component_t::FOCUS_IN_BCK);
  }
  return true;
}

void split_t::next() {
  split_t *current_window = dynamic_cast<split_t *>(*current);
  if (current_window == nullptr || !current_window->next_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    current++;
    if (current == widgets.end()) {
      current = widgets.begin();
    }

    if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
      current_window->set_to_begin();
    }
    if (focus) {
      (*current)->set_focus(window_component_t::FOCUS_IN_FWD);
    }
  }
}

void split_t::previous() {
  split_t *current_window = dynamic_cast<split_t *>(*current);
  if (current_window == nullptr || !current_window->previous_recurse()) {
    (*current)->set_focus(window_component_t::FOCUS_OUT);
    if (current == widgets.begin()) {
      current = widgets.end();
    }
    current--;

    if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
      current_window->set_to_end();
    }
    if (focus) {
      (*current)->set_focus(window_component_t::FOCUS_IN_BCK);
    }
  }
}

widget_t *split_t::get_current() {
  split_t *current_window = dynamic_cast<split_t *>(*current);
  if (current_window == nullptr) {
    return *current;
  } else {
    return current_window->get_current();
  }
}

void split_t::set_to_begin() {
  split_t *current_window;

  current = widgets.begin();
  if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
    current_window->set_to_begin();
  }
}

void split_t::set_to_end() {
  split_t *current_window;

  current = widgets.end();
  current--;
  if ((current_window = dynamic_cast<split_t *>(*current)) != nullptr) {
    current_window->set_to_end();
  }
}

}  // namespace
