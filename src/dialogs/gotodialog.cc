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
#include "main.h"

#define GOTO_DIALOG_WIDTH 30
#define GOTO_DIALOG_HEIGHT 4

static key_t accepted_keys[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };


goto_dialog_t::goto_dialog_t(void) :
	dialog_t(GOTO_DIALOG_HEIGHT, GOTO_DIALOG_WIDTH, (screenLines - GOTO_DIALOG_HEIGHT) / 2,
		(screenColumns - GOTO_DIALOG_WIDTH) / 2, DIALOG_DEPTH - 10, "Goto line_t")
{
	smart_label_t *number_label;
	button_t *ok_button, *cancel_button;

	number_label = new smart_label_t(this, 1, 2, "_Goto;gG", true);
	number_line = new text_field_t(this, number_label, 0, 1, GOTO_DIALOG_WIDTH - number_label->get_width() - 5, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	number_line->set_label(number_label);
	number_line->set_callback(text_field_t::ENTER, this, OK);
	number_line->set_key_filter(accepted_keys, sizeof(accepted_keys) / sizeof(accepted_keys[0]), true);
	//FIXME: set key filter

	cancel_button = new button_t(this, this, -1, -1, -2, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT), "_Cancel;cC", false);
	cancel_button->set_callback(button_t::ENTER, this, CLOSE);
	cancel_button->set_callback(checkbox_t::MOVE_LEFT, this, FOCUS_PREVIOUS);
	/* Nasty trick: registering a callback twice will call the callback twice. We need to do
	   FOCUS_PREVIOUS twice here to emulate moving up, because the ok_button is in the way. */
	cancel_button->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	cancel_button->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	ok_button = new button_t(this, cancel_button, -1, 0, -2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT), "_OK;oO", true);
	ok_button->set_callback(button_t::ENTER, this, OK);
	ok_button->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	ok_button->set_callback(checkbox_t::MOVE_RIGHT, this, FOCUS_NEXT);

	components.push_back(number_label);
	components.push_back(number_line);
	components.push_back(ok_button);
	components.push_back(cancel_button);

	draw_dialog();
}

bool goto_dialog_t::resize(optint height, optint width, optint top, optint left) {
	(void) height;
	(void) width;
	(void) top;
	(void) left;
	return dialog_t::resize(None, None, (screenLines - GOTO_DIALOG_HEIGHT) /2, (screenColumns - GOTO_DIALOG_WIDTH) / 2);
}

void goto_dialog_t::set_show(bool show) {
	dialog_t::set_show(show);
	if (!show)
		number_line->set_text("");
}

void goto_dialog_t::callback(int action, const void *data) {
	switch (action) {
		case OK: {
			int value;
			value = atoi(number_line->get_text()->c_str());
			deactivate_window();
			goto_line(value);
			break;
		}
		default:
			dialog_t::callback(action, data);
			break;
	}
}
