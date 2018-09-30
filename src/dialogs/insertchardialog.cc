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
#include <cstring>

#include "t3widget/dialogs/insertchardialog.h"
#include "t3widget/internal.h"
#include "t3widget/log.h"
#include "t3widget/main.h"
#include "t3widget/util.h"
#include "t3widget/widgets/button.h"

// FIXME: auto-complete list from previous entries
namespace t3widget {

#define INSERT_CHAR_DIALOG_WIDTH 30
#define INSERT_CHAR_DIALOG_HEIGHT 4

struct insert_char_dialog_t::implementation_t {
  text_field_t *description_line;
};

insert_char_dialog_t::insert_char_dialog_t()
    : dialog_t(INSERT_CHAR_DIALOG_HEIGHT, INSERT_CHAR_DIALOG_WIDTH, _("Insert Character"),
               impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>()) {
  smart_label_t *description_label = emplace_back<smart_label_t>("C_haracter", true);
  description_label->set_position(1, 2);
  impl->description_line = emplace_back<text_field_t>();
  impl->description_line->set_anchor(description_label,
                                     T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->description_line->set_position(0, 1);
  impl->description_line->set_size(1,
                                   INSERT_CHAR_DIALOG_WIDTH - description_label->get_width() - 5);
  impl->description_line->set_label(description_label);
  impl->description_line->connect_activate([this] { ok_activate(); });

  button_t *ok_button = emplace_back<button_t>("_OK", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel", false);

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });
  /* Nasty trick: registering a callback twice will call the callback twice. We need to do
     focus_previous twice here to emulate moving up, because the ok_button is in the way. */
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });

  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate([this] { ok_activate(); });
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
}

insert_char_dialog_t::~insert_char_dialog_t() {}

bool insert_char_dialog_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void insert_char_dialog_t::reset() { impl->description_line->set_text(""); }

key_t insert_char_dialog_t::interpret_key(const std::string &descr) {
  char codepoint[16];
  key_t result;
  int next;

  if (sscanf(descr.c_str(), " %*[uU]+%6[0-9a-fA-F]%n", codepoint, &next) >= 1) {
    if (descr.find_first_not_of(" \t", next) != std::string::npos) {
      return -1;
    }
    result = static_cast<key_t>(strtol(codepoint, nullptr, 16));
    if (result > 0x10FFFF) {
      return -1;
    }
    return result;
  } else if (sscanf(descr.c_str(), " \\%15[^ ]%n", codepoint, &next) >= 1) {
    if (descr.find_first_not_of(" \t", next) != std::string::npos) {
      return -1;
    }

    size_t readposition = 0;
    std::string error_message;
    result = parse_escape(codepoint, &error_message, readposition);
    if (result < 0 || readposition != strlen(codepoint)) {
      return -1;
    }
    if (result & ESCAPE_UNICODE) {
      return result & ~ESCAPE_UNICODE;
    }
    return result;
  }
  // FIXME: also accept Unicode names
  return -1;
}

void insert_char_dialog_t::ok_activate() {
  key_t key = interpret_key(impl->description_line->get_text());
  if (key >= 0) {
    hide();
    lprintf("Inserting key: %d\n", key);
    insert_protected_key(key);
  } else {
    std::string message = _("Invalid character description: '");
    message += impl->description_line->get_text().c_str();
    message += '\'';
    message_dialog->set_message(message);
    message_dialog->center_over(this);
    message_dialog->show();
  }
}

}  // namespace t3widget
