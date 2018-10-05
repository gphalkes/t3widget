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
#include "t3widget/dialogs/gotodialog.h"
#include "t3widget/main.h"
#include "t3widget/widgets/button.h"
#include "t3widget/widgets/smartlabel.h"

#define GOTO_DIALOG_WIDTH 30
#define GOTO_DIALOG_HEIGHT 4

namespace t3widget {

static key_t accepted_keys[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

struct goto_dialog_t::implementation_t {
  text_field_t *number_line;
  signal_t<text_pos_t> activate;
};

goto_dialog_t::goto_dialog_t()
    : dialog_t(GOTO_DIALOG_HEIGHT, GOTO_DIALOG_WIDTH, "Goto Line", impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>()) {
  smart_label_t *number_label = emplace_back<smart_label_t>("_Goto", true);
  number_label->set_position(1, 2);
  impl->number_line = emplace_back<text_field_t>();
  impl->number_line->set_anchor(number_label,
                                T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->number_line->set_position(0, 1);
  impl->number_line->set_size(None, GOTO_DIALOG_WIDTH - number_label->get_width() - 5);
  impl->number_line->set_label(number_label);
  impl->number_line->connect_activate([this] { ok_activate(); });
  impl->number_line->set_key_filter(accepted_keys, sizeof(accepted_keys) / sizeof(accepted_keys[0]),
                                    true);

  button_t *ok_button = emplace_back<button_t>("_OK", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel", false);

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);

  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });
  /* Nasty trick: registering a callback twice will call the callback twice. We need to do
     FOCUS_PREVIOUS twice here to emulate moving up, because the ok_button is in the way. */
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });

  ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);

  ok_button->connect_activate([this] { ok_activate(); });
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
}

goto_dialog_t::~goto_dialog_t() {}

bool goto_dialog_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void goto_dialog_t::reset() { impl->number_line->set_text(""); }

void goto_dialog_t::ok_activate() {
  hide();
  impl->activate(static_cast<text_pos_t>(std::atoll(impl->number_line->get_text().c_str())));
}

connection_t goto_dialog_t::connect_activate(std::function<void(text_pos_t)> cb) {
  return impl->activate.connect(cb);
}

}  // namespace t3widget
