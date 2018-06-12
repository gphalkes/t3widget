/* Copyright (C) 2012-2013,2018 G.P. Halkes
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
#include "widgets/widgetgroup.h"
#include "log.h"

namespace t3widget {

struct widget_group_t::implementation_t {
  owned_widgets_t children;
  int current_child;
  bool has_focus;
  implementation_t() : current_child(-1), has_focus(false) {}
};

widget_group_t::widget_group_t()
    : widget_t(impl_alloc<focus_widget_t::implementation_t>(impl_alloc<implementation_t>(0))),
      focus_widget_t(this),
      impl(new_impl<implementation_t>()) {
  init_unbacked_window(1, 1);
}

bool widget_group_t::focus_next_int() {
  int next;

  if (!impl->has_focus) {
    return false;
  }

  for (next = impl->current_child + 1; next < static_cast<int>(impl->children.size()); next++) {
    if (impl->children[next]->accepts_focus()) {
      impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
      impl->current_child = next;
      impl->children[next]->set_focus(window_component_t::FOCUS_IN_FWD);
      return true;
    }
  }
  return false;
}

bool widget_group_t::focus_previous_int() {
  int next;

  if (!impl->has_focus) {
    return false;
  }

  for (next = impl->current_child - 1; next >= 0; next--) {
    if (impl->children[next]->accepts_focus()) {
      impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
      impl->current_child = next;
      impl->children[next]->set_focus(window_component_t::FOCUS_IN_BCK);
      return true;
    }
  }
  return false;
}

void widget_group_t::focus_next() {
  if (!focus_next_int()) {
    move_focus_down();
  }
}

void widget_group_t::focus_previous() {
  if (!focus_previous_int()) {
    move_focus_up();
  }
}

void widget_group_t::add_child(std::unique_ptr<widget_t> child) {
  set_widget_parent(child.get());

  impl->children.push_back(std::move(child));
  if (impl->children.size() == 1) {
    impl->current_child = 0;
  }
}

widget_group_t::~widget_group_t() {}

bool widget_group_t::process_key(key_t key) {
  if (impl->children.size() == 0) {
    return false;
  }

  if (impl->children[impl->current_child]->process_key(key)) {
    return true;
  }

  if (key == '\t') {
    if (focus_next_int()) {
      return true;
    }
  } else if (key == (EKEY_SHIFT | '\t')) {
    if (focus_previous_int()) {
      return true;
    }
  }
  return false;
}

void widget_group_t::update_contents() {
  for (const std::unique_ptr<widget_t> &widget : impl->children) {
    widget->update_contents();
  }
}

void widget_group_t::set_focus(focus_t focus) {
  impl->has_focus = focus != window_component_t::FOCUS_OUT && impl->children.size() > 0;
  if (impl->children.size() == 0) {
    return;
  }
  switch (focus) {
    case window_component_t::FOCUS_OUT:
      impl->children[impl->current_child]->set_focus(focus);
      break;
    case window_component_t::FOCUS_SET:
    case window_component_t::FOCUS_IN_FWD:
      for (impl->current_child = 0; impl->current_child < static_cast<int>(impl->children.size()) &&
                                    !impl->children[impl->current_child]->accepts_focus();
           impl->current_child++) {
      }
      impl->children[impl->current_child]->set_focus(focus);
      break;
    case window_component_t::FOCUS_IN_BCK: {
      for (impl->current_child = impl->children.size() - 1;
           impl->current_child >= 0 && !impl->children[impl->current_child]->accepts_focus();
           impl->current_child--) {
      }
      impl->children[impl->current_child]->set_focus(focus);
      break;
    }
    case window_component_t::FOCUS_REVERT:
      impl->children[impl->current_child]->set_focus(focus);
      break;
    default:
      break;
  }
}

bool widget_group_t::set_size(optint height, optint width) {
  if (!width.is_valid()) {
    width = window.get_width();
  }
  if (!height.is_valid()) {
    height = window.get_height();
  }

  window.resize(height.value(), width.value());
  return true;
}

bool widget_group_t::accepts_focus() {
  for (const std::unique_ptr<widget_t> &widget : impl->children) {
    if (widget->accepts_focus()) {
      return true;
    }
  }
  return false;
}

void widget_group_t::force_redraw() {
  for (const std::unique_ptr<widget_t> &widget : impl->children) {
    widget->force_redraw();
  }
}

void widget_group_t::set_child_focus(window_component_t *target) {
  bool had_focus = impl->has_focus;
  impl->has_focus = true;
  for (int i = 0; i < static_cast<int>(impl->children.size()); i++) {
    container_t *container = nullptr;

    if (impl->children[i].get() == target ||
        ((container = dynamic_cast<container_t *>(impl->children[i].get())) != nullptr &&
         container->is_child(target))) {
      if (had_focus && i != impl->current_child) {
        impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
      }
      impl->current_child = i;

      if (impl->children[i].get() == target) {
        impl->children[i]->set_focus(window_component_t::FOCUS_SET);
      } else {
        container->set_child_focus(target);
      }
      return;
    }
  }
}

bool widget_group_t::is_child(window_component_t *component) {
  for (const std::unique_ptr<widget_t> &widget : impl->children) {
    container_t *container;
    if (widget.get() == component ||
        ((container = dynamic_cast<container_t *>(widget.get())) != nullptr &&
         container->is_child(component))) {
      return true;
    }
  }
  return false;
}

bool widget_group_t::is_hotkey(key_t key) const {
  widget_container_t *widget_container;
  for (const std::unique_ptr<widget_t> &widget : impl->children) {
    if (!widget->is_enabled() || !widget->is_shown()) {
      continue;
    }

    if (widget->is_hotkey(key)) {
      return true;
    }

    widget_container = dynamic_cast<widget_container_t *>(widget.get());
    if (widget_container != nullptr && widget_container->is_child_hotkey(key) != nullptr) {
      return true;
    }
  }
  return false;
}

}  // namespace t3widget
