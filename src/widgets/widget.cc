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
#include "t3widget/widgets/widget.h"

#include <type_traits>
#include <typeinfo>
#include <utility>

#include "t3widget/interfaces.h"
#include "t3widget/internal.h"
#include "t3widget/log.h"

namespace t3widget {

struct widget_t::implementation_t {
  bool redraw = true, /**< Widget requires redrawing on next #update_contents call. */
      enabled = true, /**< Widget is enabled. */
      shown = true;   /**< Widget is shown. */
};

/* The default_parent must exist before any widgets are created. Thus using the #on_init method
   won't work. */
t3window::window_t widget_t::default_parent(nullptr, 1, 1, 0, 0, 0, false);

bool widget_t::reset_redraw() {
  bool result = impl->redraw;
  impl->redraw = false;
  return result;
}

bool widget_t::is_hotkey(key_t key) const {
  (void)key;
  return false;
}

bool widget_t::accepts_focus() const { return impl->enabled && impl->shown; }

widget_t::widget_t(int height, int width, bool register_as_mouse_target, size_t impl_size)
    : impl_allocator_t(impl_alloc<implementation_t>(impl_size)),
      impl(new_impl<implementation_t>()) {
  init_window(height, width, register_as_mouse_target);
}

widget_t::widget_t(size_t impl_size)
    : impl_allocator_t(impl_alloc<implementation_t>(impl_size)),
      impl(new_impl<implementation_t>()) {}

widget_t::~widget_t() {}

void widget_t::init_window(int height, int width, bool register_as_mouse_target) {
  window.alloc(&default_parent, height, width, 0, 0, 0);
  window.show();
  if (register_as_mouse_target) {
    register_mouse_target(&window);
  }
}

void widget_t::init_unbacked_window(int height, int width, bool register_as_mouse_target) {
  window.alloc_unbacked(&default_parent, height, width, 0, 0, 0);
  window.show();
  if (register_as_mouse_target) {
    register_mouse_target(&window);
  }
}

void widget_t::set_anchor(window_component_t *anchor, int relation) {
  window.set_anchor(anchor->get_base_window(), relation);
}

void widget_t::set_position(optint top, optint left) {
  if (!top.is_valid()) {
    top = window.get_y();
  }
  if (!left.is_valid()) {
    left = window.get_x();
  }

  window.move(top.value(), left.value());
}

void widget_t::show() {
  window.show();
  impl->shown = true;
}

void widget_t::hide() {
  window.hide();
  impl->shown = false;
}

void widget_t::force_redraw() { impl->redraw = true; }

void widget_t::set_enabled(bool enable) { impl->enabled = enable; }

bool widget_t::is_enabled() const { return impl->enabled; }

bool widget_t::is_shown() const { return impl->shown; }

void widget_t::set_focus(focus_t focus) {
  (void)focus;
  return;
}

bool widget_t::process_mouse_event(mouse_event_t event) {
  lprintf("Default mouse handling for %s (%d)\n", typeid(*this).name(), accepts_focus());
  return accepts_focus() && (event.button_state & EMOUSE_CLICK_BUTTONS);
}

struct focus_widget_t::implementation_t {
  signal_t<> move_focus_left;
  signal_t<> move_focus_right;
  signal_t<> move_focus_up;
  signal_t<> move_focus_down;
  bool must_delete;
  implementation_t(bool _must_delete) : must_delete(_must_delete) {}
};

focus_widget_t::focus_widget_t(t3widget::impl_allocator_t *allocator) {
  if (allocator) {
    impl = allocator->new_impl<implementation_t>(false);
  } else {
    impl = new implementation_t(true);
  }
}

focus_widget_t::~focus_widget_t() {
  if (impl->must_delete) {
    delete impl;
  } else {
    impl->~implementation_t();
  }
}

template <>
size_t impl_allocator_t::impl_alloc<focus_widget_t::implementation_t>(size_t impl_size) {
  return impl_allocator_t::impl_alloc_internal<focus_widget_t::implementation_t>(impl_size);
}

_T3_WIDGET_IMPL_SIGNAL(focus_widget_t, move_focus_left)
_T3_WIDGET_IMPL_SIGNAL(focus_widget_t, move_focus_right)
_T3_WIDGET_IMPL_SIGNAL(focus_widget_t, move_focus_up)
_T3_WIDGET_IMPL_SIGNAL(focus_widget_t, move_focus_down)

void focus_widget_t::move_focus_left() const { impl->move_focus_left(); }
void focus_widget_t::move_focus_right() const { impl->move_focus_right(); }
void focus_widget_t::move_focus_up() const { impl->move_focus_up(); }
void focus_widget_t::move_focus_down() const { impl->move_focus_down(); }

std::function<void()> focus_widget_t::get_move_focus_left_trigger() const {
  return impl->move_focus_left.get_trigger();
}
std::function<void()> focus_widget_t::get_move_focus_right_trigger() const {
  return impl->move_focus_right.get_trigger();
}
std::function<void()> focus_widget_t::get_move_focus_up_trigger() const {
  return impl->move_focus_up.get_trigger();
}
std::function<void()> focus_widget_t::get_move_focus_down_trigger() const {
  return impl->move_focus_down.get_trigger();
}

}  // namespace t3widget
