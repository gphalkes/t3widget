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
#include "widgets/frame.h"
#include "colorscheme.h"

namespace t3_widget {

frame_t::frame_t(frame_dimension_t _dimension)
    : widget_t(3, 3, false), dimension(_dimension), child(nullptr) {}

void frame_t::set_child(widget_t *_child) {
  int child_top = 1, child_left = 1;

  child.reset(_child);

  if (dimension & COVER_TOP) {
    child_top = 0;
  }
  if (dimension & COVER_LEFT) {
    child_left = 0;
  }
  set_widget_parent(child.get());
  child->set_anchor(this, 0);
  child->set_position(child_top, child_left);
  set_size(None, None);
}

bool frame_t::process_key(key_t key) { return child != nullptr ? child->process_key(key) : false; }
void frame_t::update_contents() {
  if (child != nullptr) {
    child->update_contents();
  }
  if (!redraw) {
    return;
  }
  redraw = false;
  window.set_default_attrs(attributes.dialog);

  window.set_paint(0, 0);
  window.clrtobot();
  window.box(0, 0, window.get_height(), window.get_width(), 0);
}

void frame_t::set_focus(focus_t focus) {
  if (child != nullptr) {
    child->set_focus(focus);
  }
}

bool frame_t::set_size(optint height, optint width) {
  int child_height, child_width;
  bool result;

  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  result = window.resize(height.value(), width.value());
  redraw = true;

  if (child != nullptr) {
    child_height = window.get_height();
    if (!(dimension & COVER_TOP)) {
      child_height--;
    }
    if (!(dimension & COVER_BOTTOM)) {
      child_height--;
    }
    child_width = window.get_width();
    if (!(dimension & COVER_LEFT)) {
      child_width--;
    }
    if (!(dimension & COVER_RIGHT)) {
      child_width--;
    }

    result &= child->set_size(child_height, child_width);
  }
  return result;
}

bool frame_t::accepts_focus() { return child != nullptr ? child->accepts_focus() : false; }
bool frame_t::is_hotkey(key_t key) { return child != nullptr ? child->is_hotkey(key) : false; }
void frame_t::set_enabled(bool enable) {
  if (child != nullptr) {
    child->set_enabled(enable);
  }
}
void frame_t::force_redraw() {
  if (child != nullptr) {
    child->force_redraw();
  }
}

void frame_t::set_child_focus(window_component_t *target) {
  container_t *container;
  if (target == child.get()) {
    child->set_focus(window_component_t::FOCUS_SET);
  }
  container = dynamic_cast<container_t *>(child.get());
  if (container != nullptr) {
    container->set_child_focus(target);
  }
}

bool frame_t::is_child(window_component_t *component) {
  container_t *container;
  if (component == child.get()) {
    return true;
  }
  container = dynamic_cast<container_t *>(child.get());
  return container != nullptr && container->is_child(component);
}

}  // namespace
