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
#include "dialogs/altmessagedialog.h"

alt_message_dialog_t::alt_message_dialog_t(const char *_message) :
	dialog_t(5, t3_term_strwidth(_message) + 4, screenLines / 3, (screenColumns - t3_term_strwidth(_message) - 4) / 2, 40, NULL),
	message(_message)
{
	components.push_back(new dummy_widget_t());
	draw_dialog();
}

void alt_message_dialog_t::draw_dialog(void) {
	dialog_t::draw_dialog();
	t3_win_set_paint(window, 2, 2);
	t3_win_addstr(window, message, 0);
}

bool alt_message_dialog_t::resize(optint height, optint width, optint top, optint left) {
	(void) height;
	(void) width;
	(void) top;
	(void) left;
	return dialog_t::resize(None, None, screenLines / 3, (screenColumns - t3_win_get_width(window)) / 2);
}

void alt_message_dialog_t::process_key(key_t key) {
	deactivate_window();
	if (key == EKEY_ESC)
		return;
	sendKey(key | EKEY_META);
}
