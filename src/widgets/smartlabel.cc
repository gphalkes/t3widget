/* Copyright (C) 2011-2012 G.P. Halkes
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
#include <climits>
#include <cstring>
#include <t3window/utf8.h>
#include <t3window/window.h>
#include <unicase.h>
#include <vector>

#include "colorscheme.h"
#include "internal.h"
#include "textline.h"
#include "util.h"
#include "widgets/smartlabel.h"
#include "widgets/widget.h"

namespace t3_widget {

static uint32_t casefold_single(uint32_t c) {
  uint32_t result[8];
  size_t result_size = sizeof(result) / sizeof(result[0]);

  /* Case folding never results in more than two codepoints for case folding a
     single codepoint. Thus, we can use a static buffer, as long as it's big
     enough. Just in case, we use a buffer of size 8. */
  return u32_casefold(&c, 1, nullptr, nullptr, result, &result_size) == nullptr || result_size > 1
             ? 0
             : result[0];
}

smart_label_text_t::smart_label_text_t(const char *spec, bool _add_colon)
    : add_colon(_add_colon), underlined(false), hotkey(0) {
  cleanup_ptr<text_line_t>::t line;
  char *underline_ptr;

  text_length = strlen(spec);
  if ((text = _t3_widget_strdup(spec)) == nullptr) throw std::bad_alloc();

  if ((underline_ptr = strchr(text, '_')) != nullptr) {
    size_t src_size;

    underlined = true;
    underline_start = underline_ptr - text;
    memmove(underline_ptr, underline_ptr + 1, text_length - underline_start);
    text_length--;

    src_size = text_length - underline_start;
    hotkey = casefold_single(t3_utf8_get(underline_ptr, &src_size));

    line = new text_line_t(text, text_length);
    underline_length = line->adjust_position(underline_start, 1) - underline_start;
  }
}

smart_label_text_t::~smart_label_text_t() {}

void smart_label_text_t::draw(t3_window_t *window, int attr, bool selected) {
  if (!underlined) {
    t3_win_addnstr(window, text, text_length, attr);
  } else {
    t3_win_addnstr(window, text, underline_start, attr);
    t3_win_addnstr(window, text + underline_start, underline_length,
                   selected ? attr : t3_term_combine_attrs(attributes.hotkey_highlight, attr));
    t3_win_addnstr(window, text + underline_start + underline_length,
                   text_length - underline_start - underline_length, attr);
  }
  if (add_colon) t3_win_addch(window, ':', attr);
}

int smart_label_text_t::get_width() { return t3_term_strwidth(text) + (add_colon ? 1 : 0); }

bool smart_label_text_t::is_hotkey(key_t key) {
  if (hotkey == 0) return false;

  return (key_t)casefold_single(key & 0x1fffffl) == hotkey;
}

//======= smart_label_t =======
smart_label_t::smart_label_t(const char *spec, bool _add_colon)
    : smart_label_text_t(spec, _add_colon), widget_t(1, get_width(), false) {}

bool smart_label_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool smart_label_t::set_size(optint height, optint width) {
  (void)height;
  if (!width.is_valid()) width = t3_win_get_width(window);
  t3_win_resize(window, 1, width);
  return true;
}

void smart_label_t::update_contents() {
  if (!redraw) return;
  redraw = false;
  t3_win_set_paint(window, 0, 0);
  draw(window, attributes.dialog);
}

void smart_label_t::set_focus(focus_t focus) { (void)focus; }

bool smart_label_t::is_hotkey(key_t key) { return smart_label_text_t::is_hotkey(key); }

bool smart_label_t::accepts_focus() { return false; }

};  // namespace
