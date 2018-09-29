/* Copyright (C) 2011-2013,2018 G.P. Halkes
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

#include "t3widget/colorscheme.h"
#include "t3widget/internal.h"
#include "t3widget/util.h"
#include "t3widget/widgets/textwindow.h"
#include "t3widget/wrapinfo.h"

namespace t3widget {

struct text_window_t::implementation_t {
  std::unique_ptr<scrollbar_t> scrollbar;
  text_buffer_t *text;
  std::unique_ptr<wrap_info_t> wrap_info;
  text_coordinate_t top;
  signal_t<> activate;
  bool focus;

  implementation_t() : top(0, 0), focus(false) {}
};

text_window_t::text_window_t(text_buffer_t *_text, bool with_scrollbar)
    : widget_t(11, 11, false, impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()) {
  /* Note: we use a dirty trick here: the last position of the window is obscured by
     the impl->scrollbar-> However, the last position will only contain the wrap character
     anyway, so we don't care. If the impl->scrollbar is disabled, we set the wrap width
     one higher, such that the wrap character falls off the edge. */
  if (with_scrollbar) {
    impl->scrollbar.reset(new scrollbar_t(true));
    container_t::set_widget_parent(impl->scrollbar.get());
    impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
    impl->scrollbar->set_size(11, None);
    impl->scrollbar->connect_clicked(bind_front(&text_window_t::scrollbar_clicked, this));
    impl->scrollbar->connect_dragged(bind_front(&text_window_t::scrollbar_dragged, this));
  }

  if (_text == nullptr) {
    impl->text = new text_buffer_t();
  } else {
    impl->text = _text;
  }

  impl->wrap_info.reset(new wrap_info_t(impl->scrollbar != nullptr ? 11 : 12));
  impl->wrap_info->set_text_buffer(impl->text);
}

text_window_t::~text_window_t() {}

void text_window_t::set_text(text_buffer_t *_text) {
  if (impl->text == _text) {
    return;
  }

  impl->text = _text;
  impl->wrap_info->set_text_buffer(impl->text);
  impl->top.line = 0;
  impl->top.pos = 0;
  force_redraw();
}

bool text_window_t::set_size(optint height, optint width) {
  bool result = true;

  if (!width.is_valid()) {
    width = window.get_width();
  }
  if (!height.is_valid()) {
    height = window.get_height();
  }

  if (width.value() != window.get_width() || height.value() > window.get_height()) {
    force_redraw();
  }

  result &= window.resize(height.value(), width.value());
  if (impl->scrollbar != nullptr) {
    result &= impl->scrollbar->set_size(height, None);
    impl->wrap_info->set_wrap_width(width.value());
  } else {
    impl->wrap_info->set_wrap_width(width.value() + 1);
  }

  return result;
}

void text_window_t::set_scrollbar(bool with_scrollbar) {
  if (with_scrollbar == (impl->scrollbar != nullptr)) {
    return;
  }
  if (with_scrollbar) {
    impl->scrollbar.reset(new scrollbar_t(true));
    set_widget_parent(impl->scrollbar.get());
    impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
    impl->scrollbar->set_size(window.get_height(), None);
    impl->wrap_info->set_wrap_width(window.get_width());
  } else {
    impl->scrollbar = nullptr;
    impl->wrap_info->set_wrap_width(window.get_width() + 1);
  }
  force_redraw();
}

void text_window_t::scroll_down(int lines) {
  text_coordinate_t new_top = impl->top;

  if (impl->wrap_info->add_lines(new_top, window.get_height() + lines)) {
    impl->wrap_info->sub_lines(new_top, window.get_height() - 1);
    if (impl->top != new_top) {
      impl->top = new_top;
      force_redraw();
    }
  } else {
    impl->wrap_info->add_lines(impl->top, lines);
    force_redraw();
  }
}

void text_window_t::scroll_up(int lines) {
  if (impl->top.line == 0 && impl->top.pos == 0) {
    return;
  }

  impl->wrap_info->sub_lines(impl->top, lines);
  force_redraw();
}

bool text_window_t::process_key(key_t key) {
  switch (key) {
    case EKEY_NL:
      impl->activate();
      break;
    case EKEY_DOWN:
      scroll_down(1);
      break;
    case EKEY_UP:
      scroll_up(1);
      break;
    case EKEY_PGUP:
      scroll_up(window.get_height() - 1);
      break;
    case EKEY_PGDN:
    case ' ':
      scroll_down(window.get_height() - 1);
      break;
    case EKEY_HOME | EKEY_CTRL:
    case EKEY_HOME:
      if (impl->top.line != 0 || impl->top.pos != 0) {
        impl->top.line = 0;
        impl->top.pos = 0;
        force_redraw();
      }
      break;
    case EKEY_END | EKEY_CTRL:
    case EKEY_END: {
      text_coordinate_t new_top = impl->wrap_info->get_end();
      impl->wrap_info->sub_lines(new_top, window.get_height());
      if (new_top != impl->top) {
        force_redraw();
      }
      break;
    }
    default:
      return false;
  }
  return true;
}

void text_window_t::update_contents() {
  text_coordinate_t current_start, current_end;
  text_line_t::paint_info_t info;
  int i, count = 0;

  if (!reset_redraw()) {
    return;
  }

  window.set_default_attrs(attributes.dialog);

  info.size = window.get_width();
  if (impl->scrollbar == nullptr) {
    info.size++;
  }
  info.normal_attr = 0;
  info.selected_attr = 0; /* There is no selected impl->text anyway. */
  info.selection_start = -1;
  info.selection_end = -1;
  info.cursor = -1;
  info.leftcol = 0;
  info.flags = 0;

  text_coordinate_t end = impl->wrap_info->get_end();
  text_coordinate_t draw_line = impl->top;

  for (i = 0; i < window.get_height(); i++, impl->wrap_info->add_lines(draw_line, 1)) {
    if (impl->focus) {
      if (i == 0) {
        info.cursor = impl->wrap_info->calculate_line_pos(draw_line.line, 0, draw_line.pos);
      } else {
        info.cursor = -1;
      }
    }
    window.set_paint(i, 0);
    window.clrtoeol();
    impl->wrap_info->paint_line(&window, draw_line, info);

    if (draw_line.line == end.line && draw_line.pos == end.pos) {
      break;
    }
  }

  window.clrtobot();

  for (i = 0; i < impl->top.line; i++) {
    count += impl->wrap_info->get_line_count(i);
  }
  count += impl->top.pos;

  if (impl->scrollbar != nullptr) {
    impl->scrollbar->set_parameters(
        std::max(impl->wrap_info->get_text_size(), count + window.get_height()), count,
        window.get_height());
    impl->scrollbar->update_contents();
  }
}

void text_window_t::set_focus(focus_t _focus) {
  if (impl->focus != _focus) {
    force_redraw();
  }
  impl->focus = _focus;
}

void text_window_t::set_child_focus(window_component_t *target) {
  (void)target;
  set_focus(window_component_t::FOCUS_SET);
}

bool text_window_t::is_child(const window_component_t *component) const {
  return component == impl->scrollbar.get();
}

text_buffer_t *text_window_t::get_text() { return impl->text; }

void text_window_t::set_tabsize(int size) { impl->wrap_info->set_tabsize(size); }

int text_window_t::get_text_height() { return impl->wrap_info->get_text_size(); }

bool text_window_t::process_mouse_event(mouse_event_t event) {
  if (event.window != window || event.type != EMOUSE_BUTTON_PRESS) {
    return true;
  }
  if (event.button_state & EMOUSE_SCROLL_UP) {
    scroll_up(3);
  } else if (event.button_state & EMOUSE_SCROLL_DOWN) {
    scroll_down(3);
  }
  return true;
}

void text_window_t::scrollbar_clicked(scrollbar_t::step_t step) {
  int scroll =
      step == scrollbar_t::BACK_SMALL
          ? -3
          : step == scrollbar_t::FWD_SMALL
                ? 3
                : step == scrollbar_t::BACK_MEDIUM
                      ? -window.get_height() / 2
                      : step == scrollbar_t::FWD_MEDIUM
                            ? window.get_height() / 2
                            : step == scrollbar_t::BACK_PAGE
                                  ? -(window.get_height() - 1)
                                  : step == scrollbar_t::FWD_PAGE ? (window.get_height() - 1) : 0;

  if (scroll < 0) {
    scroll_up(-scroll);
  } else {
    scroll_down(scroll);
  }
}

void text_window_t::scrollbar_dragged(int start) {
  text_coordinate_t new_top_left(0, 0);
  int count = 0;

  if (start < 0 || start + window.get_height() > impl->wrap_info->get_text_size()) {
    return;
  }

  for (new_top_left.line = 0; new_top_left.line < impl->text->size() && count < start;
       new_top_left.line++) {
    count += impl->wrap_info->get_line_count(new_top_left.line);
  }

  if (count > start) {
    new_top_left.line--;
    count -= impl->wrap_info->get_line_count(new_top_left.line);
    new_top_left.pos = start - count;
  }

  if (new_top_left == impl->top || new_top_left.line < 0) {
    return;
  }
  impl->top = new_top_left;
  force_redraw();
}

_T3_WIDGET_IMPL_SIGNAL(text_window_t, activate)

}  // namespace t3widget
