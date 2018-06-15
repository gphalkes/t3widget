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
#include "t3widget/widgets/label.h"
#include "t3widget/colorscheme.h"
#include "t3widget/log.h"
#include "t3widget/textline.h"
#include <t3window/window.h>

namespace t3_widget {

// FIXME: maybe we should allow scrolling with the left and right keys

label_t::label_t(const char *_text)
    : text(_text), align(ALIGN_LEFT), focus(false), can_focus(true) {
  int width = text_width = t3_term_strwidth(text.c_str());
  if (width == 0) {
    width = 1;
  }
  init_window(1, width, false);
}

bool label_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool label_t::set_size(optint height, optint width) {
  bool result = true;

  (void)height;
  if (width.is_valid() && t3_win_get_width(window) != width) {
    result = t3_win_resize(window, 1, width);
    redraw = true;
  }
  return result;
}

void label_t::update_contents() {
  int width;

  if (!redraw) {
    return;
  }
  redraw = false;

  width = t3_win_get_width(window);
  text_line_t line(&text);
  text_line_t::paint_info_t paint_info;

  t3_win_set_default_attrs(window, focus ? attributes.dialog_selected : attributes.dialog);
  t3_win_set_paint(window, 0, 0);
  t3_win_clrtoeol(window);
  int x = 0;
  if (width > text_width) {
    switch (align) {
      default:
        break;
      case ALIGN_RIGHT:
      case ALIGN_RIGHT_UNDERFLOW:
        x = width - text_width;
        break;
      case ALIGN_CENTER:
        x = (width - text_width) / 2;
        break;
    }
  }
  t3_win_set_paint(window, 0, x);

  paint_info.start = 0;
  if (width < text_width && (align == ALIGN_LEFT_UNDERFLOW || align == ALIGN_RIGHT_UNDERFLOW)) {
    paint_info.leftcol = text_width - width + 2;
    paint_info.size = width - 2;
    t3_win_addstr(window, "..", 0);
  } else {
    paint_info.leftcol = 0;
    paint_info.size = width;
  }
  paint_info.max = INT_MAX;
  paint_info.tabsize = 0;
  paint_info.flags = text_line_t::TAB_AS_CONTROL;
  paint_info.selection_start = -1;
  paint_info.selection_end = -1;
  paint_info.cursor = -1;
  paint_info.normal_attr = 0;
  paint_info.selected_attr = 0;

  line.paint_line(window, &paint_info);
}

void label_t::set_focus(focus_t _focus) {
  redraw = true;
  focus = _focus;
}

void label_t::set_align(label_t::align_t _align) { align = _align; }

void label_t::set_text(const char *_text) {
  text = _text;
  text_width = t3_term_strwidth(text.c_str());
  redraw = true;
}

int label_t::get_text_width() const { return text_width; }

void label_t::set_accepts_focus(bool _can_focus) { can_focus = _can_focus; }
bool label_t::accepts_focus() { return can_focus && widget_t::accepts_focus(); }

};  // namespace
