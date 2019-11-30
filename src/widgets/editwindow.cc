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
#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <stdio.h>
#include <string>
#include <type_traits>
#include <utility>

#include "t3widget/autocompleter.h"
#include "t3widget/clipboard.h"
#include "t3widget/colorscheme.h"
#include "t3widget/contentlist.h"
#include "t3widget/dialogs/finddialog.h"
#include "t3widget/dialogs/gotodialog.h"
#include "t3widget/dialogs/insertchardialog.h"
#include "t3widget/dialogs/menupanel.h"
#include "t3widget/dialogs/messagedialog.h"
#include "t3widget/dialogs/popup.h"
#include "t3widget/findcontext.h"
#include "t3widget/interfaces.h"
#include "t3widget/internal.h"
#include "t3widget/key.h"
#include "t3widget/key_binding.h"
#include "t3widget/log.h"
#include "t3widget/main.h"
#include "t3widget/mouse.h"
#include "t3widget/signals.h"
#include "t3widget/string_view.h"
#include "t3widget/textbuffer.h"
#include "t3widget/textline.h"
#include "t3widget/util.h"
#include "t3widget/widget_api.h"
#include "t3widget/widgets/editwindow.h"
#include "t3widget/widgets/label.h"
#include "t3widget/widgets/listpane.h"
#include "t3widget/widgets/scrollbar.h"
#include "t3widget/widgets/widget.h"
#include "t3widget/wrapinfo.h"
#include "t3window/terminal.h"
#include "t3window/window.h"

/* FIXME: implement Ctrl-up and Ctrl-down for shifting the window contents without the cursor. */

namespace t3widget {

goto_dialog_t *edit_window_t::goto_dialog;
connection_t edit_window_t::goto_connection;
find_dialog_t *edit_window_t::global_find_dialog;
connection_t edit_window_t::global_find_dialog_connection;
std::shared_ptr<finder_t> edit_window_t::global_finder;
replace_buttons_dialog_t *edit_window_t::replace_buttons;
connection_t edit_window_t::replace_buttons_connection;
menu_panel_t *edit_window_t::right_click_menu;
connection_t edit_window_t::right_click_menu_connection;

connection_t edit_window_t::init_connected = connect_on_init(edit_window_t::init);

#define _T3_ACTION_FILE "t3widget/widgets/editwindow.actions.h"
#define _T3_ACTION_TYPE edit_window_t
#include "t3widget/key_binding_def.h"

const char *edit_window_t::ins_string[] = {"INS", "OVR"};
bool (text_buffer_t::*edit_window_t::proces_char[])(key_t) = {&text_buffer_t::insert_char,
                                                              &text_buffer_t::overwrite_char};

class T3_WIDGET_LOCAL edit_window_t::autocomplete_panel_t : public popup_t {
 private:
  list_pane_t *list_pane;

 public:
  autocomplete_panel_t(edit_window_t *parent);
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;

  void set_completions(string_list_base_t *completions);
  size_t get_selected_idx() const;
  void connect_activate(std::function<void()> func);
};

struct edit_window_t::implementation_t {
  t3window::window_t edit_window, /**< Window containing the text. */
      indicator_window; /**< Window holding the line, column, modified, etc. information line at
                           the bottom. */
  std::unique_ptr<scrollbar_t> scrollbar; /**< Scrollbar on the right of the text. */
  text_pos_t screen_pos = 0;              /**< Cached position of cursor in screen coordinates. */
  int tabsize = 8;                        /**< Width of a tab, in cells. */
  bool focus = false; /**< Boolean indicating whether this edit_window_t has the input focus. */
  bool tab_spaces = false; /**< Boolean indicating whether to use spaces for tab. */
  /** Associated find dialog.
      By default this is the shared dialog, but can be set to a different one. */
  find_dialog_t *find_dialog = nullptr;
  bool use_local_finder = false;
  std::shared_ptr<finder_t> finder;          /**< Object used for find actions in the text. */
  wrap_type_t wrap_type = wrap_type_t::NONE; /**< The wrap_type_t used for display. */
  wrap_info_t *wrap_info =
      nullptr; /**< Required information for wrapped display, or @c nullptr if not in use. */
  /** The top-left coordinate in the text.
          This is either a proper text_coordinate_t when wrapping is disabled, or
          a line and sub-line (pos @c member) coordinate when wrapping is enabled.
  */
  text_coordinate_t top_left;
  int ins_mode = 0;            /**< Current insert/overwrite mode. */
  text_pos_t last_set_pos = 0; /**< Last horiziontal position set by user action. */
  bool auto_indent =
      true; /**< Boolean indicating whether automatic indentation should be enabled. */
  /** Boolean indicating whether the current text is part of a paste operation.
      Set automatically as a response to bracketed paste. Disables auto-indent. */
  bool pasting_text = false;
  /** Boolean indicating whether home key should handle indentation specially. */
  bool indent_aware_home = true;
  bool show_tabs = false; /**< Boolean indicating whether to explicitly show tabs. */

  std::unique_ptr<autocompleter_t> autocompleter; /**< Object used for autocompletion. */
  std::unique_ptr<autocomplete_panel_t>
      autocomplete_panel; /**< Panel for showing autocomplete options. */

  text_pos_t repaint_min = 0,                               /**< First line to repaint. */
      repaint_max = std::numeric_limits<text_pos_t>::max(); /**< Last line to repaint. */
};

struct edit_window_t::behavior_parameters_t::implementation_t {
  text_coordinate_t top_left{0, 0};
  text_pos_t last_set_pos = 0;
  int tabsize = 8;
  int ins_mode = 0;
  wrap_type_t wrap_type = wrap_type_t::NONE;
  bool tab_spaces = false;
  bool auto_indent = true;
  bool indent_aware_home = true;
  bool show_tabs = false;
};

void edit_window_t::init(bool _init) {
  if (_init) {
    /* Construct these from t3widget::init, such that the locale is set correctly and
       gettext therefore returns the correctly localized strings. */
    goto_dialog = new goto_dialog_t();
    global_find_dialog = new find_dialog_t();
    replace_buttons = new replace_buttons_dialog_t();
    right_click_menu = new menu_panel_t("");
    right_click_menu->insert_item(nullptr, _("Cu_t"), "", ACTION_CUT);
    right_click_menu->insert_item(nullptr, _("_Copy"), "", ACTION_COPY);
    right_click_menu->insert_item(nullptr, _("_Paste"), "", ACTION_PASTE);
    right_click_menu->insert_item(nullptr, _("Paste _Selection"), "", ACTION_PASTE_SELECTION);
  } else {
    delete goto_dialog;
    goto_dialog = nullptr;
    delete global_find_dialog;
    global_find_dialog = nullptr;
    delete replace_buttons;
    replace_buttons = nullptr;
    delete right_click_menu;
    right_click_menu = nullptr;
  }
}

void edit_window_t::init_instance() {
  /* Register the unbacked window for mouse events, such that we can get focus
     if the bottom line is clicked. */
  init_unbacked_window(11, 11, true);

  impl->edit_window.alloc(&window, 10, 10, 0, 0, 0);
  impl->edit_window.show();

  impl->indicator_window.alloc(&window, 1, 10, 0, 0, 0);

  impl->indicator_window.set_anchor(
      &window, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  impl->indicator_window.show();

  info_window.alloc(&window, 1, 1, 0, 0, 1);

  info_window.set_anchor(&window, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
  info_window.show();

  impl->scrollbar.reset(new scrollbar_t(true));
  container_t::set_widget_parent(impl->scrollbar.get());
  impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  impl->scrollbar->set_size(10, None);
  impl->scrollbar->connect_clicked(bind_front(&edit_window_t::scrollbar_clicked, this));
  impl->scrollbar->connect_dragged(bind_front(&edit_window_t::scrollbar_dragged, this));

  impl->screen_pos = 0;
  impl->focus = false;

  impl->autocomplete_panel.reset(new autocomplete_panel_t(this));
  impl->autocomplete_panel->connect_activate([this] { autocomplete_activated(); });
}

edit_window_t::edit_window_t(text_buffer_t *_text, const view_parameters_t *params)
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()), text(nullptr) {
  init_instance();
  set_text(_text == nullptr ? new text_buffer_t() : _text, params);
}

edit_window_t::edit_window_t(text_buffer_t *_text, const behavior_parameters_t *params)
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()), text(nullptr) {
  init_instance();
  set_text(_text == nullptr ? new text_buffer_t() : _text, params);
}

edit_window_t::~edit_window_t() { delete impl->wrap_info; }

void edit_window_t::set_text(text_buffer_t *_text, const view_parameters_t *params) {
  if (text == _text) {
    return;
  }
  if (params != nullptr) {
    behavior_parameters_t new_params(*params);
    set_text(_text, &new_params);
  } else {
    set_text(_text, static_cast<behavior_parameters_t *>(nullptr));
  }
}

void edit_window_t::set_text(text_buffer_t *_text, const behavior_parameters_t *params) {
  if (text == _text) {
    return;
  }

  text = _text;
  if (params != nullptr) {
    params->apply_parameters(this);
  } else {
    if (impl->wrap_info != nullptr) {
      impl->wrap_info->set_text_buffer(text);
      impl->wrap_info->set_wrap_width(impl->edit_window.get_width() - 1);
    }
    impl->top_left.line = 0;
    impl->top_left.pos = 0;
    impl->last_set_pos = 0;
  }

  ensure_cursor_on_screen();
  draw_info_window();
  update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
}

bool edit_window_t::set_size(optint height, optint width) {
  bool result = true;
  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  if (width.value() != window.get_width() || height.value() > window.get_height()) {
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
  }

  result &= window.resize(height.value(), width.value());
  result &= impl->edit_window.resize(height.value() - 1, width.value() - 1);
  result &= impl->scrollbar->set_size(height.value() - 1, None);

  if (impl->wrap_type != wrap_type_t::NONE) {
    impl->top_left.pos =
        impl->wrap_info->calculate_line_pos(impl->top_left.line, 0, impl->top_left.pos);
    impl->wrap_info->set_wrap_width(width.value() - 1);
    impl->top_left.pos = impl->wrap_info->find_line(impl->top_left);
    impl->last_set_pos = impl->wrap_info->calculate_screen_pos();
  }
  ensure_cursor_on_screen();
  return result;
}

void edit_window_t::ensure_cursor_on_screen() {
  const text_coordinate_t cursor = text->get_cursor();
  text_pos_t width;

  if (cursor.pos == text->get_line_size(cursor.line)) {
    width = 1;
  } else {
    width = text->width_at_cursor();
  }

  if (impl->wrap_type == wrap_type_t::NONE) {
    impl->screen_pos = text->calculate_screen_pos(impl->tabsize);

    if (cursor.line < impl->top_left.line) {
      impl->top_left.line = cursor.line;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (cursor.line >= impl->top_left.line + impl->edit_window.get_height()) {
      impl->top_left.line = cursor.line - impl->edit_window.get_height() + 1;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (impl->screen_pos < impl->top_left.pos) {
      impl->top_left.pos = impl->screen_pos;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (impl->screen_pos + width > impl->top_left.pos + impl->edit_window.get_width()) {
      impl->top_left.pos = impl->screen_pos + width - impl->edit_window.get_width();
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }
  } else {
    text_coordinate_t bottom;
    text_pos_t sub_line = impl->wrap_info->find_line(cursor);
    impl->screen_pos = impl->wrap_info->calculate_screen_pos();

    if (cursor.line < impl->top_left.line ||
        (cursor.line == impl->top_left.line && sub_line < impl->top_left.pos)) {
      impl->top_left.line = cursor.line;
      impl->top_left.pos = sub_line;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    } else {
      bottom = impl->top_left;
      impl->wrap_info->add_lines(bottom, impl->edit_window.get_height() - 1);

      while (cursor.line > bottom.line) {
        impl->wrap_info->add_lines(impl->top_left,
                                   impl->wrap_info->get_line_count(bottom.line) - bottom.pos);
        bottom.line++;
        bottom.pos = 0;
        update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
      }

      if (cursor.line == bottom.line && sub_line > bottom.pos) {
        impl->wrap_info->add_lines(impl->top_left, sub_line - bottom.pos);
        update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
      }
    }
  }
}

void edit_window_t::repaint_screen() {
  text_coordinate_t current_start, current_end;
  text_line_t::paint_info_t info;
  int i;

  impl->edit_window.set_default_attrs(attributes.text);

  const text_coordinate_t cursor = text->get_cursor();
  update_repaint_lines(cursor.line);

  current_start = text->get_selection_start();
  current_end = text->get_selection_end();

  if (current_end < current_start) {
    current_start = current_end;
    current_end = text->get_selection_start();
  }

  info.size = impl->edit_window.get_width();
  info.tabsize = impl->tabsize;
  info.normal_attr = 0;
  info.selected_attr = attributes.text_selected;
  info.flags = impl->show_tabs ? text_line_t::SHOW_TABS : 0;

  if (impl->wrap_type == wrap_type_t::NONE) {
    info.leftcol = impl->top_left.pos;
    info.start = 0;
    info.max = std::numeric_limits<text_pos_t>::max();

    for (i = 0; i < impl->edit_window.get_height() && (i + impl->top_left.line) < text->size();
         i++) {
      if (impl->top_left.line + i < impl->repaint_min ||
          impl->top_left.line + i > impl->repaint_max) {
        continue;
      }

      info.selection_start = impl->top_left.line + i == current_start.line ? current_start.pos : -1;
      if (impl->top_left.line + i >= current_start.line) {
        if (impl->top_left.line + i < current_end.line) {
          info.selection_end = std::numeric_limits<text_pos_t>::max();
        } else if (impl->top_left.line + i == current_end.line) {
          info.selection_end = current_end.pos;
        } else {
          info.selection_end = -1;
        }
      } else {
        info.selection_end = -1;
      }

      info.cursor = impl->top_left.line + i == cursor.line ? cursor.pos : -1;
      impl->edit_window.set_paint(i, 0);
      impl->edit_window.clrtoeol();
      text->paint_line(&impl->edit_window, impl->top_left.line + i, info);
    }
  } else {
    text_coordinate_t end_coord = impl->wrap_info->get_end();
    text_coordinate_t draw_line = impl->top_left;
    info.leftcol = 0;

    for (i = 0; i < impl->edit_window.get_height(); i++, impl->wrap_info->add_lines(draw_line, 1)) {
      if (draw_line.line < impl->repaint_min || draw_line.line > impl->repaint_max) {
        continue;
      }
      info.selection_start = draw_line.line == current_start.line ? current_start.pos : -1;
      if (draw_line.line >= current_start.line) {
        if (draw_line.line < current_end.line) {
          info.selection_end = std::numeric_limits<text_pos_t>::max();
        } else if (draw_line.line == current_end.line) {
          info.selection_end = current_end.pos;
        } else {
          info.selection_end = -1;
        }
      } else {
        info.selection_end = -1;
      }

      info.cursor = draw_line.line == cursor.line ? cursor.pos : -1;
      impl->edit_window.set_paint(i, 0);
      impl->edit_window.clrtoeol();
      impl->wrap_info->paint_line(&impl->edit_window, draw_line, info);

      if (draw_line.line == end_coord.line && draw_line.pos == end_coord.pos) {
        /* Increase i, to make sure this line is not erased. */
        i++;
        break;
      }
    }
  }
  /* Clear the bottom part of the window (if applicable). */
  impl->edit_window.set_paint(i, 0);
  impl->edit_window.clrtobot();

  impl->repaint_min = cursor.line;
  impl->repaint_max = cursor.line;
}

void edit_window_t::inc_x() {
  const text_coordinate_t cursor = text->get_cursor();
  if (cursor.pos == text->get_line_size(cursor.line)) {
    if (cursor.line >= text->size() - 1) {
      return;
    }

    text->set_cursor({cursor.line + 1, 0});
  } else {
    text->adjust_position(1);
  }
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::next_word() {
  text->goto_next_word();
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::dec_x() {
  const text_coordinate_t cursor = text->get_cursor();
  if (cursor.pos == 0) {
    if (cursor.line == 0) {
      return;
    }

    text->set_cursor({cursor.line - 1, text->get_line_size(cursor.line - 1)});
  } else {
    text->adjust_position(-1);
  }
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::previous_word() {
  text->goto_previous_word();
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::inc_y() {
  const text_coordinate_t cursor = text->get_cursor();
  if (impl->wrap_type == wrap_type_t::NONE) {
    if (cursor.line + 1 < text->size()) {
      text->set_cursor({cursor.line + 1, text->calculate_line_pos(
                                             cursor.line + 1, impl->last_set_pos, impl->tabsize)});
      ensure_cursor_on_screen();
    } else {
      text->set_cursor_pos(text->get_line_size(cursor.line));
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    }
  } else {
    text_pos_t new_sub_line = impl->wrap_info->find_line(cursor) + 1;
    if (impl->wrap_info->get_line_count(cursor.line) == new_sub_line) {
      if (cursor.line + 1 < text->size()) {
        text->set_cursor({cursor.line + 1, impl->wrap_info->calculate_line_pos(
                                               cursor.line + 1, impl->last_set_pos, 0)});
        ensure_cursor_on_screen();
      } else {
        text->set_cursor_pos(text->get_line_size(cursor.line));
        ensure_cursor_on_screen();
        impl->last_set_pos = impl->screen_pos;
      }
    } else {
      text->set_cursor_pos(
          impl->wrap_info->calculate_line_pos(cursor.line, impl->last_set_pos, new_sub_line));
      ensure_cursor_on_screen();
    }
  }
}

void edit_window_t::dec_y() {
  const text_coordinate_t cursor = text->get_cursor();
  if (impl->wrap_type == wrap_type_t::NONE) {
    if (cursor.line > 0) {
      text->set_cursor({cursor.line - 1, text->calculate_line_pos(
                                             cursor.line - 1, impl->last_set_pos, impl->tabsize)});
      ensure_cursor_on_screen();
    } else {
      impl->last_set_pos = 0;
      text->set_cursor_pos(0);
      ensure_cursor_on_screen();
    }
  } else {
    text_pos_t sub_line = impl->wrap_info->find_line(cursor);
    if (sub_line > 0) {
      text->set_cursor_pos(
          impl->wrap_info->calculate_line_pos(cursor.line, impl->last_set_pos, sub_line - 1));
      ensure_cursor_on_screen();
    } else if (cursor.line > 0) {
      text->set_cursor(
          {cursor.line - 1, impl->wrap_info->calculate_line_pos(
                                cursor.line - 1, impl->last_set_pos,
                                impl->wrap_info->get_line_count(cursor.line - 1) - 1)});
      ensure_cursor_on_screen();
    } else {
      text->set_cursor_pos(0);
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    }
  }
}

void edit_window_t::pgdn() {
  text_coordinate_t cursor = text->get_cursor();
  bool need_adjust = true;

  if (impl->wrap_type == wrap_type_t::NONE) {
    if (cursor.line + impl->edit_window.get_height() - 1 < text->size()) {
      cursor.line += impl->edit_window.get_height() - 1;
    } else {
      cursor.line = text->size() - 1;
      cursor.pos = text->get_line_size(cursor.line);
      need_adjust = false;
    }

    /* If the end of the text is already on the screen, don't change the top line. */
    if (impl->top_left.line + impl->edit_window.get_height() < text->size()) {
      impl->top_left.line += impl->edit_window.get_height() - 1;
      if (impl->top_left.line + impl->edit_window.get_height() > text->size()) {
        impl->top_left.line = text->size() - impl->edit_window.get_height();
      }
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (need_adjust) {
      cursor.pos = text->calculate_line_pos(cursor.line, impl->last_set_pos, impl->tabsize);
    }
  } else {
    text_coordinate_t new_top_left = impl->top_left;
    text_coordinate_t new_cursor(cursor.line, impl->wrap_info->find_line(cursor));

    if (impl->wrap_info->add_lines(new_cursor, impl->edit_window.get_height() - 1)) {
      cursor.line = new_cursor.line;
      cursor.pos = text->get_line_size(cursor.line);
      need_adjust = false;
    } else {
      cursor.line = new_cursor.line;
    }

    /* If the end of the text is already on the screen, don't change the top line. */
    if (!impl->wrap_info->add_lines(new_top_left, impl->edit_window.get_height())) {
      impl->top_left = new_top_left;
      impl->wrap_info->sub_lines(impl->top_left, 1);
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (need_adjust) {
      cursor.pos =
          impl->wrap_info->calculate_line_pos(cursor.line, impl->last_set_pos, new_cursor.pos);
    }
  }
  text->set_cursor(cursor);
  ensure_cursor_on_screen();

  if (!need_adjust) {
    impl->last_set_pos = impl->screen_pos;
  }
}

void edit_window_t::pgup() {
  text_coordinate_t cursor = text->get_cursor();
  bool need_adjust = true;

  if (impl->wrap_type == wrap_type_t::NONE) {
    if (impl->top_left.line < impl->edit_window.get_height() - 1) {
      if (impl->top_left.line != 0) {
        impl->top_left.line = 0;
        update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
      }

      if (cursor.line < impl->edit_window.get_height() - 1) {
        cursor.line = 0;
        impl->last_set_pos = cursor.pos = 0;
        need_adjust = false;
      } else {
        cursor.line -= impl->edit_window.get_height() - 1;
      }
    } else {
      cursor.line -= impl->edit_window.get_height() - 1;
      impl->top_left.line -= impl->edit_window.get_height() - 1;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }

    if (need_adjust) {
      cursor.pos = text->calculate_line_pos(cursor.line, impl->last_set_pos, impl->tabsize);
    }
  } else {
    text_coordinate_t new_cursor(cursor.line, impl->wrap_info->find_line(cursor));

    if (impl->wrap_info->sub_lines(new_cursor, impl->edit_window.get_height() - 1)) {
      cursor.line = 0;
      cursor.pos = 0;
      impl->last_set_pos = 0;
      need_adjust = false;
    } else {
      cursor.line = new_cursor.line;
    }

    impl->wrap_info->sub_lines(impl->top_left, impl->edit_window.get_height() - 1);
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());

    if (need_adjust) {
      cursor.pos =
          impl->wrap_info->calculate_line_pos(cursor.line, impl->last_set_pos, new_cursor.pos);
    }
  }
  text->set_cursor(cursor);

  ensure_cursor_on_screen();
}

void edit_window_t::home_key() {
  const text_coordinate_t cursor = text->get_cursor();
  text_pos_t pos;

  if (!impl->indent_aware_home) {
    text->set_cursor_pos(impl->wrap_type == wrap_type_t::NONE
                             ? 0
                             : impl->wrap_info->calculate_line_pos(
                                   cursor.line, 0, impl->wrap_info->find_line(cursor)));
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
    return;
  }

  if (impl->wrap_type != wrap_type_t::NONE) {
    pos = impl->wrap_info->calculate_line_pos(cursor.line, 0, impl->wrap_info->find_line(cursor));
    if (pos != cursor.pos) {
      text->set_cursor_pos(pos);
      impl->screen_pos = impl->last_set_pos = 0;
      return;
    }
  }
  const text_line_t &line = text->get_line_data(cursor.line);
  for (pos = 0; pos < line.size() && line.is_space(pos); pos = line.adjust_position(pos, 1)) {
  }

  text->set_cursor_pos(cursor.pos != pos ? pos : 0);
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::end_key() {
  const text_coordinate_t cursor = text->get_cursor();
  if (impl->wrap_type == wrap_type_t::NONE) {
    text->set_cursor_pos(text->get_line_size(cursor.line));
  } else {
    text_pos_t sub_line = impl->wrap_info->find_line(cursor);
    if (sub_line + 1 < impl->wrap_info->get_line_count(cursor.line)) {
      text_pos_t before_pos = cursor.pos;
      text->set_cursor_pos(impl->wrap_info->calculate_line_pos(cursor.line, 0, sub_line + 1));
      text->adjust_position(-1);
      const text_coordinate_t new_cursor = text->get_cursor();
      if (before_pos == new_cursor.pos) {
        text->set_cursor({new_cursor.line, text->get_line_size(new_cursor.line)});
      }
    } else {
      text->set_cursor_pos(text->get_line_size(cursor.line));
    }
  }
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::reset_selection() {
  update_repaint_lines(text->get_selection_start().line, text->get_selection_end().line);
  text->set_selection_mode(selection_mode_t::NONE);
}

bool edit_window_t::set_selection_mode(key_t key) {
  selection_mode_t selection_mode = text->get_selection_mode();
  switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
    case EKEY_END:
    case EKEY_HOME:
    case EKEY_PGUP:
    case EKEY_PGDN:
    case EKEY_LEFT:
    case EKEY_RIGHT:
    case EKEY_UP:
    case EKEY_DOWN:
      if ((selection_mode == selection_mode_t::SHIFT || selection_mode == selection_mode_t::ALL) &&
          !(key & EKEY_SHIFT)) {
        bool done = false;
        if (key == EKEY_LEFT) {
          if (text->get_selection_start() < text->get_selection_end()) {
            text->set_cursor(text->get_selection_start());
          } else {
            text->set_cursor(text->get_selection_end());
          }
          done = true;
        } else if (key == EKEY_RIGHT) {
          if (text->get_selection_start() < text->get_selection_end()) {
            text->set_cursor(text->get_selection_end());
          } else {
            text->set_cursor(text->get_selection_start());
          }
          done = true;
        }
        reset_selection();
        return done;
      } else if ((key & EKEY_SHIFT) && selection_mode != selection_mode_t::MARK) {
        text->set_selection_mode(selection_mode_t::SHIFT);
      }
      break;
    default:
      break;
  }
  return false;
}

void edit_window_t::delete_selection() {
  text_coordinate_t current_start, current_end;

  current_start = text->get_selection_start();
  current_end = text->get_selection_end();
  text->delete_block(current_start, current_end);

  update_repaint_lines(
      current_start.line < current_end.line ? current_start.line : current_end.line,
      std::numeric_limits<text_pos_t>::max());
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
  reset_selection();
}

void edit_window_t::find_activated(std::shared_ptr<finder_t> _finder, find_action_t action) {
  find_result_t result;

  if (_finder) {
    if (impl->use_local_finder) {
      impl->finder = _finder;
    } else {
      global_finder = _finder;
    }
  }
  finder_t *local_finder = impl->use_local_finder ? impl->finder.get() : global_finder.get();

  switch (action) {
    case find_action_t::FIND:
      result.start = text->get_cursor();

      if (!text->find(local_finder, &result)) {
        goto not_found;
      }

      text->set_selection_from_find(result);
      update_repaint_lines(result.start.line, result.end.line);
      ensure_cursor_on_screen();
      if (local_finder->get_flags() & find_flags_t::REPLACEMENT_VALID) {
        replace_buttons_connection.disconnect();
        replace_buttons_connection = replace_buttons->connect_activate(
            bind_front(&edit_window_t::find_activated, this, nullptr));
        replace_buttons->center_over(center_window);
        replace_buttons->show();
      }
      break;
    case find_action_t::REPLACE:
      result.start = text->get_selection_start();
      result.end = text->get_selection_end();
      text->replace(*local_finder, result);
      update_repaint_lines(
          result.start.line < result.end.line ? result.start.line : result.end.line,
          std::numeric_limits<text_pos_t>::max());
      /* FALLTHROUGH */
      if (false) {
        case find_action_t::SKIP:
          /* This part is skipped when the action is replace */
          result.start = text->get_selection_start();
      }
      if (!text->find(local_finder, &result)) {
        ensure_cursor_on_screen();
        goto not_found;
      }

      text->set_selection_from_find(result);
      update_repaint_lines(result.start.line, result.end.line);
      ensure_cursor_on_screen();
      replace_buttons->reshow(action);
      break;
    case find_action_t::REPLACE_ALL: {
      int replacements;
      text_coordinate_t start(0, -1);
      text_coordinate_t eof(std::numeric_limits<text_pos_t>::max(),
                            std::numeric_limits<text_pos_t>::max());

      for (replacements = 0; text->find_limited(local_finder, start, eof, &result);
           replacements++) {
        lprintf("Find result: %ld %ld\n", result.start.line, result.start.pos);
        if (replacements == 0) {
          text->start_undo_block();
        }
        text->replace(*local_finder, result);
        start = text->get_cursor();
        lprintf("New start: %ld %ld\n", start.line, start.pos);
      }

      if (replacements == 0) {
        goto not_found;
      }

      text->end_undo_block();
      reset_selection();
      ensure_cursor_on_screen();
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
      break;
    }
    case find_action_t::REPLACE_IN_SELECTION: {
      if (text->selection_empty()) {
        return;
      }

      text_coordinate_t start(text->get_selection_start());
      text_coordinate_t end(text->get_selection_end());
      text_coordinate_t saved_start;
      int replacements;
      text_pos_t end_line_length;
      bool reverse_selection = false;

      if (end < start) {
        start = text->get_selection_end();
        end = text->get_selection_start();
        reverse_selection = true;
      }
      end_line_length = text->get_line_size(end.line);
      saved_start = start;

      for (replacements = 0; text->find_limited(local_finder, start, end, &result);
           replacements++) {
        if (replacements == 0) {
          text->start_undo_block();
        }
        text->replace(*local_finder, result);
        start = text->get_cursor();
        end.pos -= end_line_length - text->get_line_size(end.line);
        end_line_length = text->get_line_size(end.line);
      }

      if (replacements == 0) {
        goto not_found;
      }

      text->end_undo_block();

      text->set_selection_mode(selection_mode_t::NONE);
      if (reverse_selection) {
        text->set_cursor(end);
        text->set_selection_mode(selection_mode_t::SHIFT);
        text->set_cursor(saved_start);
        text->set_selection_end();
      } else {
        text->set_cursor(saved_start);
        text->set_selection_mode(selection_mode_t::SHIFT);
        text->set_cursor(end);
        text->set_selection_end();
      }

      ensure_cursor_on_screen();
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
      break;
    }
    default:
      break;
  }
  return;

not_found:
  // FIXME: show search string
  message_dialog->set_message("Search string not found");
  message_dialog->center_over(center_window);
  message_dialog->show();
}

// FIXME: make every action into a separate function for readability
bool edit_window_t::process_key(key_t key) {
  if (set_selection_mode(key)) {
    return true;
  }

  switch (key) {
    case EKEY_RIGHT | EKEY_SHIFT:
    case EKEY_RIGHT:
      inc_x();
      break;
    case EKEY_RIGHT | EKEY_CTRL:
    case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
      next_word();
      break;
    case EKEY_LEFT | EKEY_SHIFT:
    case EKEY_LEFT:
      dec_x();
      break;
    case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
    case EKEY_LEFT | EKEY_CTRL:
      previous_word();
      break;
    case EKEY_DOWN | EKEY_SHIFT:
    case EKEY_DOWN:
      inc_y();
      break;
    case EKEY_UP | EKEY_SHIFT:
    case EKEY_UP:
      dec_y();
      break;
    case EKEY_PGUP | EKEY_SHIFT:
    case EKEY_PGUP:
      pgup();
      break;
    case EKEY_PGDN | EKEY_SHIFT:
    case EKEY_PGDN:
      pgdn();
      break;
    case EKEY_HOME | EKEY_SHIFT:
    case EKEY_HOME:
      home_key();
      break;
    case EKEY_HOME | EKEY_CTRL | EKEY_SHIFT:
    case EKEY_HOME | EKEY_CTRL:
      impl->screen_pos = impl->last_set_pos;
      text->set_cursor({0, 0});
      if ((impl->wrap_type == wrap_type_t::NONE && impl->top_left.pos != 0) ||
          impl->top_left.line != 0) {
        ensure_cursor_on_screen();
      }
      break;
    case EKEY_END | EKEY_SHIFT:
    case EKEY_END:
      end_key();
      break;
    case EKEY_END | EKEY_CTRL | EKEY_SHIFT:
    case EKEY_END | EKEY_CTRL: {
      text_pos_t cursor_line = text->size() - 1;
      text->set_cursor({cursor_line, text->get_line_size(cursor_line)});
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
      break;
    }
    case EKEY_INS:
      impl->ins_mode ^= 1;
      break;

    case EKEY_DEL:
      if (text->get_selection_mode() == selection_mode_t::NONE) {
        const text_coordinate_t cursor = text->get_cursor();
        if (cursor.pos != text->get_line_size(cursor.line)) {
          text->delete_char();

          if (impl->wrap_type != wrap_type_t::NONE) {
            ensure_cursor_on_screen();
          }

          update_repaint_lines(cursor.line);
        } else if (cursor.line + 1 < text->size()) {
          text->merge(false);

          if (impl->wrap_type != wrap_type_t::NONE) {
            ensure_cursor_on_screen();
          }

          update_repaint_lines(cursor.line, std::numeric_limits<text_pos_t>::max());
        }
      } else {
        delete_selection();
      }
      break;

    case EKEY_NL: {
      int indent, tabs;
      std::string space;

      if (text->get_selection_mode() != selection_mode_t::NONE) {
        delete_selection();
      }

      const text_coordinate_t cursor = text->get_cursor();
      update_repaint_lines(cursor.line, std::numeric_limits<text_pos_t>::max());
      if (impl->auto_indent && !impl->pasting_text) {
        const std::string &current_line = text->get_line_data(cursor.line).get_data();
        text_pos_t i;
        for (i = 0, indent = 0, tabs = 0; i < cursor.pos; i++) {
          if (current_line[i] == '\t') {
            indent = 0;
            tabs++;
          } else if (current_line[i] == ' ') {
            indent++;
            if (indent == impl->tabsize) {
              indent = 0;
              tabs++;
            }
          } else {
            break;
          }
        }
        if (impl->tab_spaces) {
          space.append(impl->tabsize * tabs, ' ');
        } else {
          space.append(tabs, '\t');
        }

        if (indent != 0) {
          space.append(indent, ' ');
        }

        text->break_line(space);
      } else {
        text->break_line();
      }
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
      break;
    }
    case EKEY_BS | EKEY_CTRL:
      if (text->get_selection_mode() == selection_mode_t::NONE) {
        const text_coordinate_t cursor = text->get_cursor();
        if (cursor.pos <= text->get_line_size(cursor.line)) {
          if (cursor.pos != 0) {
            text->backspace_word();
            update_repaint_lines(cursor.line);
          } else if (cursor.line != 0) {
            text->merge(true);
            update_repaint_lines(cursor.line);
          }
        } else {
          ASSERT(false);
        }
        ensure_cursor_on_screen();
        impl->last_set_pos = impl->screen_pos;
      } else {
        delete_selection();
      }
      if (impl->autocomplete_panel->is_shown()) {
        activate_autocomplete(false);
      }
      break;
    case EKEY_BS:
      if (text->get_selection_mode() == selection_mode_t::NONE) {
        const text_coordinate_t cursor = text->get_cursor();
        if (cursor.pos <= text->get_line_size(cursor.line)) {
          if (cursor.pos != 0) {
            text->backspace_char();
            update_repaint_lines(cursor.line);
          } else if (cursor.line != 0) {
            text->merge(true);
            update_repaint_lines(cursor.line);
          }
        } else {
          ASSERT(false);
        }
        ensure_cursor_on_screen();
        impl->last_set_pos = impl->screen_pos;
      } else {
        delete_selection();
      }
      if (impl->autocomplete_panel->is_shown()) {
        activate_autocomplete(false);
      }
      break;

    case EKEY_ESC:
      if (text->get_selection_mode() == selection_mode_t::MARK) {
        reset_selection();
      }
      break;

    case EKEY_SHIFT | '\t':
      unindent_selection();
      break;
    case '\t':
      indent_selection();
      break;
    case EKEY_PASTE_START:
      if (!impl->pasting_text) {
        impl->pasting_text = true;
        text->start_undo_block();
      }
      break;
    case EKEY_PASTE_END:
      if (impl->pasting_text) {
        impl->pasting_text = false;
        text->end_undo_block();
      }
      break;
    default: {
      int local_insmode;

      optional<Action> action = key_bindings.find_action(key);
      if (action.is_valid()) {
        switch (action.value()) {
          case ACTION_COPY:
            cut_copy(false);
            return true;
          case ACTION_CUT:
            cut_copy(true);
            return true;
          case ACTION_PASTE:
            paste(true);
            return true;
          case ACTION_PASTE_SELECTION:
            paste(false);
            return true;
          case ACTION_REDO:
            redo();
            return true;
          case ACTION_UNDO:
            undo();
            return true;
          case ACTION_SELECT_ALL:
            select_all();
            return true;
          case ACTION_GOTO_LINE:
            goto_line();
            return true;
          case ACTION_AUTOCOMPLETE: /* CTRL-space and others */
            activate_autocomplete(true);
            return true;
          case ACTION_DELETE_LINE:
            delete_line();
            return true;
          case ACTION_MARK_SELECTION:
            mark_selection();
            return true;
          case ACTION_FIND:
            find_replace(false);
            return true;
          case ACTION_REPLACE:
            find_replace(true);
            return true;
          case ACTION_FIND_NEXT:
            find_next(false);
            return true;
          case ACTION_FIND_PREVIOUS:
            find_next(true);
            return true;
          case ACTION_INSERT_SPECIAL:
            insert_special();
            return true;

          default:
            break;
        }
      }

      if (key < 32) {
        return false;
      }

      key &= ~EKEY_PROTECT;
      if (key == 10) {
        return false;
      }

      if (key >= EKEY_FIRST_SPECIAL) {
        return false;
      }

      local_insmode = impl->ins_mode;
      if (text->get_selection_mode() != selection_mode_t::NONE) {
        delete_selection();
        local_insmode = 0;
      }

      (text->*proces_char[local_insmode])(key);
      ensure_cursor_on_screen();
      update_repaint_lines(text->get_cursor().line);
      impl->last_set_pos = impl->screen_pos;
      if (impl->autocomplete_panel->is_shown()) {
        activate_autocomplete(false);
      }
      break;
    }
  }
  return true;
}

void edit_window_t::update_contents() {
  text_coordinate_t logical_cursor_pos;
  char info[30];
  int name_width;
  selection_mode_t selection_mode;

  /* TODO: see if we can optimize this somewhat by not redrawing the whole thing
     on every key.

     - cursor-only movements mostly don't require entire redraws [well, that depends:
        for matching brace/parenthesis it may require more than a single line update]
  */
  if (!reset_redraw()) {
    return;
  }

  selection_mode = text->get_selection_mode();
  if (selection_mode != selection_mode_t::NONE && selection_mode != selection_mode_t::ALL) {
    text->set_selection_end();

    if (selection_mode == selection_mode_t::SHIFT) {
      if (text->selection_empty()) {
        reset_selection();
      }
    }
  }

  repaint_screen();

  impl->indicator_window.set_default_attrs(attributes.menubar);
  impl->indicator_window.set_paint(0, 0);
  impl->indicator_window.addchrep(' ', 0, impl->indicator_window.get_width());

  if (impl->wrap_type == wrap_type_t::NONE) {
    impl->scrollbar->set_parameters(
        std::max(text->size(), impl->top_left.line + impl->edit_window.get_height()),
        impl->top_left.line, impl->edit_window.get_height());
  } else {
    text_pos_t count = 0;
    for (text_pos_t i = 0; i < impl->top_left.line; i++) {
      count += impl->wrap_info->get_line_count(i);
    }
    count += impl->top_left.pos;

    impl->scrollbar->set_parameters(
        std::max(impl->wrap_info->wrapped_size(), count + impl->edit_window.get_height()), count,
        impl->edit_window.get_height());
  }
  impl->scrollbar->update_contents();

  logical_cursor_pos = text->get_cursor();
  logical_cursor_pos.pos = text->calculate_screen_pos(impl->tabsize);

  snprintf(info, 29, "L: %-4td C: %-4td %c %s", logical_cursor_pos.line + 1,
           logical_cursor_pos.pos + 1, text->is_modified() ? '*' : ' ', ins_string[impl->ins_mode]);
  size_t info_width = t3_term_strcwidth(info);
  impl->indicator_window.resize(1, info_width + 3);
  name_width = window.get_width() - impl->indicator_window.get_width();

  if (info_window.get_width() != name_width && name_width > 0) {
    info_window.resize(1, name_width);
    draw_info_window();
  }

  impl->indicator_window.set_paint(0, impl->indicator_window.get_width() - info_width - 1);
  impl->indicator_window.addstr(info, 0);
}

void edit_window_t::set_focus(focus_t _focus) {
  if (_focus != impl->focus) {
    impl->focus = _focus;
    impl->autocomplete_panel->hide();
    update_repaint_lines(text->get_cursor().line);
  }
}

void edit_window_t::undo() {
  if (text->apply_undo() == 0) {
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
  }
}

void edit_window_t::redo() {
  if (text->apply_redo() == 0) {
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
  }
}

void edit_window_t::cut_copy(bool cut) {
  if (text->get_selection_mode() != selection_mode_t::NONE) {
    if (text->selection_empty()) {
      reset_selection();
      return;
    }

    set_clipboard(text->convert_block(text->get_selection_start(), text->get_selection_end()));

    if (cut) {
      delete_selection();
    } else if (text->get_selection_mode() == selection_mode_t::MARK) {
      text->set_selection_mode(selection_mode_t::SHIFT);
    }
  }
}

void edit_window_t::paste() { paste(true); }

void edit_window_t::paste_selection() { paste(false); }

void edit_window_t::paste(bool clipboard) {
  {
    ensure_clipboard_lock_t lock;
    std::shared_ptr<std::string> copy_buffer = clipboard ? get_clipboard() : get_primary();
    if (copy_buffer != nullptr) {
      if (text->get_selection_mode() == selection_mode_t::NONE) {
        update_repaint_lines(text->get_cursor().line, std::numeric_limits<text_pos_t>::max());
        text->insert_block(*copy_buffer);
      } else {
        text_coordinate_t current_start;
        text_coordinate_t current_end;
        current_start = text->get_selection_start();
        current_end = text->get_selection_end();
        update_repaint_lines(
            current_start.line < current_end.line ? current_start.line : current_end.line,
            std::numeric_limits<text_pos_t>::max());
        text->replace_block(current_start, current_end, *copy_buffer);
        reset_selection();
      }
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    }
  }
}

void edit_window_t::right_click_menu_activated(int action) {
  lprintf("right click menu activated: %d\n", action);
  switch (action) {
    case ACTION_CUT:
      cut_copy(true);
      break;
    case ACTION_COPY:
      cut_copy(false);
      break;
    case ACTION_PASTE:
      paste(true);
      break;
    case ACTION_PASTE_SELECTION:
      paste(false);
      break;
    default:
      break;
  }
}

void edit_window_t::select_all() {
  text->set_selection_mode(selection_mode_t::ALL);
  update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
}

void edit_window_t::insert_special() {
  insert_char_dialog->center_over(center_window);
  insert_char_dialog->reset();
  insert_char_dialog->show();
}

void edit_window_t::indent_selection() {
  if (text->get_selection_mode() != selection_mode_t::NONE &&
      text->get_selection_start().line != text->get_selection_end().line) {
    text->indent_selection(impl->tabsize, impl->tab_spaces);
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
    update_repaint_lines(text->get_selection_start().line, text->get_selection_end().line);
  } else {
    std::string space;
    if (text->get_selection_mode() != selection_mode_t::NONE) {
      delete_selection();
    }

    if (impl->tab_spaces) {
      space.append(impl->tabsize - (impl->screen_pos % impl->tabsize), ' ');
    } else {
      space.append(1, '\t');
    }
    text->insert_block(space);
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
  }
}

void edit_window_t::unindent_selection() {
  if (text->get_selection_mode() != selection_mode_t::NONE &&
      text->get_selection_start().line != text->get_selection_end().line) {
    text->unindent_selection(impl->tabsize);
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
    update_repaint_lines(text->get_selection_start().line, text->get_selection_end().line);
  } else {
    text->unindent_line(impl->tabsize);
    ensure_cursor_on_screen();
    impl->last_set_pos = impl->screen_pos;
    text_pos_t cursor_line = text->get_cursor().line;
    update_repaint_lines(cursor_line);
  }
}

void edit_window_t::goto_line() {
  goto_connection.disconnect();
  goto_connection = goto_dialog->connect_activate([this](text_pos_t line) { goto_line(line); });
  goto_dialog->center_over(center_window);
  goto_dialog->reset();
  goto_dialog->show();
}

void edit_window_t::goto_line(text_pos_t line) {
  if (line < 1) {
    return;
  }

  reset_selection();
  text_pos_t cursor_line = (line > text->size() ? text->size() : line) - 1;
  text->set_cursor(
      {cursor_line, text->calculate_line_pos(cursor_line, impl->screen_pos, impl->tabsize)});
  ensure_cursor_on_screen();
  impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::find_replace(bool replace) {
  find_dialog_t *dialog;
  if (impl->find_dialog == nullptr) {
    global_find_dialog_connection.disconnect();
    global_find_dialog_connection =
        global_find_dialog->connect_activate(bind_front(&edit_window_t::find_activated, this));
    dialog = global_find_dialog;
  } else {
    dialog = impl->find_dialog;
  }
  dialog->center_over(center_window);
  dialog->set_replace(replace);

  if (!text->selection_empty() &&
      text->get_selection_start().line == text->get_selection_end().line) {
    std::unique_ptr<std::string> selected_text(
        text->convert_block(text->get_selection_start(), text->get_selection_end()));
    dialog->set_text(*selected_text);
  }
  dialog->show();
}

void edit_window_t::find_next(bool backward) {
  find_result_t result;
  if (text->get_selection_mode() == selection_mode_t::NONE) {
    result.start = text->get_cursor();

  } else {
    if (text->get_selection_start() < text->get_selection_end()) {
      result.start = text->get_selection_start();
    } else {
      result.start = text->get_selection_end();
    }
  }

  finder_t *local_finder = impl->use_local_finder ? impl->finder.get() : global_finder.get();
  if (local_finder == nullptr) {
    message_dialog->set_message("No previous search");
    message_dialog->center_over(center_window);
    message_dialog->show();
  } else {
    if (!text->find(local_finder, &result, backward)) {
      // FIXME: show search string
      message_dialog->set_message("Search string not found");
      message_dialog->center_over(center_window);
      message_dialog->show();
    } else {
      text->set_selection_from_find(result);
      ensure_cursor_on_screen();
    }
  }
}

text_buffer_t *edit_window_t::get_text() const { return text; }

void edit_window_t::set_find_dialog(find_dialog_t *_find_dialog) {
  impl->find_dialog = _find_dialog;
}

void edit_window_t::set_use_local_finder(bool _use_local_finder) {
  impl->use_local_finder = _use_local_finder;
}

void edit_window_t::force_redraw() {
  update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
  draw_info_window();
  ensure_cursor_on_screen();
}

void edit_window_t::set_tabsize(int _tabsize) {
  if (_tabsize == impl->tabsize) {
    return;
  }
  impl->tabsize = _tabsize;
  if (impl->wrap_info != nullptr) {
    impl->wrap_info->set_tabsize(impl->tabsize);
  }
  force_redraw();
}

void edit_window_t::bad_draw_recheck() { widget_t::force_redraw(); }

void edit_window_t::set_child_focus(window_component_t *target) {
  (void)target;
  set_focus(window_component_t::FOCUS_SET);
}

bool edit_window_t::is_child(const window_component_t *widget) const {
  return widget == impl->scrollbar.get();
}

bool edit_window_t::process_mouse_event(mouse_event_t event) {
  if (event.window == impl->edit_window) {
    if (event.button_state & EMOUSE_TRIPLE_CLICKED_LEFT) {
      text->set_cursor_pos(0);
      text->set_selection_mode(selection_mode_t::SHIFT);
      text->set_cursor_pos(text->get_line_size(text->get_cursor().line));
      text->set_selection_end(true);
    } else if (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) {
      text->goto_previous_word_boundary();
      text->set_selection_mode(selection_mode_t::SHIFT);
      text->goto_next_word_boundary();
      text->set_selection_end(true);
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    } else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT) &&
               event.previous_button_state == 0) {
      if ((event.modifier_state & EMOUSE_SHIFT) == 0) {
        reset_selection();
      } else if (text->get_selection_mode() == selection_mode_t::NONE) {
        text->set_selection_mode(selection_mode_t::SHIFT);
      }
      text->set_cursor(xy_to_text_coordinate(event.x, event.y));
      if ((event.modifier_state & EMOUSE_SHIFT) != 0) {
        text->set_selection_end();
      }
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    } else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_MIDDLE)) {
      reset_selection();
      text->set_cursor(xy_to_text_coordinate(event.x, event.y));
      {
        ensure_clipboard_lock_t lock;
        std::shared_ptr<std::string> primary = get_primary();
        if (primary != nullptr) text->insert_block(*primary);
      }
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    } else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_RIGHT)) {
      right_click_menu->set_position(event.y + window.get_abs_y(), event.x + window.get_abs_x());
      right_click_menu->connect_closed([] { right_click_menu_connection.disconnect(); });
      right_click_menu_connection = right_click_menu->connect_activate(
          [this](int action) { right_click_menu_activated(action); });
      right_click_menu->show();
    } else if (event.type == EMOUSE_BUTTON_PRESS &&
               (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
      scroll(event.button_state & EMOUSE_SCROLL_UP ? -3 : 3);
    } else if ((event.type == EMOUSE_MOTION && (event.button_state & EMOUSE_BUTTON_LEFT)) ||
               (event.type == EMOUSE_BUTTON_RELEASE &&
                (event.previous_button_state & EMOUSE_BUTTON_LEFT))) {
      /* Complex handling here is required to prevent claiming the X11 selection
         when no text is selected at all. The basic idea however is to start the
         selection if none has been started yet, move the cursor and move the end
         of the selection to the new cursor location. */
      text_coordinate_t new_cursor = xy_to_text_coordinate(event.x, event.y);
      if (text->get_selection_mode() == selection_mode_t::NONE &&
          new_cursor != text->get_cursor()) {
        text->set_selection_mode(selection_mode_t::SHIFT);
      }
      text->set_cursor(new_cursor);
      if (text->get_selection_mode() != selection_mode_t::NONE) {
        text->set_selection_end(event.type == EMOUSE_BUTTON_RELEASE);
      }
      ensure_cursor_on_screen();
      impl->last_set_pos = impl->screen_pos;
    }
  }
  return true;
}

void edit_window_t::set_wrap(wrap_type_t wrap) {
  if (wrap == impl->wrap_type) {
    return;
  }

  if (wrap == wrap_type_t::NONE) {
    impl->top_left.pos = 0;
    if (impl->wrap_info != nullptr) {
      delete impl->wrap_info;
      impl->wrap_info = nullptr;
    }
  } else {
    // FIXME: differentiate between wrap types
    if (impl->wrap_info == nullptr) {
      impl->wrap_info = new wrap_info_t(impl->edit_window.get_width() - 1, impl->tabsize);
    }
    impl->wrap_info->set_text_buffer(text);
    impl->wrap_info->set_wrap_width(impl->edit_window.get_width() - 1);
    impl->top_left.pos = impl->wrap_info->find_line(impl->top_left);
  }
  impl->wrap_type = wrap;
  update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
  ensure_cursor_on_screen();
}

void edit_window_t::set_tab_spaces(bool _tab_spaces) { impl->tab_spaces = _tab_spaces; }

void edit_window_t::set_auto_indent(bool _auto_indent) { impl->auto_indent = _auto_indent; }

void edit_window_t::set_indent_aware_home(bool _indent_aware_home) {
  impl->indent_aware_home = _indent_aware_home;
}

void edit_window_t::set_show_tabs(bool _show_tabs) { impl->show_tabs = _show_tabs; }

int edit_window_t::get_tabsize() const { return impl->tabsize; }

wrap_type_t edit_window_t::get_wrap() const { return impl->wrap_type; }

bool edit_window_t::get_tab_spaces() const { return impl->tab_spaces; }

bool edit_window_t::get_auto_indent() const { return impl->auto_indent; }

bool edit_window_t::get_indent_aware_home() const { return impl->indent_aware_home; }

bool edit_window_t::get_show_tabs() const { return impl->show_tabs; }

std::unique_ptr<edit_window_t::view_parameters_t> edit_window_t::save_view_parameters() {
  // This can't use make_unique, as the constructor is private and only this class is a friend.
  return wrap_unique(new view_parameters_t(this));
}

void edit_window_t::save_view_parameters(view_parameters_t *params) {
  *params = view_parameters_t(this);
}

std::unique_ptr<edit_window_t::behavior_parameters_t> edit_window_t::save_behavior_parameters() {
  // This can't use make_unique, as the constructor is private and only this class is a friend.
  return wrap_unique(new behavior_parameters_t(*this));
}

void edit_window_t::save_behavior_parameters(behavior_parameters_t *params) {
  *params = behavior_parameters_t(*this);
}

void edit_window_t::draw_info_window() {}

void edit_window_t::set_autocompleter(autocompleter_t *_autocompleter) {
  impl->autocomplete_panel->hide();
  impl->autocompleter.reset(_autocompleter);
}

void edit_window_t::autocomplete() { activate_autocomplete(true); }

void edit_window_t::activate_autocomplete(bool autocomplete_single) {
  if (impl->autocompleter == nullptr) {
    return;
  }

  text_coordinate_t anchor(text->get_cursor());
  string_list_base_t *autocomplete_list =
      impl->autocompleter->build_autocomplete_list(text, &anchor.pos);

  if (autocomplete_list != nullptr) {
    if (autocomplete_single && autocomplete_list->size() == 1) {
      impl->autocompleter->autocomplete(text, 0);
      /* Just in case... */
      impl->autocomplete_panel->hide();
      return;
    }

    impl->autocomplete_panel->set_completions(autocomplete_list);
    const text_coordinate_t cursor = text->get_cursor();
    if (impl->wrap_type == wrap_type_t::NONE) {
      text_pos_t position = text->calculate_screen_pos(anchor, impl->tabsize);
      impl->autocomplete_panel->set_position((cursor.line - impl->top_left.line + 1),
                                             (position - impl->top_left.pos - 1));
    } else {
      text_pos_t sub_line = impl->wrap_info->find_line(cursor);
      text_pos_t position = impl->wrap_info->calculate_screen_pos(anchor);
      text_pos_t line;

      if (cursor.line == impl->top_left.line) {
        line = sub_line - impl->top_left.pos;
      } else {
        line = impl->wrap_info->get_line_count(impl->top_left.line) - impl->top_left.pos + sub_line;
        for (text_pos_t i = impl->top_left.line + 1; i < cursor.line; i++) {
          line += impl->wrap_info->get_line_count(i);
        }
      }
      impl->autocomplete_panel->set_position(line + 1, position - 1);
    }
    impl->autocomplete_panel->show();
  } else if (impl->autocomplete_panel->is_shown()) {
    impl->autocomplete_panel->hide();
  }
}

void edit_window_t::autocomplete_activated() {
  size_t idx = impl->autocomplete_panel->get_selected_idx();
  impl->autocomplete_panel->hide();
  impl->autocompleter->autocomplete(text, idx);
}

text_coordinate_t edit_window_t::xy_to_text_coordinate(int x, int y) const {
  text_coordinate_t coord;
  text_pos_t x_pos = x;
  text_pos_t y_pos = y;
  if (impl->wrap_type == wrap_type_t::NONE) {
    coord.line = y_pos + impl->top_left.line;
    x_pos += impl->top_left.pos;
    if (coord.line >= text->size()) {
      coord.line = text->size() - 1;
      x_pos = std::numeric_limits<text_pos_t>::max();
    }
    if (coord.line < 0) {
      coord.line = 0;
      coord.pos = 0;
    } else {
      coord.pos = text->calculate_line_pos(coord.line, x_pos, impl->tabsize);
    }
  } else {
    coord.line = impl->top_left.line;
    y_pos += impl->top_left.pos;
    while (y_pos < 0 && coord.line > 0) {
      coord.line--;
      y_pos += impl->wrap_info->get_line_count(coord.line);
    }
    while (coord.line < impl->wrap_info->unwrapped_size() - 1 &&
           y_pos >= impl->wrap_info->get_line_count(coord.line)) {
      y_pos -= impl->wrap_info->get_line_count(coord.line);
      coord.line++;
    }
    if (y_pos >= impl->wrap_info->get_line_count(coord.line)) {
      y_pos = impl->wrap_info->get_line_count(coord.line) - 1;
      x_pos = std::numeric_limits<text_pos_t>::max();
    }
    if (y_pos < 0) {
      coord.pos = 0;
    } else {
      coord.pos = impl->wrap_info->calculate_line_pos(coord.line, x_pos, y_pos);
    }
  }
  return coord;
}

void edit_window_t::scroll(text_pos_t lines) {
  // FIXME: maybe we should use this for pgup/pgdn and up/down as well?
  if (impl->wrap_type == wrap_type_t::NONE) {
    if (lines < 0) {
      if (impl->top_left.line > -lines) {
        impl->top_left.line += lines;
      } else {
        impl->top_left.line = 0;
      }
    } else {
      if (impl->top_left.line + impl->edit_window.get_height() <= text->size() - lines) {
        impl->top_left.line += lines;
      } else if (impl->top_left.line + impl->edit_window.get_height() - 1 < text->size()) {
        impl->top_left.line = text->size() - impl->edit_window.get_height();
      }
    }
  } else {
    if (lines < 0) {
      impl->wrap_info->sub_lines(impl->top_left, -lines);
    } else {
      impl->wrap_info->add_lines(impl->top_left, lines);
    }
  }
  update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
}

void edit_window_t::scrollbar_clicked(scrollbar_t::step_t step) {
  scroll(step == scrollbar_t::BACK_SMALL
             ? -3
             : step == scrollbar_t::FWD_SMALL
                   ? 3
                   : step == scrollbar_t::BACK_MEDIUM
                         ? -impl->edit_window.get_height() / 2
                         : step == scrollbar_t::FWD_MEDIUM
                               ? impl->edit_window.get_height() / 2
                               : step == scrollbar_t::BACK_PAGE
                                     ? -(impl->edit_window.get_height() - 1)
                                     : step == scrollbar_t::FWD_PAGE
                                           ? (impl->edit_window.get_height() - 1)
                                           : 0);
}

void edit_window_t::scrollbar_dragged(text_pos_t start) {
  if (impl->wrap_type == wrap_type_t::NONE) {
    if (start >= 0 && start + impl->edit_window.get_height() <= text->size() &&
        start != impl->top_left.line) {
      impl->top_left.line = start;
      update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
    }
  } else {
    text_coordinate_t new_top_left(0, 0);
    text_pos_t count = 0;

    if (start < 0 || start + impl->edit_window.get_height() > impl->wrap_info->wrapped_size()) {
      return;
    }

    for (new_top_left.line = 0; new_top_left.line < text->size() && count < start;
         new_top_left.line++) {
      count += impl->wrap_info->get_line_count(new_top_left.line);
    }

    if (count > start) {
      new_top_left.line--;
      count -= impl->wrap_info->get_line_count(new_top_left.line);
      new_top_left.pos = start - count;
    }

    if (new_top_left == impl->top_left || new_top_left.line < 0) {
      return;
    }
    impl->top_left = new_top_left;
    update_repaint_lines(0, std::numeric_limits<text_pos_t>::max());
  }
}

void edit_window_t::update_repaint_lines(text_pos_t line) { update_repaint_lines(line, line); }

void edit_window_t::update_repaint_lines(text_pos_t start, text_pos_t end) {
  if (start > end) {
    text_pos_t tmp = start;
    start = end;
    end = tmp;
  }

  if (start < impl->repaint_min) {
    impl->repaint_min = start;
  }
  if (end > impl->repaint_max) {
    impl->repaint_max = end;
  }
  widget_t::force_redraw();
}

void edit_window_t::delete_line() {
  text_coordinate_t start;
  text_coordinate_t end;
  if (text->selection_empty()) {
    start = text->get_cursor();
    end = text->get_cursor();
  } else {
    if (text->get_selection_start() < text->get_selection_end()) {
      start = text->get_selection_start();
      end = text->get_selection_end();
    } else {
      start = text->get_selection_end();
      end = text->get_selection_start();
    }
  }
  reset_selection();
  text_pos_t saved_pos = text->calculate_screen_pos(0);
  start.pos = 0;
  if (end.line + 1 >= text->size()) {
    end.pos = text->get_line_size(end.line);
  } else {
    end.line = end.line + 1;
    end.pos = 0;
  }
  text->delete_block(start, end);
  text->set_cursor_pos(text->calculate_line_pos(text->get_cursor().line, saved_pos, impl->tabsize));
  ensure_cursor_on_screen();
}

void edit_window_t::mark_selection() {
  switch (text->get_selection_mode()) {
    case selection_mode_t::MARK:
      reset_selection();
      break;
    case selection_mode_t::NONE:
    case selection_mode_t::ALL:
    case selection_mode_t::SHIFT:
      text->set_selection_mode(selection_mode_t::MARK);
      break;
    default:
      /* Should not happen, but just try to get back to a sane state. */
      reset_selection();
      break;
  }
}

//====================== view_parameters_t ========================

edit_window_t::view_parameters_t::view_parameters_t(edit_window_t *view) {
  top_left = view->impl->top_left;
  wrap_type = view->impl->wrap_type;
  if (wrap_type != wrap_type_t::NONE) {
    top_left.pos = view->impl->wrap_info->calculate_line_pos(top_left.line, 0, top_left.pos);
  }
  tabsize = view->impl->tabsize;
  tab_spaces = view->impl->tab_spaces;
  ins_mode = view->impl->ins_mode;
  last_set_pos = view->impl->last_set_pos;
  auto_indent = view->impl->auto_indent;
  indent_aware_home = view->impl->indent_aware_home;
  show_tabs = view->impl->show_tabs;
}

edit_window_t::view_parameters_t::view_parameters_t()
    : top_left(0, 0),
      wrap_type(wrap_type_t::NONE),
      tabsize(8),
      tab_spaces(false),
      ins_mode(0),
      last_set_pos(0),
      auto_indent(true),
      indent_aware_home(true),
      show_tabs(false) {}

void edit_window_t::view_parameters_t::apply_parameters(edit_window_t *view) const {
  view->impl->top_left = top_left;
  view->impl->tabsize = tabsize;
  view->set_wrap(wrap_type);
  /* view->set_wrap will make sure that view->wrap_info is nullptr if
     wrap_type != NONE. */
  if (view->impl->wrap_info != nullptr) {
    view->impl->wrap_info->set_text_buffer(view->text);
    view->impl->top_left.pos = view->impl->wrap_info->find_line(top_left);
  }
  // the calling function will call ensure_cursor_on_screen
  view->impl->tab_spaces = tab_spaces;
  view->impl->ins_mode = ins_mode;
  view->impl->last_set_pos = last_set_pos;
  view->impl->auto_indent = auto_indent;
  view->impl->indent_aware_home = indent_aware_home;
  view->impl->show_tabs = show_tabs;
}

void edit_window_t::view_parameters_t::set_tabsize(int _tabsize) { tabsize = _tabsize; }
void edit_window_t::view_parameters_t::set_wrap(wrap_type_t _wrap_type) { wrap_type = _wrap_type; }
void edit_window_t::view_parameters_t::set_tab_spaces(bool _tab_spaces) {
  tab_spaces = _tab_spaces;
}
void edit_window_t::view_parameters_t::set_auto_indent(bool _auto_indent) {
  auto_indent = _auto_indent;
}
void edit_window_t::view_parameters_t::set_indent_aware_home(bool _indent_aware_home) {
  indent_aware_home = _indent_aware_home;
}
void edit_window_t::view_parameters_t::set_show_tabs(bool _show_tabs) { show_tabs = _show_tabs; }

//====================== behavior_parameters_t ========================

edit_window_t::behavior_parameters_t::behavior_parameters_t(const edit_window_t &view)
    : impl(new implementation_t) {
  impl->top_left = view.impl->top_left;
  impl->wrap_type = view.impl->wrap_type;
  if (impl->wrap_type != wrap_type_t::NONE) {
    impl->top_left.pos =
        view.impl->wrap_info->calculate_line_pos(impl->top_left.line, 0, impl->top_left.pos);
  }
  impl->tabsize = view.impl->tabsize;
  impl->tab_spaces = view.impl->tab_spaces;
  impl->ins_mode = view.impl->ins_mode;
  impl->last_set_pos = view.impl->last_set_pos;
  impl->auto_indent = view.impl->auto_indent;
  impl->indent_aware_home = view.impl->indent_aware_home;
  impl->show_tabs = view.impl->show_tabs;
}

edit_window_t::behavior_parameters_t::behavior_parameters_t() : impl(new implementation_t) {}

edit_window_t::behavior_parameters_t::behavior_parameters_t(
    const edit_window_t::view_parameters_t &params)
    : impl(new implementation_t) {
  impl->top_left = params.top_left;
  impl->wrap_type = params.wrap_type;
  impl->tabsize = params.tabsize;
  impl->tab_spaces = params.tab_spaces;
  impl->ins_mode = params.ins_mode;
  impl->last_set_pos = params.last_set_pos;
  impl->auto_indent = params.auto_indent;
  impl->indent_aware_home = params.indent_aware_home;
  impl->show_tabs = params.show_tabs;
}

/* Destructor must be defined in the .cc file, because in the .h file the size of implementation_t
   is undefined. */
edit_window_t::behavior_parameters_t::~behavior_parameters_t() {}

edit_window_t::behavior_parameters_t &edit_window_t::behavior_parameters_t::operator=(
    const behavior_parameters_t &other) {
  *impl = *other.impl;
  return *this;
}

void edit_window_t::behavior_parameters_t::apply_parameters(edit_window_t *view) const {
  view->impl->top_left = impl->top_left;
  view->impl->tabsize = impl->tabsize;
  view->set_wrap(impl->wrap_type);
  /* view->set_wrap will make sure that view->wrap_info is nullptr if
     wrap_type != NONE. */
  if (view->impl->wrap_info != nullptr) {
    view->impl->wrap_info->set_text_buffer(view->text);
    view->impl->top_left.pos = view->impl->wrap_info->find_line(impl->top_left);
  }
  // the calling function will call ensure_cursor_on_screen
  view->impl->tab_spaces = impl->tab_spaces;
  view->impl->ins_mode = impl->ins_mode;
  view->impl->last_set_pos = impl->last_set_pos;
  view->impl->auto_indent = impl->auto_indent;
  view->impl->indent_aware_home = impl->indent_aware_home;
  view->impl->show_tabs = impl->show_tabs;
}

void edit_window_t::behavior_parameters_t::set_tabsize(int _tabsize) { impl->tabsize = _tabsize; }
void edit_window_t::behavior_parameters_t::set_wrap(wrap_type_t _wrap_type) {
  impl->wrap_type = _wrap_type;
}
void edit_window_t::behavior_parameters_t::set_tab_spaces(bool _tab_spaces) {
  impl->tab_spaces = _tab_spaces;
}
void edit_window_t::behavior_parameters_t::set_auto_indent(bool _auto_indent) {
  impl->auto_indent = _auto_indent;
}
void edit_window_t::behavior_parameters_t::set_indent_aware_home(bool _indent_aware_home) {
  impl->indent_aware_home = _indent_aware_home;
}
void edit_window_t::behavior_parameters_t::set_show_tabs(bool _show_tabs) {
  impl->show_tabs = _show_tabs;
}
void edit_window_t::behavior_parameters_t::set_top_left(text_coordinate_t pos) {
  impl->top_left = pos;
}

int edit_window_t::behavior_parameters_t::get_tabsize() const { return impl->tabsize; }
wrap_type_t edit_window_t::behavior_parameters_t::get_wrap_type() const { return impl->wrap_type; }
bool edit_window_t::behavior_parameters_t::get_tab_spaces() const { return impl->tab_spaces; }
bool edit_window_t::behavior_parameters_t::get_auto_indent() const { return impl->auto_indent; }
bool edit_window_t::behavior_parameters_t::get_indent_aware_home() const {
  return impl->indent_aware_home;
}
bool edit_window_t::behavior_parameters_t::get_show_tabs() const { return impl->show_tabs; }
text_coordinate_t edit_window_t::behavior_parameters_t::get_top_left() const {
  return impl->top_left;
}

//====================== autocomplete_panel_t ========================

edit_window_t::autocomplete_panel_t::autocomplete_panel_t(edit_window_t *parent)
    : popup_t(7, 7), list_pane(nullptr) {
  window.set_anchor(parent->get_base_window(), 0);

  list_pane = emplace_back<list_pane_t>(false);
  list_pane->set_size(5, 6);
  list_pane->set_position(1, 1);
  list_pane->set_focus(window_component_t::FOCUS_SET);
  list_pane->set_single_click_activate(true);
}

bool edit_window_t::autocomplete_panel_t::process_key(key_t key) {
  if (popup_t::process_key(key)) {
    return true;
  }
  if (key <= 0x20 || key >= EKEY_FIRST_SPECIAL) {
    hide();
  }
  return false;
}

void edit_window_t::autocomplete_panel_t::set_position(optint _top, optint _left) {
  int screen_height, screen_width;
  int left, top, absolute_pos;

  get_screen_size(&screen_height, &screen_width);

  top = _top.is_valid() ? _top.value() : window.get_y();
  left = _left.is_valid() ? _left.value() : window.get_x();

  window.move(top, left);

  absolute_pos = window.get_abs_x();
  if (absolute_pos + window.get_width() > screen_width) {
    left -= absolute_pos + window.get_width() - screen_width;
  }

  absolute_pos = window.get_abs_y();
  if (absolute_pos + window.get_height() > screen_height) {
    top = top - window.get_height() - 2;
  }

  window.move(top, left);
  force_redraw();
}

bool edit_window_t::autocomplete_panel_t::set_size(optint height, optint width) {
  bool result;
  result = popup_t::set_size(height, width);
  result &= list_pane->set_size(window.get_height() - 2, window.get_width() - 1);
  return result;
}

void edit_window_t::autocomplete_panel_t::set_completions(string_list_base_t *completions) {
  int new_width = 1;
  int new_height;

  while (!list_pane->empty()) {
    list_pane->pop_back();
  }

  for (const std::string &str : *completions) {
    std::unique_ptr<label_t> label(new label_t(str));
    if (label->get_text_width() > new_width) {
      new_width = label->get_text_width();
    }
    list_pane->push_back(std::move(label));
  }

  new_height = list_pane->size() + 2;
  if (new_height > 7) {
    new_height = 7;
  } else if (new_height < 5) {
    new_height = 5;
  }

  set_size(new_height, new_width + 3);
}

size_t edit_window_t::autocomplete_panel_t::get_selected_idx() const {
  return list_pane->get_current();
}

void edit_window_t::autocomplete_panel_t::connect_activate(std::function<void()> func) {
  list_pane->connect_activate(func);
}

}  // namespace t3widget
