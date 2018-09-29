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
#include "t3widget/widgets/frame.h"
#include "t3widget/colorscheme.h"

namespace t3widget {

struct frame_t::implementation_t {
  frame_dimension_t dimension;     /**< Requested overlaps. */
  std::unique_ptr<widget_t> child; /**< The widget to enclose. */
  implementation_t(frame_dimension_t _dimension) : dimension(_dimension) {}
};

frame_t::frame_t(frame_dimension_t _dimension)
    : widget_t(3, 3, false, impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>(_dimension)) {}

frame_t::~frame_t() {}

void frame_t::set_child(widget_t *_child) {
  int child_top = 1, child_left = 1;

  impl->child.reset(_child);

  if (impl->dimension & COVER_TOP) {
    child_top = 0;
  }
  if (impl->dimension & COVER_LEFT) {
    child_left = 0;
  }
  set_widget_parent(impl->child.get());
  impl->child->set_anchor(this, 0);
  impl->child->set_position(child_top, child_left);
  set_size(None, None);
}

bool frame_t::process_key(key_t key) {
  return impl->child != nullptr ? impl->child->process_key(key) : false;
}
void frame_t::update_contents() {
  if (impl->child != nullptr) {
    impl->child->update_contents();
  }
  if (!reset_redraw()) {
    return;
  }
  window.set_default_attrs(attributes.dialog);

  window.set_paint(0, 0);
  window.clrtobot();
  window.box(0, 0, window.get_height(), window.get_width(), 0);
}

void frame_t::set_focus(focus_t focus) {
  if (impl->child != nullptr) {
    impl->child->set_focus(focus);
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
  force_redraw();

  if (impl->child != nullptr) {
    child_height = window.get_height();
    if (!(impl->dimension & COVER_TOP)) {
      child_height--;
    }
    if (!(impl->dimension & COVER_BOTTOM)) {
      child_height--;
    }
    child_width = window.get_width();
    if (!(impl->dimension & COVER_LEFT)) {
      child_width--;
    }
    if (!(impl->dimension & COVER_RIGHT)) {
      child_width--;
    }

    result &= impl->child->set_size(child_height, child_width);
  }
  return result;
}

bool frame_t::accepts_focus() const {
  return impl->child != nullptr ? impl->child->accepts_focus() : false;
}
bool frame_t::is_hotkey(key_t key) const {
  return impl->child != nullptr ? impl->child->is_hotkey(key) : false;
}
void frame_t::set_enabled(bool enable) {
  if (impl->child != nullptr) {
    impl->child->set_enabled(enable);
  }
}
void frame_t::force_redraw() {
  if (impl->child != nullptr) {
    impl->child->force_redraw();
  }
}

void frame_t::set_child_focus(window_component_t *target) {
  container_t *container;
  if (target == impl->child.get()) {
    impl->child->set_focus(window_component_t::FOCUS_SET);
  }
  container = dynamic_cast<container_t *>(impl->child.get());
  if (container != nullptr) {
    container->set_child_focus(target);
  }
}

bool frame_t::is_child(const window_component_t *component) const {
  container_t *container;
  if (component == impl->child.get()) {
    return true;
  }
  container = dynamic_cast<container_t *>(impl->child.get());
  return container != nullptr && container->is_child(component);
}

}  // namespace t3widget
