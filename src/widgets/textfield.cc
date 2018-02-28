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

#include "clipboard.h"
#include "colorscheme.h"
#include "internal.h"
#include "log.h"
#include "main.h"
#include "t3window/utf8.h"
#include "widgets/label.h"
#include "widgets/textfield.h"

namespace t3_widget {

/*FIXME:
- auto-completion
        - when typing directories, the entries in the dir should complete as well
        - pgup pgdn should take steps in the drop-down list
        - the first autocompletion option should be filled in, in the text field itself.
          However, this text should be selected, and it should only be appended if the
          user added characters.
- undo
*/
#define _T3_ACTION_FILE "widgets/textfield.actions.h"
#define _T3_ACTION_TYPE text_field_t
#include "key_binding_def.h"

text_field_t::text_field_t() : widget_t(1, 4), impl(new implementation_t()) { reset_selection(); }

void text_field_t::reset_selection() {
  impl->selection_start_pos = -1;
  impl->selection_end_pos = -1;
  impl->selection_mode = selection_mode_t::NONE;
  redraw = true;
}

void text_field_t::set_selection(key_t key) {
  switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
    case EKEY_END:
    case EKEY_HOME:
    case EKEY_LEFT:
    case EKEY_RIGHT:
      if (impl->selection_mode == selection_mode_t::SHIFT && !(key & EKEY_SHIFT)) {
        reset_selection();
      } else if (impl->selection_mode == selection_mode_t::NONE && (key & EKEY_SHIFT)) {
        impl->selection_start_pos = impl->pos;
        impl->selection_end_pos = impl->pos;
        impl->selection_mode = selection_mode_t::SHIFT;
      }
      break;
    default:
      break;
  }
}

void text_field_t::delete_selection(bool save_to_copy_buffer) {
  text_line_t *result;

  int start, end;
  if (impl->selection_start_pos == impl->selection_end_pos) {
    reset_selection();
    return;
  } else if (impl->selection_start_pos < impl->selection_end_pos) {
    start = impl->selection_start_pos;
    end = impl->selection_end_pos;
  } else {
    start = impl->selection_end_pos;
    end = impl->selection_start_pos;
  }

  result = impl->line->cut_line(start, end);
  if (save_to_copy_buffer) {
    set_clipboard(make_unique<std::string>(*result->get_data()));
  }

  delete result;

  impl->pos = start;
  ensure_cursor_on_screen();
  reset_selection();
  redraw = true;
  impl->edited = true;
}

bool text_field_t::process_key(key_t key) {
  set_selection(key);

  switch (key) {
    case EKEY_DOWN:
      if (impl->drop_down_list != nullptr && !impl->drop_down_list->empty() &&
          impl->line->get_length() > 0) {
        impl->in_drop_down_list = true;
        impl->drop_down_list->show();
        impl->drop_down_list->set_focus(window_component_t::FOCUS_SET);
        reset_selection();
        redraw = true;
      } else {
        move_focus_down();
      }
      break;
    case EKEY_UP:
      move_focus_up();
      break;
    case EKEY_BS:
      if (impl->selection_mode != selection_mode_t::NONE) {
        delete_selection(false);
      } else if (impl->pos > 0) {
        int newpos = impl->line->adjust_position(impl->pos, -1);
        impl->line->backspace_char(impl->pos, nullptr);
        impl->pos = newpos;
        ensure_cursor_on_screen();
        redraw = true;
        impl->edited = true;
      }
      break;
    case EKEY_DEL:
      if (impl->selection_mode != selection_mode_t::NONE) {
        delete_selection(false);
      } else if (impl->pos < impl->line->get_length()) {
        impl->line->delete_char(impl->pos, nullptr);
        redraw = true;
        impl->edited = true;
      }
      break;
    case EKEY_LEFT:
    case EKEY_LEFT | EKEY_SHIFT:
      if (impl->pos > 0) {
        redraw = true;
        impl->pos = impl->line->adjust_position(impl->pos, -1);
        ensure_cursor_on_screen();
      }
      break;
    case EKEY_RIGHT:
    case EKEY_RIGHT | EKEY_SHIFT:
      if (impl->pos < impl->line->get_length()) {
        redraw = true;
        impl->pos = impl->line->adjust_position(impl->pos, 1);
        ensure_cursor_on_screen();
      }
      break;
    case EKEY_RIGHT | EKEY_CTRL:
    case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
      if (impl->pos < impl->line->get_length()) {
        redraw = true;
        impl->pos = impl->line->get_next_word(impl->pos);
        if (impl->pos < 0) {
          impl->pos = impl->line->get_length();
        }
        ensure_cursor_on_screen();
      }
      break;
    case EKEY_LEFT | EKEY_CTRL:
    case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
      if (impl->pos > 0) {
        redraw = true;
        impl->pos = impl->line->get_previous_word(impl->pos);
        if (impl->pos < 0) {
          impl->pos = 0;
        }
        ensure_cursor_on_screen();
      }
      break;
    case EKEY_HOME:
    case EKEY_HOME | EKEY_SHIFT:
      redraw = true;
      impl->pos = 0;
      ensure_cursor_on_screen();
      break;
    case EKEY_END:
    case EKEY_END | EKEY_SHIFT:
      redraw = true;
      impl->pos = impl->line->get_length();
      ensure_cursor_on_screen();
      break;

    case EKEY_NL:
      if (impl->drop_down_list != nullptr && impl->drop_down_list->is_shown()) {
        impl->drop_down_list->hide();
        impl->in_drop_down_list = false;
      }
      activate();
      break;

    case EKEY_HOTKEY:
      return true;

    default: {
      optional<Action> action = key_bindings.find_action(key);
      if (action.is_valid()) {
        switch (action) {
          case ACTION_CUT:
            if (impl->selection_mode != selection_mode_t::NONE) {
              delete_selection(true);
            }
            return true;
          case ACTION_COPY:
            if (impl->selection_mode != selection_mode_t::NONE) {
              int start, end;
              if (impl->selection_start_pos == impl->selection_end_pos) {
                reset_selection();
                break;
              } else if (impl->selection_start_pos < impl->selection_end_pos) {
                start = impl->selection_start_pos;
                end = impl->selection_end_pos;
              } else {
                start = impl->selection_end_pos;
                end = impl->selection_start_pos;
              }

              set_clipboard(make_unique<std::string>(*impl->line->get_data(), start, end - start));
            }
            return true;

          case ACTION_PASTE:
          case ACTION_PASTE_SELECTION: {
            ensure_clipboard_lock_t lock;
            std::shared_ptr<std::string> copy_buffer =
                action == ACTION_PASTE ? get_clipboard() : get_primary();
            if (copy_buffer != nullptr) {
              text_line_t insert_line(*copy_buffer.get());

              // Don't allow pasting of values that do not match the filter
              if (impl->filter_keys != nullptr) {
                const std::string *insert_data = insert_line.get_data();
                size_t insert_data_length = insert_data->size();
                size_t bytes_read;
                do {
                  key_t c;
                  bytes_read = insert_data_length;
                  c = t3_utf8_get(insert_data->data() + insert_data->size() - insert_data_length,
                                  &bytes_read);
                  if ((std::find(impl->filter_keys, impl->filter_keys + impl->filter_keys_size,
                                 c) == impl->filter_keys + impl->filter_keys_size) ==
                      impl->filter_keys_accept)
                    return true;
                  insert_data_length -= bytes_read;
                } while (insert_data_length > 0 && bytes_read > 0);
              }

              if (impl->selection_mode != selection_mode_t::NONE) delete_selection(false);

              impl->line->insert(&insert_line, impl->pos);
              impl->pos += insert_line.get_length();
              ensure_cursor_on_screen();
              redraw = true;
              impl->edited = true;
            }
            return true;
          }
          case ACTION_MARK_SELECTION:
            switch (impl->selection_mode) {
              case selection_mode_t::MARK:
                reset_selection();
                break;
              case selection_mode_t::NONE:
                impl->selection_start_pos = impl->pos;
                impl->selection_end_pos = impl->pos;
              /* FALLTHROUGH */
              case selection_mode_t::SHIFT:
                impl->selection_mode = selection_mode_t::MARK;
                break;
              default:
                /* Should not happen, but just try to get back to a sane state. */
                reset_selection();
                break;
            }
            return true;
          case ACTION_INSERT_SPECIAL:
            impl->dont_select_on_focus = true;
            insert_char_dialog->center_over(center_window);
            insert_char_dialog->reset();
            insert_char_dialog->show();
            return true;
          case ACTION_SELECT_ALL:
            if (impl->selection_mode == selection_mode_t::NONE) {
              impl->selection_mode = selection_mode_t::SHIFT;
            }
            impl->selection_start_pos = 0;
            impl->selection_end_pos = impl->line->get_length();
            impl->pos = impl->selection_end_pos;
            redraw = true;
            return true;

          default:
            break;
        }
      }
      if (key < 31) {
        return false;
      }

      key &= ~EKEY_PROTECT;

      if (key > 0x10ffff) {
        return false;
      }

      if (impl->filter_keys != nullptr &&
          (std::find(impl->filter_keys, impl->filter_keys + impl->filter_keys_size, key) ==
           impl->filter_keys + impl->filter_keys_size) == impl->filter_keys_accept) {
        return false;
      }

      if (impl->selection_mode != selection_mode_t::NONE) {
        delete_selection(false);
      }

      if (impl->pos == impl->line->get_length()) {
        impl->line->append_char(key, nullptr);
      } else {
        impl->line->insert_char(impl->pos, key, nullptr);
      }
      impl->pos = impl->line->adjust_position(impl->pos, 1);
      ensure_cursor_on_screen();
      redraw = true;
      impl->edited = true;
    }
  }

  return true;
}

bool text_field_t::set_size(optint height, optint width) {
  (void)height;
  if (width.is_valid() && window.get_width() != width) {
    window.resize(1, width);
    if (impl->drop_down_list != nullptr) {
      impl->drop_down_list->set_size(None, width);
    }
  }

  ensure_cursor_on_screen();

  redraw = true;
  // FIXME: use return values from different parts as return value!
  return true;
}

void text_field_t::update_contents() {
  if (impl->drop_down_list != nullptr && impl->edited) {
    impl->drop_down_list->update_view();
    if (!impl->drop_down_list->empty() && impl->line->get_length() > 0) {
      impl->drop_down_list->show();
    } else {
      impl->drop_down_list->hide();
    }
  }

  if (impl->drop_down_list != nullptr && !impl->drop_down_list->empty()) {
    impl->drop_down_list->update_contents();
  }

  if (!redraw) {
    return;
  }

  impl->edited = false;
  redraw = false;

  if (impl->selection_mode != selection_mode_t::NONE) {
    if (impl->selection_mode == selection_mode_t::SHIFT && impl->selection_start_pos == impl->pos) {
      reset_selection();
    } else {
      set_selection_end();
    }
  }

  text_line_t::paint_info_t info;

  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.addch(impl->leftcol == 0 ? '[' : '(', 0);

  info.start = 0;
  info.leftcol = impl->leftcol;
  info.max = INT_MAX;
  info.size = window.get_width() - 2;
  info.tabsize = 0;
  info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL;
  if (!impl->focus) {
    info.selection_start = -1;
    info.selection_end = -1;
  } else if (impl->selection_start_pos < impl->selection_end_pos) {
    info.selection_start = impl->selection_start_pos;
    info.selection_end = impl->selection_end_pos;
  } else {
    info.selection_start = impl->selection_end_pos;
    info.selection_end = impl->selection_start_pos;
  }
  info.cursor = impl->focus && !impl->in_drop_down_list ? impl->pos : -1;
  info.normal_attr = attributes.dialog;
  info.selected_attr = attributes.dialog_selected;

  impl->line->paint_line(&window, &info);
  window.addch(
      impl->line->calculate_screen_width(impl->leftcol, INT_MAX, 0) > window.get_width() - 2 ? ')'
                                                                                             : ']',
      0);
}

void text_field_t::set_focus(focus_t _focus) {
  lprintf("set focus %d\n", _focus);
  impl->focus = _focus;
  redraw = true;
  if (impl->focus) {
    if (!impl->dont_select_on_focus) {
      impl->selection_start_pos = 0;
      impl->selection_mode = selection_mode_t::SHIFT;
      impl->pos = impl->line->get_length();
      set_selection_end();
    }
    impl->dont_select_on_focus = false;
    if (impl->drop_down_list != nullptr) {
      impl->drop_down_list->update_view();
    }
  } else {
    if (impl->drop_down_list != nullptr) {
      impl->drop_down_list->set_focus(window_component_t::FOCUS_OUT);
      impl->drop_down_list->hide();
    }
    impl->in_drop_down_list = false;
  }
}

void text_field_t::show() {
  impl->in_drop_down_list = false;
  widget_t::show();
}

void text_field_t::hide() {
  if (impl->drop_down_list != nullptr) {
    impl->drop_down_list->hide();
  }
  widget_t::hide();
}

void text_field_t::ensure_cursor_on_screen() {
  int width, char_width;

  if (impl->pos == impl->line->get_length()) {
    char_width = 1;
  } else {
    char_width = impl->line->width_at(impl->pos);
  }

  impl->screen_pos = impl->line->calculate_screen_width(0, impl->pos, 0);

  if (impl->screen_pos < impl->leftcol) {
    impl->leftcol = impl->screen_pos;
    redraw = true;
  }

  width = window.get_width();
  if (impl->screen_pos + char_width > impl->leftcol + width - 2) {
    impl->leftcol = impl->screen_pos - (width - 2) + char_width;
    redraw = true;
  }
}

void text_field_t::set_text(const std::string *text) { set_text(text->data(), text->size()); }

void text_field_t::set_text(const char *text) { set_text(text, strlen(text)); }

void text_field_t::set_text(const char *text, size_t size) {
  impl->line->set_text(string_view(text, size));
  impl->pos = impl->line->get_length();
  impl->leftcol = 0;
  ensure_cursor_on_screen();
  redraw = true;
}

void text_field_t::set_key_filter(key_t *keys, size_t nr_of_keys, bool accept) {
  impl->filter_keys = keys;
  impl->filter_keys_size = nr_of_keys;
  impl->filter_keys_accept = accept;
}

const std::string *text_field_t::get_text() const { return impl->line->get_data(); }

void text_field_t::set_autocomplete(string_list_base_t *completions) {
  if (impl->drop_down_list == nullptr) {
    impl->drop_down_list.reset(new drop_down_list_t(this));
  }
  impl->drop_down_list->set_autocomplete(completions);
}

void text_field_t::set_label(smart_label_t *_label) { impl->label = _label; }

bool text_field_t::is_hotkey(key_t key) {
  return impl->label == nullptr ? false : impl->label->is_hotkey(key);
}

void text_field_t::bad_draw_recheck() { redraw = true; }

bool text_field_t::process_mouse_event(mouse_event_t event) {
  if (event.button_state & EMOUSE_TRIPLE_CLICKED_LEFT) {
    impl->selection_mode = selection_mode_t::SHIFT;
    impl->selection_start_pos = 0;
    impl->pos = impl->line->get_length();
    set_selection_end(true);
    ensure_cursor_on_screen();
    redraw = true;
  } else if (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) {
    impl->selection_mode = selection_mode_t::SHIFT;
    impl->selection_start_pos = impl->line->get_previous_word_boundary(impl->pos);
    impl->pos = impl->line->get_next_word_boundary(impl->pos);
    set_selection_end(true);
    ensure_cursor_on_screen();
    redraw = true;
  } else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT) &&
             event.previous_button_state == 0) {
    if ((event.modifier_state & EMOUSE_SHIFT) == 0) {
      reset_selection();
    } else if (impl->selection_mode == selection_mode_t::NONE) {
      impl->selection_mode = selection_mode_t::SHIFT;
      impl->selection_start_pos = impl->pos;
    }
    impl->pos = impl->line->calculate_line_pos(0, INT_MAX, event.x - 1 + impl->leftcol, 0);
    if ((event.modifier_state & EMOUSE_SHIFT) != 0) {
      set_selection_end();
    }
    ensure_cursor_on_screen();
  } else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_MIDDLE)) {
    reset_selection();
    impl->pos = impl->line->calculate_line_pos(0, INT_MAX, event.x - 1 + impl->leftcol, 0);

    {
      ensure_clipboard_lock_t lock;
      std::shared_ptr<std::string> primary = get_primary();
      if (primary != nullptr) {
        text_line_t insert_line(*primary);

        impl->line->insert(&insert_line, impl->pos);
        impl->pos += insert_line.get_length();
      }
    }
    ensure_cursor_on_screen();
  } else if ((event.type == EMOUSE_MOTION && (event.button_state & EMOUSE_BUTTON_LEFT)) ||
             (event.type == EMOUSE_BUTTON_RELEASE &&
              (event.previous_button_state & EMOUSE_BUTTON_LEFT))) {
    /* Complex handling here is required to prevent claiming the X11 selection
       when no text is selected at all. The basic idea however is to start the
       selection if none has been started yet, move the cursor and move the end
       of the selection to the new cursor location. */
    int newpos = impl->line->calculate_line_pos(0, INT_MAX, event.x - 1 + impl->leftcol, 0);
    if (impl->selection_mode == selection_mode_t::NONE && newpos != impl->pos) {
      impl->selection_mode = selection_mode_t::SHIFT;
      impl->selection_start_pos = impl->pos;
    }
    impl->pos = newpos;
    if (impl->selection_mode != selection_mode_t::NONE) {
      set_selection_end(event.type == EMOUSE_BUTTON_RELEASE);
    }
    ensure_cursor_on_screen();
    redraw = true;
  }
  impl->dont_select_on_focus = true;
  return true;
}

void text_field_t::set_selection_end(bool update_primary) {
  impl->selection_end_pos = impl->pos;
  if (update_primary) {
    size_t start, length;

    if (impl->selection_start_pos == impl->selection_end_pos) {
      set_primary(nullptr);
      return;
    }

    if (impl->selection_start_pos < impl->selection_end_pos) {
      start = impl->selection_start_pos;
      length = impl->selection_end_pos - start;
    } else {
      start = impl->selection_end_pos;
      length = impl->selection_start_pos - start;
    }
    set_primary(make_unique<std::string>(*impl->line->get_data(), start, length));
  }
}

bool text_field_t::has_focus() const { return impl->focus; }

/*======================
  == drop_down_list_t ==
  ======================*/
#define DDL_HEIGHT 6

text_field_t::drop_down_list_t::drop_down_list_t(text_field_t *_field)
    : popup_t(DDL_HEIGHT, _field->get_base_window()->get_width(), false, false),
      field(_field),
      list_pane(nullptr) {
  window.set_anchor(field->get_base_window(),
                    T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  window.move(1, 0);

  list_pane = new list_pane_t(false);
  list_pane->set_size(DDL_HEIGHT - 1, window.get_width() - 1);
  list_pane->set_position(0, 1);
  list_pane->connect_activate([this] { item_activated(); });
  list_pane->connect_selection_changed([this] { selection_changed(); });
  list_pane->set_single_click_activate(true);

  push_back(list_pane);
}

bool text_field_t::drop_down_list_t::process_key(key_t key) {
  if (!field->impl->in_drop_down_list) {
    if (key == EKEY_ESC) {
      hide();
      return true;
    }
    return false;
  }

  switch (key) {
    case EKEY_UP:
      if (list_pane->get_current() == 0) {
        list_pane->set_focus(FOCUS_OUT);
        field->impl->in_drop_down_list = false;
        field->redraw = true;
      }
      break;
    case EKEY_DOWN:
    case EKEY_PGUP:
    case EKEY_PGDN:
      break;
    case EKEY_END:
    case EKEY_HOME:
      return false;
    case EKEY_NL:
      hide();
      return false;
    default:
      if (key >= 32 && key < EKEY_FIRST_SPECIAL) {
        list_pane->set_focus(FOCUS_OUT);
        // Make sure that the cursor will be visible, by forcing a redraw of the field
        field->redraw = true;
        field->impl->in_drop_down_list = false;
        return false;
      }
  }
  return popup_t::process_key(key);
}

void text_field_t::drop_down_list_t::set_position(optint top, optint left) {
  (void)top;
  (void)left;
}

bool text_field_t::drop_down_list_t::set_size(optint height, optint width) {
  bool result;
  (void)height;
  result = popup_t::set_size(DDL_HEIGHT, width);
  result &= list_pane->set_size(DDL_HEIGHT - 1, width - 1);
  return result;
}

void text_field_t::drop_down_list_t::update_contents() {
  bool saved_redraw = redraw;
  int i, width;

  popup_t::update_contents();
  if (saved_redraw) {
    width = window.get_width();
    window.set_default_attrs(attributes.dialog);
    window.set_paint(0, 0);
    window.clrtobot();
    for (i = 0; i < (DDL_HEIGHT - 1); i++) {
      window.set_paint(i, 0);
      window.addch(T3_ACS_VLINE, T3_ATTR_ACS);
      window.set_paint(i, width - 1);
      window.addch(T3_ACS_VLINE, T3_ATTR_ACS);
    }
    window.set_paint((DDL_HEIGHT - 1), 0);
    window.addch(T3_ACS_LLCORNER, T3_ATTR_ACS);
    window.addchrep(T3_ACS_HLINE, T3_ATTR_ACS, width - 2);
    window.addch(T3_ACS_LRCORNER, T3_ATTR_ACS);
  }
}

void text_field_t::drop_down_list_t::set_focus(focus_t focus) {
  if (focus && field->impl->in_drop_down_list) {
    field->set_text((*completions)[list_pane->get_current()]);
  }
  popup_t::set_focus(focus);
}

void text_field_t::drop_down_list_t::hide() {
  field->impl->in_drop_down_list = false;
  field->redraw = true;
  popup_t::hide();
}

void text_field_t::drop_down_list_t::show() {
  if (!is_shown()) {
    popup_t::show();
    list_pane->set_focus(FOCUS_OUT);
  }
}

void text_field_t::drop_down_list_t::update_view() {
  if (completions != nullptr) {
    if (field->impl->line->get_length() == 0) {
      completions->reset_filter();
    } else {
      completions->set_filter(bind_front(string_compare_filter, field->impl->line->get_data()));
    }
    update_list_pane();
  }
}

void text_field_t::drop_down_list_t::set_autocomplete(string_list_base_t *_completions) {
  /* completions is a unique_ptr, thus it will be deleted if it is not nullptr. */
  if (dynamic_cast<file_list_t *>(_completions) != nullptr) {
    completions.reset(new filtered_file_list_t(static_cast<file_list_t *>(_completions)));
  } else {
    completions.reset(new filtered_string_list_t(_completions));
  }
  update_list_pane();
}

bool text_field_t::drop_down_list_t::empty() { return completions->size() == 0; }

bool text_field_t::drop_down_list_t::process_mouse_event(mouse_event_t event) {
  if ((event.type & EMOUSE_OUTSIDE_GRAB) &&
      (event.type & ~EMOUSE_OUTSIDE_GRAB) == EMOUSE_BUTTON_PRESS) {
    hide();
    return false;
  }
  return true;
}

void text_field_t::drop_down_list_t::update_list_pane() {
  while (!list_pane->empty()) {
    widget_t *widget = list_pane->back();
    list_pane->pop_back();
    delete widget;
  }

  for (size_t i = 0; i < completions->size(); i++) {
    label_t *label = new label_t((*completions)[i]->c_str());
    list_pane->push_back(label);
  }
}

void text_field_t::drop_down_list_t::item_activated() {
  field->set_text((*completions)[list_pane->get_current()]);
  hide();
}

void text_field_t::drop_down_list_t::selection_changed() {
  field->set_text((*completions)[list_pane->get_current()]);
}

}  // namespace
