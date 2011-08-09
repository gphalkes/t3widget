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
#include <climits>
#include <cstring>
#include <cstdarg>
#include "main.h"
#include "dialogs/messagedialog.h"
#include "textline.h"

#define MESSAGEDIALOG_MAX_LINES 10

using namespace std;
namespace t3_widget {

message_dialog_t::message_dialog_t(int width, const char *_title, ...) : dialog_t(5, width, _title),
		max_text_height(MESSAGEDIALOG_MAX_LINES)
{
	va_list ap;
	button_t *button;
	const char *button_name;
	int total_width = 0;

	text_window = new text_window_t(NULL, false);
	text_window->set_size(1, width - 2);
	text_window->set_position(1, 1);
	text_window->connect_activate(activate_internal.make_slot());
	text_window->connect_activate(sigc::mem_fun(this, &message_dialog_t::hide));
	text_window->set_tabsize(0);
	text_window->set_enabled(false);

	push_back(text_window);

	va_start(ap, _title);
	while ((button_name = va_arg(ap, const char *)) != NULL) {
		if (widgets.size() == 1) {
			button = new button_t(button_name, true);
			button->connect_activate(activate_internal.make_slot());
			button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
		} else {
			button = new button_t(button_name);
			button->set_anchor(widgets.back(), T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
			button->set_position(0, 2);

			((button_t *) widgets.back())->connect_move_focus_right(sigc::mem_fun(this, &message_dialog_t::focus_next));
			button->connect_move_focus_left(sigc::mem_fun(this, &message_dialog_t::focus_previous));

			total_width += 2;
		}

		button->connect_activate(sigc::mem_fun(this, &message_dialog_t::hide));
		total_width += button->get_width();
		push_back(button);
	}

	widgets[1]->set_position(-1, -total_width / 2 );
}

message_dialog_t::~message_dialog_t(void) {
	delete text_window->get_text();
}

void message_dialog_t::set_message(const char *message, size_t length) {
	text_buffer_t *old_text;
	text_buffer_t *text = new text_buffer_t();
	int text_height;

	text_window->set_size(None, t3_win_get_width(window) - 2);

	text->append_text(message, length);
	old_text = text_window->get_text();
	text_window->set_text(text);
	delete old_text;

	text_window->set_anchor(this, 0);
	text_window->set_position(1, 1);
	text_window->set_scrollbar(false);
	text_window->set_enabled(false);

	text_height = text_window->get_text_height();
	if (text_height > max_text_height) {
		height = max_text_height + 3;
		text_window->set_scrollbar(true);
		text_window->set_enabled(true);
	} else if (text_height == 1) {
		text_coordinate_t coord(0, INT_MAX);
		height = 4;
		text_window->set_size(1, text->calculate_screen_pos(&coord, 0));
		text_window->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPCENTER) | T3_CHILD(T3_ANCHOR_TOPCENTER));
		text_window->set_position(1, 0);
	} else {
		height = text_height + 3;
	}
	text_window->set_size(height - 3, None);
	set_size(height, None);
	update_contents();
}

void message_dialog_t::set_message(const char *message) {
	set_message(message, strlen(message));
}

void message_dialog_t::set_message(const string *message) {
	set_message(message->data(), message->size());
}

bool message_dialog_t::process_key(key_t key) {
	if (key >= 0x20 && key < 0x10ffff)
		key |= EKEY_META;
	return dialog_t::process_key(key);
}

sigc::connection message_dialog_t::connect_activate(const sigc::slot<void> &_slot, size_t idx) {
	if (idx > widgets.size() - 1)
		return sigc::connection();
	if (idx == 0)
		return activate_internal.connect(_slot);
	return ((button_t *) widgets[idx + 1])->connect_activate(_slot);
}

void message_dialog_t::set_max_text_height(int max) {
	max_text_height = max;
}

}; // namespace
