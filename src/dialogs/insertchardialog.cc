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
#include <cstring>

#include "dialogs/insertchardialog.h"
#include "main.h"
#include "util.h"
#include "log.h"

//FIXME: auto-complete list from previous entries

#define INSERT_CHAR_DIALOG_WIDTH 30
#define INSERT_CHAR_DIALOG_HEIGHT 4

insert_char_dialog_t::insert_char_dialog_t(void) :
	dialog_t(INSERT_CHAR_DIALOG_HEIGHT, INSERT_CHAR_DIALOG_WIDTH, (screenLines - INSERT_CHAR_DIALOG_HEIGHT) / 2,
		(screenColumns - INSERT_CHAR_DIALOG_WIDTH) / 2, DIALOG_DEPTH - 10, "Insert Character")
{
	smart_label_t *description_label;
	button_t *ok_button, *cancel_button;

	description_label = new smart_label_t(this, 1, 2, "C_haracter;hH", true);
	description_line = new text_field_t(this, description_label, 0, 1, INSERT_CHAR_DIALOG_WIDTH - description_label->get_width() - 5, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	description_line->set_label(description_label);
	description_line->set_callback(text_field_t::ENTER, this, OK);

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

	components.push_back(description_label);
	components.push_back(description_line);
	components.push_back(ok_button);
	components.push_back(cancel_button);

	draw_dialog();
}

bool insert_char_dialog_t::resize(optint height, optint width, optint top, optint left) {
	(void) height;
	(void) width;
	(void) top;
	(void) left;
	return dialog_t::resize(None, None, (screenLines - INSERT_CHAR_DIALOG_HEIGHT) /2, (screenColumns - INSERT_CHAR_DIALOG_WIDTH) / 2);
}

void insert_char_dialog_t::set_show(bool show) {
	dialog_t::set_show(show);
	if (!show)
		description_line->set_text("");
}

key_t insert_char_dialog_t::interpret_key(const string *descr) {
	char codepoint[16];
	key_t result;
	int next;

	if (sscanf(descr->c_str(), " %*[uU]+%6[0-9a-fA-F]%n", codepoint, &next) >= 1) {
		if (descr->find_first_not_of(" \t", next) != string::npos)
			return -1;
		result = (key_t) strtol(codepoint, NULL, 16);
		if (result > 0x10FFFF)
			return -1;
		return result;
	} else if (sscanf(descr->c_str(), " \\%15[^ ]%n", codepoint, &next) >= 1) {
		if (descr->find_first_not_of(" \t", next) != string::npos)
			return -1;

		size_t readposition = 0;
		const char *error_message;
		result = parseEscape(codepoint, &error_message, readposition, strlen(codepoint));
		if (result < 0 || readposition != strlen(codepoint))
			return -1;
		if (result & ESCAPE_UNICODE)
			return result & ~ESCAPE_UNICODE;
		if (UTF8mode && result < 256 && result > 127)
			return -1;
		return result;
	}
	//FIXME: also accept Unicode names
	return -1;
}

void insert_char_dialog_t::callback(int action, const void *data) {
	switch (action) {
		case OK: {
			key_t key = interpret_key(description_line->get_text());
			if (key >= 0) {
				deactivate_window();
				lprintf("Inserting key: %d\n", key);
				insert_protected_key(key);
			} else {
				string message;
				printfInto(&message, "Invalid character description: '%s'", description_line->get_text()->c_str());
				deactivate_window();
				activate_window(WindowID::ERROR_DIALOG, &message);
			}
			break;
		}
		default:
			dialog_t::callback(action, data);
			break;
	}
}
