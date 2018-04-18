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
#include <cstring>

#include "colorscheme.h"
#include "widgets/button.h"

namespace t3_widget {

struct button_t::implementation_t {
  /** Text to display on the button. */
  smart_label_text_t text;

  /** Width of the text. */
  int text_width;
  /** Boolean indicating whether this button should be drawn as the default button.
      The default button is the button that displays the action taken when
      the enter key is pressed inside another widget on the same dialog.
      It is drawn differently from other buttons, and there should be only
      one such button on each dialog. */
  bool is_default,
      has_focus = false; /**< Boolean indicating whether this button has the input focus. */
  implementation_t(const char *_text, bool _is_default, impl_allocator_t *allocator)
      : text(_text, false, allocator), is_default(_is_default) {
    text_width = text.get_width();
  }
};

button_t::button_t(const char *_text, bool _is_default)
    : widget_t(impl_alloc<implementation_t>(smart_label_text_t::impl_alloc(0))),
      impl(new_impl<implementation_t>(_text, _is_default, this)) {
  init_window(1, impl->text_width + 4);
}

button_t::~button_t() {}

bool button_t::process_key(key_t key) {
  switch (key) {
    case EKEY_HOTKEY:
    case EKEY_NL:
    case ' ':
      activate();
      break;
    case EKEY_LEFT:
      move_focus_left();
      break;
    case EKEY_RIGHT:
      move_focus_right();
      break;
    case EKEY_UP:
      move_focus_up();
      break;
    case EKEY_DOWN:
      move_focus_down();
      break;
    default:
      return false;
  }
  return true;
}

bool button_t::set_size(optint height, optint width) {
  (void)height;

  if (width.is_valid()) {
    if (width.value() <= 0) {
      if (impl->text_width + 4 == window.get_width()) {
        return true;
      }
      width = impl->text_width + 4;
    }
    return window.resize(1, width.value());
  }
  return true;
}

void button_t::update_contents() {
  t3_attr_t attr;
  int width;

  if (!reset_redraw()) {
    return;
  }

  attr = impl->has_focus ? attributes.button_selected : 0;

  width = window.get_width();

  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.addstr(impl->is_default ? "[<" : "[ ", attr);
  if (width > impl->text_width + 4) {
    window.addchrep(' ', attr, (width - 4 - impl->text_width) / 2);
  }
  impl->text.draw(&window, attr, impl->has_focus);
  if (width > impl->text_width + 4) {
    window.addchrep(' ', attr, (width - 4 - impl->text_width + 1) / 2);
  } else if (width > 0) {
    window.set_paint(0, width - 2);
  }
  window.addstr(impl->is_default ? ">]" : " ]", attr);
}

void button_t::set_focus(focus_t focus) {
  if (focus != impl->has_focus) {
    force_redraw();
  }

  impl->has_focus = focus;
}

bool button_t::process_mouse_event(mouse_event_t event) {
  if (event.button_state & EMOUSE_CLICKED_LEFT) {
    activate();
  }
  return true;
}

int button_t::get_width() { return window.get_width(); }

bool button_t::is_hotkey(key_t key) const { return impl->text.is_hotkey(key); }

}  // namespace
