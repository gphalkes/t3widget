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

namespace t3widget {

// FIXME: maybe we should allow scrolling with the left and right keys
struct label_t::implementation_t {
  std::string text;           /**< Text currently displayed. */
  int text_width;             /**< Width of the text if displayed in full. */
  align_t align = ALIGN_LEFT; /**< Text alignment. Default is #ALIGN_LEFT. */
  /** Boolean indicating whether this label_t has the input focus. */
  bool focus = false;
  /** Boolean indicating whether this label_t will accept the input focus. Default is @c true. */
  bool can_focus = false;
  implementation_t(string_view _text) : text(_text) {}
};

label_t::label_t(string_view _text)
    : widget_t(impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>(std::move(_text))) {
  int width = impl->text_width = t3_term_strwidth(impl->text.c_str());
  if (width == 0) {
    width = 1;
  }
  init_window(1, width, false);
}

label_t::~label_t() {}

bool label_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool label_t::set_size(optint height, optint width) {
  bool result = true;

  (void)height;
  if (width.is_valid() && window.get_width() != width.value()) {
    result = window.resize(1, width.value());
    force_redraw();
  }
  return result;
}

void label_t::update_contents() {
  int width;

  if (!reset_redraw()) {
    return;
  }

  width = window.get_width();
  text_line_t line(impl->text);
  text_line_t::paint_info_t paint_info;

  window.set_default_attrs(impl->focus ? attributes.dialog_selected : attributes.dialog);
  window.set_paint(0, 0);
  window.clrtoeol();

  int &text_width = impl->text_width;

  int x = 0;
  if (width > text_width) {
    switch (impl->align) {
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
  window.set_paint(0, x);

  paint_info.start = 0;
  if (width < text_width &&
      (impl->align == ALIGN_LEFT_UNDERFLOW || impl->align == ALIGN_RIGHT_UNDERFLOW)) {
    paint_info.leftcol = text_width - width + 2;
    paint_info.size = width - 2;
    window.addstr("..", 0);
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

  line.paint_line(&window, paint_info);
}

void label_t::set_focus(focus_t _focus) {
  force_redraw();
  impl->focus = _focus;
}

void label_t::set_align(label_t::align_t _align) { impl->align = _align; }

void label_t::set_text(t3widget::string_view _text) {
  impl->text = std::string(_text);
  impl->text_width = t3_term_strnwidth(impl->text.data(), impl->text.size());
  force_redraw();
}

int label_t::get_text_width() const { return impl->text_width; }

void label_t::set_accepts_focus(bool _can_focus) { impl->can_focus = _can_focus; }
bool label_t::accepts_focus() const { return impl->can_focus && widget_t::accepts_focus(); }

}  // namespace t3widget
