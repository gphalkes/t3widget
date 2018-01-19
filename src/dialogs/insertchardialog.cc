/* Copyright (C) 2011 G.P. Halkes
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

#include "dialogs/insertchardialog.h"
#include "internal.h"
#include "log.h"
#include "main.h"
#include "util.h"
#include "widgets/button.h"

// FIXME: auto-complete list from previous entries
namespace t3_widget {

#define INSERT_CHAR_DIALOG_WIDTH 30
#define INSERT_CHAR_DIALOG_HEIGHT 4

insert_char_dialog_t::insert_char_dialog_t()
    : dialog_t(INSERT_CHAR_DIALOG_HEIGHT, INSERT_CHAR_DIALOG_WIDTH, "Insert Character") {
  smart_label_t *description_label;
  button_t *ok_button, *cancel_button;

  description_label = new smart_label_t("C_haracter", true);
  description_label->set_position(1, 2);
  description_line = new text_field_t();
  description_line->set_anchor(description_label,
                               T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  description_line->set_position(0, 1);
  description_line->set_size(1, INSERT_CHAR_DIALOG_WIDTH - description_label->get_width() - 5);
  description_line->set_label(description_label);
  description_line->connect_activate(signals::mem_fun(this, &insert_char_dialog_t::ok_activate));

  cancel_button = new button_t("_Cancel", false);
  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate(signals::mem_fun(this, &insert_char_dialog_t::close));
  cancel_button->connect_move_focus_left(
      signals::mem_fun(this, &insert_char_dialog_t::focus_previous));
  /* Nasty trick: registering a callback twice will call the callback twice. We need to do
     focus_previous twice here to emulate moving up, because the ok_button is in the way. */
  cancel_button->connect_move_focus_up(
      signals::mem_fun(this, &insert_char_dialog_t::focus_previous));
  cancel_button->connect_move_focus_up(
      signals::mem_fun(this, &insert_char_dialog_t::focus_previous));
  ok_button = new button_t("_OK", true);
  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate(signals::mem_fun(this, &insert_char_dialog_t::ok_activate));
  ok_button->connect_move_focus_up(signals::mem_fun(this, &insert_char_dialog_t::focus_previous));
  ok_button->connect_move_focus_right(signals::mem_fun(this, &insert_char_dialog_t::focus_next));

  push_back(description_label);
  push_back(description_line);
  push_back(ok_button);
  push_back(cancel_button);
}

bool insert_char_dialog_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void insert_char_dialog_t::reset() { description_line->set_text(""); }

key_t insert_char_dialog_t::interpret_key(const std::string *descr) {
  char codepoint[16];
  key_t result;
  int next;

  if (sscanf(descr->c_str(), " %*[uU]+%6[0-9a-fA-F]%n", codepoint, &next) >= 1) {
    if (descr->find_first_not_of(" \t", next) != std::string::npos) return -1;
    result = (key_t)strtol(codepoint, nullptr, 16);
    if (result > 0x10FFFF) return -1;
    return result;
  } else if (sscanf(descr->c_str(), " \\%15[^ ]%n", codepoint, &next) >= 1) {
    if (descr->find_first_not_of(" \t", next) != std::string::npos) return -1;

    size_t readposition = 0;
    const char *error_message;
    result = parse_escape(codepoint, &error_message, readposition);
    if (result < 0 || readposition != strlen(codepoint)) return -1;
    if (result & ESCAPE_UNICODE) return result & ~ESCAPE_UNICODE;
    return result;
  }
  // FIXME: also accept Unicode names
  return -1;
}

void insert_char_dialog_t::ok_activate() {
  key_t key = interpret_key(description_line->get_text());
  if (key >= 0) {
    hide();
    lprintf("Inserting key: %d\n", key);
    insert_protected_key(key);
  } else {
    std::string message = _("Invalid character description: '");
    message += description_line->get_text()->c_str();
    message += '\'';
    message_dialog->set_message(&message);
    message_dialog->center_over(this);
    message_dialog->show();
  }
}

};  // namespace
