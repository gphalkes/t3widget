/* Copyright (C) 2011,2018 G.P. Halkes
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
#include "dialogs/messagedialog.h"
#include "main.h"
#include "textline.h"
#include <climits>
#include <cstdarg>
#include <cstring>

#define MESSAGEDIALOG_MAX_LINES 10

namespace t3_widget {

message_dialog_t::implementation_t::implementation_t() : max_text_height(MESSAGEDIALOG_MAX_LINES) {}

message_dialog_t::message_dialog_t(int width, optional<std::string> _title, ...)
    : dialog_t(5, width, std::move(_title)), impl(new implementation_t()) {
  va_list ap;
  button_t *button;
  const char *button_name;
  int total_width = 0;

  impl->text_window = new text_window_t(nullptr, false);
  impl->text_window->set_size(1, width - 2);
  impl->text_window->set_position(1, 1);
  impl->text_window->connect_activate([this] { hide(); });
  impl->text_window->connect_activate(activate_internal.get_trigger());
  impl->text_window->set_tabsize(0);
  impl->text_window->set_enabled(false);

  push_back(impl->text_window);

  va_start(ap, _title);
  while ((button_name = va_arg(ap, const char *)) != nullptr) {
    if (widgets.size() == 1) {
      button = new button_t(button_name, true);
      button->connect_activate([this] { hide(); });
      button->connect_activate(activate_internal.get_trigger());
      button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
    } else {
      button = new button_t(button_name);
      button->set_anchor(widgets.back(),
                         T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
      button->set_position(0, 2);

      static_cast<button_t *>(widgets.back())->connect_move_focus_right([this] { focus_next(); });
      button->connect_move_focus_left([this] { focus_previous(); });
      button->connect_activate([this] { hide(); });

      total_width += 2;
    }

    total_width += button->get_width();
    push_back(button);
  }

  widgets[1]->set_position(-1, -total_width / 2);
}

message_dialog_t::~message_dialog_t() { delete impl->text_window->get_text(); }

void message_dialog_t::set_message(string_view message) {
  text_buffer_t *old_text;
  text_buffer_t *text = new text_buffer_t();
  int text_height;

  impl->text_window->set_size(None, window.get_width() - 2);

  text->append_text(message);
  old_text = impl->text_window->get_text();
  impl->text_window->set_text(text);
  delete old_text;

  impl->text_window->set_anchor(this, 0);
  impl->text_window->set_position(1, 1);
  impl->text_window->set_scrollbar(false);
  impl->text_window->set_enabled(false);

  text_height = impl->text_window->get_text_height();
  if (text_height > impl->max_text_height) {
    impl->height = impl->max_text_height + 4;
    impl->text_window->set_scrollbar(true);
    impl->text_window->set_enabled(true);
  } else if (text_height == 1) {
    text_coordinate_t coord(0, INT_MAX);
    impl->height = 5;
    impl->text_window->set_size(1, text->calculate_screen_pos(&coord, 0));
    impl->text_window->set_anchor(this,
                                  T3_PARENT(T3_ANCHOR_TOPCENTER) | T3_CHILD(T3_ANCHOR_TOPCENTER));
    impl->text_window->set_position(1, 0);
  } else {
    impl->height = text_height + 4;
  }
  impl->text_window->set_size(impl->height - 4, None);
  set_size(impl->height, None);
  update_contents();
}

bool message_dialog_t::process_key(key_t key) {
  if (key > 0x20 && key < 0x10ffff) {
    key |= EKEY_META;
  }
  return dialog_t::process_key(key);
}

connection_t message_dialog_t::connect_activate(std::function<void()> _slot, size_t idx) {
  if (idx > widgets.size() - 1) {
    return connection_t();
  }
  if (idx == 0) {
    return activate_internal.connect(_slot);
  }
  return static_cast<button_t *>(widgets[idx + 1])->connect_activate(_slot);
}

void message_dialog_t::set_max_text_height(int max) { impl->max_text_height = max; }

}  // namespace
