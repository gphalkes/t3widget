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

button_t::button_t(const char *_text, bool _is_default)
    : text(new smart_label_t(_text)), is_default(_is_default) {
  text_width = text->get_width();
  init_window(1, text_width + 4);
  has_focus = false;
}

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
    if (width <= 0) {
      if (text_width + 4 == window.get_width()) {
        return true;
      }
      width = text_width + 4;
    }
    return window.resize(1, width);
  }
  return true;
}

void button_t::update_contents() {
  t3_attr_t attr;
  int width;

  if (!redraw) {
    return;
  }
  redraw = false;

  attr = has_focus ? attributes.button_selected : 0;

  width = window.get_width();

  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.addstr(is_default ? "[<" : "[ ", attr);
  if (width > text_width + 4) {
    window.addchrep(' ', attr, (width - 4 - text_width) / 2);
  }
  text->draw(&window, attr, has_focus);
  if (width > text_width + 4) {
    window.addchrep(' ', attr, (width - 4 - text_width + 1) / 2);
  } else if (width > 0) {
    window.set_paint(0, width - 2);
  }
  window.addstr(is_default ? ">]" : " ]", attr);
}

void button_t::set_focus(focus_t focus) {
  if (focus != has_focus) {
    redraw = true;
  }

  has_focus = focus;
}

bool button_t::process_mouse_event(mouse_event_t event) {
  if (event.button_state & EMOUSE_CLICKED_LEFT) {
    activate();
  }
  return true;
}

int button_t::get_width() { return window.get_width(); }

bool button_t::is_hotkey(key_t key) { return text->is_hotkey(key); }

}  // namespace
