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
#include "t3widget/widgets/checkbox.h"

#include <type_traits>

#include "t3widget/colorscheme.h"
#include "t3widget/internal.h"
#include "t3widget/widgets/smartlabel.h"
#include "t3widget/widgets/widget.h"
#include "t3window/terminal.h"
#include "t3window/window.h"

namespace t3widget {
namespace {
const char kStateToChar[] = " X:";
}

struct checkbox_t::implementation_t {
  /** Whether tri-state mode is enabled. */
  bool is_tristate;
  /** Current state (true if checked). */
  TriState state;
  /** Boolean indicating whether this widget should be drawn as focuessed. */
  bool has_focus = false;
  /** Label associated with this checkbox_t. Used for determining the hotkey. */
  smart_label_t *label = nullptr;
  signal_t<> activate;
  signal_t<> toggled;

  implementation_t(bool _state)
      : is_tristate(false), state(_state ? checkbox_t::CHECKED : checkbox_t::UNCHECKED) {}
  implementation_t(TriState _state) : is_tristate(true), state(_state) {}
};

checkbox_t::checkbox_t(bool state)
    : widget_t(1, 3, true,
               impl_alloc<focus_widget_t::implementation_t>(impl_alloc<implementation_t>(0))),
      focus_widget_t(this),
      impl(new_impl<implementation_t>(state)) {}

checkbox_t::checkbox_t(TriState state)
    : widget_t(1, 3, true,
               impl_alloc<focus_widget_t::implementation_t>(impl_alloc<implementation_t>(0))),
      focus_widget_t(this),
      impl(new_impl<implementation_t>(state)) {}

checkbox_t::~checkbox_t() {}

void checkbox_t::next_state() {
  if (impl->is_tristate) {
    switch (impl->state) {
      case UNCHECKED:
        impl->state = CHECKED;
        break;
      case CHECKED:
        impl->state = INDERMINATE;
        break;
      default:
        impl->state = UNCHECKED;
        break;
    }
  } else {
    if (impl->state == UNCHECKED) {
      impl->state = CHECKED;
    } else {
      impl->state = UNCHECKED;
    }
  }
  force_redraw();
  impl->toggled();
  update_contents();
}

bool checkbox_t::process_key(key_t key) {
  switch (key) {
    case ' ':
    case EKEY_HOTKEY:
      next_state();
      break;
    case EKEY_NL:
      impl->activate();
      break;
    case EKEY_LEFT:
      move_focus_left();
      return !impl->has_focus;
    case EKEY_RIGHT:
      move_focus_right();
      return !impl->has_focus;
    case EKEY_UP:
      move_focus_up();
      return !impl->has_focus;
    case EKEY_DOWN:
      move_focus_down();
      return !impl->has_focus;
    default:
      return false;
  }
  return true;
}

bool checkbox_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void checkbox_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }
  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.addch('[', 0);
  char c = '-';
  if (is_enabled()) {
    c = kStateToChar[std::min<int>(impl->state, 2)];
  }

  window.addch(c, impl->has_focus ? T3_ATTR_REVERSE : 0);
  window.addch(']', 0);
}

void checkbox_t::set_focus(focus_t focus) {
  if (impl->has_focus != focus) {
    force_redraw();
  }

  impl->has_focus = focus;
}

bool checkbox_t::get_state() { return impl->state == CHECKED; }

void checkbox_t::set_state(bool _state) {
  impl->state = _state ? CHECKED : UNCHECKED;
  force_redraw();
}

void checkbox_t::set_label(smart_label_t *_label) {
  if (impl->label != nullptr) {
    unregister_mouse_target(impl->label->get_base_window());
  }
  impl->label = _label;
  register_mouse_target(impl->label->get_base_window());
}

bool checkbox_t::is_hotkey(key_t key) const {
  return impl->label == nullptr ? false : impl->label->is_hotkey(key);
}

void checkbox_t::set_enabled(bool enable) {
  widget_t::set_enabled(enable);
  force_redraw();
}

bool checkbox_t::process_mouse_event(mouse_event_t event) {
  if (event.button_state & EMOUSE_CLICKED_LEFT) {
    next_state();
  }
  return true;
}

void checkbox_t::set_tristate_mode(bool is_tristate) {
  if (impl->is_tristate == is_tristate) {
    return;
  }
  if (impl->is_tristate && impl->state == INDERMINATE) {
    impl->state = UNCHECKED;
    force_redraw();
  }
  impl->is_tristate = is_tristate;
}

checkbox_t::TriState checkbox_t::get_tristate() const { return impl->state; }

void checkbox_t::set_tristate(TriState state) {
  impl->state = state;
  force_redraw();
}

_T3_WIDGET_IMPL_SIGNAL(checkbox_t, activate)
_T3_WIDGET_IMPL_SIGNAL(checkbox_t, toggled)

}  // namespace t3widget
