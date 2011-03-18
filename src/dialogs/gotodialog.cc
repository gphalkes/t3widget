/* Copyright (C) 2010 G.P. Halkes
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
#include "dialogs/gotodialog.h"
#include "widgets/button.h"
#include "widgets/smartlabel.h"
#include "main.h"

#define GOTO_DIALOG_WIDTH 30
#define GOTO_DIALOG_HEIGHT 4

namespace t3_widget {

static key_t accepted_keys[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

goto_dialog_t::goto_dialog_t(void) :
	dialog_t(GOTO_DIALOG_HEIGHT, GOTO_DIALOG_WIDTH, "Goto Line")
{
	smart_label_t *number_label;
	button_t *ok_button, *cancel_button;

	number_label = new smart_label_t(this, "_Goto;gG", true);
	number_label->set_position(1, 2);
	number_line = new text_field_t(this);
	number_line->set_anchor(number_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	number_line->set_position(0, 1);
	number_line->set_size(None, GOTO_DIALOG_WIDTH - number_label->get_width() - 5);
	number_line->set_label(number_label);
	number_line->connect_activate(sigc::mem_fun(this, &goto_dialog_t::ok_activate));
	number_line->set_key_filter(accepted_keys, sizeof(accepted_keys) / sizeof(accepted_keys[0]), true);

	cancel_button = new button_t(this, "_Cancel;cC", false);
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);

	cancel_button->connect_activate(sigc::mem_fun(this, &goto_dialog_t::hide));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &goto_dialog_t::focus_previous));
	/* Nasty trick: registering a callback twice will call the callback twice. We need to do
	   FOCUS_PREVIOUS twice here to emulate moving up, because the ok_button is in the way. */
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &goto_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &goto_dialog_t::focus_previous));
	ok_button = new button_t(this, "_OK;oO", true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);

	ok_button->connect_activate(sigc::mem_fun(this, &goto_dialog_t::ok_activate));
	ok_button->connect_move_focus_up(sigc::mem_fun(this, &goto_dialog_t::focus_previous));
	ok_button->connect_move_focus_right(sigc::mem_fun(this, &goto_dialog_t::focus_next));

	widgets.push_back(number_label);
	widgets.push_back(number_line);
	widgets.push_back(ok_button);
	widgets.push_back(cancel_button);
}

bool goto_dialog_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void goto_dialog_t::show() {
	dialog_t::show();
	number_line->set_text("");
}

void goto_dialog_t::ok_activate(void) {
	hide();
	activate(atoi(number_line->get_text()->c_str()));
}

}; // namespace
