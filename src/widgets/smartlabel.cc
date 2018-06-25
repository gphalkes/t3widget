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
#include <algorithm>
#include <climits>
#include <cstring>
#include <t3window/utf8.h>
#include <t3window/window.h>
#include <unicase.h>
#include <vector>

#include "t3widget/colorscheme.h"
#include "t3widget/internal.h"
#include "t3widget/textline.h"
#include "t3widget/util.h"
#include "t3widget/widgets/smartlabel.h"
#include "t3widget/widgets/widget.h"

namespace t3widget {

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

struct smart_label_text_t::implementation_t {
  bool add_colon;
  std::string text;
  size_t underline_start, underline_length;
  bool underlined;
  key_t hotkey;
  bool must_delete;

  implementation_t(string_view spec, bool _add_colon, bool _must_delete)
      : add_colon(_add_colon), text(spec), underlined(false), hotkey(0), must_delete(_must_delete) {
    if ((underline_start = text.find('_')) != std::string::npos) {
      size_t src_size;

      underlined = true;
      text.erase(underline_start, 1);

      src_size = text.size() - underline_start;
      hotkey = casefold_single(t3_utf8_get(text.data() + underline_start, &src_size));

      text_line_t line(text);
      underline_length = line.adjust_position(underline_start, 1) - underline_start;
    }
  }
};

smart_label_text_t::smart_label_text_t(string_view spec, bool add_colon,
                                       impl_allocator_t *allocator) {
  if (allocator) {
    impl = allocator->new_impl<implementation_t>(spec, add_colon, false);
  } else {
    impl = new implementation_t(spec, add_colon, true);
  }
}

smart_label_text_t::~smart_label_text_t() {
  if (impl->must_delete) {
    delete impl;
  } else {
    impl->~implementation_t();
  }
}

void smart_label_text_t::draw(t3window::window_t *window, t3_attr_t attr, bool selected) {
  const std::string &text = impl->text;
  size_t underline_start = impl->underline_start;
  size_t underline_length = impl->underline_length;

  if (!impl->underlined) {
    window->addnstr(text.data(), text.size(), attr);
  } else {
    window->addnstr(text.data(), underline_start, attr);
    window->addnstr(text.data() + underline_start, underline_length,
                    selected ? attr : t3_term_combine_attrs(attributes.hotkey_highlight, attr));
    window->addnstr(text.data() + underline_start + underline_length,
                    text.size() - underline_start - underline_length, attr);
  }
  if (impl->add_colon) {
    window->addch(':', attr);
  }
}

int smart_label_text_t::get_width() const {
  return t3_term_strnwidth(impl->text.data(), impl->text.size()) + (impl->add_colon ? 1 : 0);
}

bool smart_label_text_t::is_hotkey(key_t key) const {
  if (impl->hotkey == 0) {
    return false;
  }

  return static_cast<key_t>(casefold_single(key & 0x1fffffl)) == impl->hotkey;
}

size_t smart_label_text_t::impl_alloc(size_t impl_size) {
  return impl_allocator_t::impl_alloc<implementation_t>(impl_size);
}

//======= smart_label_t =======
smart_label_t::smart_label_t(string_view spec, bool _add_colon)
    : widget_t(smart_label_text_t::impl_alloc(0)), smart_label_text_t(spec, _add_colon, this) {
  init_window(1, get_width(), false);
}

bool smart_label_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool smart_label_t::set_size(optint height, optint width) {
  (void)height;
  if (!width.is_valid()) {
    width = window.get_width();
  }
  window.resize(1, width.value());
  return true;
}

void smart_label_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }
  window.set_paint(0, 0);
  draw(&window, attributes.dialog);
}

void smart_label_t::set_focus(focus_t focus) { (void)focus; }

bool smart_label_t::is_hotkey(key_t key) const { return smart_label_text_t::is_hotkey(key); }

bool smart_label_t::accepts_focus() { return false; }

}  // namespace t3widget
