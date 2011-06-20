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
#include <climits>
#include <cstring>
#include <cstdarg>
#include "main.h"
#include "dialogs/messagedialog.h"
#include "textline.h"

using namespace std;
namespace t3_widget {

message_dialog_t::message_dialog_t(int width, const char *_title, ...) : dialog_t(5, width, _title), total_width(0) {
	va_list ap;
	button_t *button;
	const char *button_name;

	for (int i = 0; i < _T3_WIDGET_MESSAGEDIALOG_MAX_LINES + 1; i++)
		break_positions[i] = INT_MAX;

	va_start(ap, _title);
	while ((button_name = va_arg(ap, const char *)) != NULL) {
		button = new button_t(button_name, widgets.empty());
		button->connect_activate(sigc::mem_fun(this, &message_dialog_t::close));
		total_width += button->get_width();
		if (widgets.empty()) {
			button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
		} else {
			button->set_anchor(widgets.back(), T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
			button->set_position(0, 2);
			((button_t *) widgets.back())->connect_move_focus_right(sigc::mem_fun(this, &message_dialog_t::focus_next));
			button->connect_move_focus_left(sigc::mem_fun(this, &message_dialog_t::focus_previous));
			total_width += 2;
		}
		push_back(button);
	}
	widgets.front()->set_position(-1, -total_width / 2 );
}

void message_dialog_t::draw_dialog(void) {
	text_line_t::paint_info_t info;
	int i;

	dialog_t::draw_dialog();

	info.leftcol = 0;
	info.tabsize = 0;
	info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL;
	info.selection_start = -1;
	info.selection_end = -1;
	info.cursor = -1;
	info.normal_attr = 0;
	info.selected_attr = 0;

	if (break_positions[1] == INT_MAX) {
		int message_width = message.calculate_screen_width(0, message.get_length(), 0 /* Tab as control */);
		info.start = 0;
		info.max = INT_MAX;
		info.size = message_width;
		t3_win_set_paint(window, 1, (t3_win_get_width(window) - message_width) / 2);
		message.paint_line(window, &info);
		return;
	}


	for (i = 0; i < _T3_WIDGET_MESSAGEDIALOG_MAX_LINES && break_positions[i] < INT_MAX; i++) {
		t3_win_set_paint(window, i + 1, 2);
		info.start = break_positions[i];
		info.max = break_positions[i + 1];
		info.size = t3_win_get_width(window) - 4;
		message.paint_line(window, &info);
	}
}

void message_dialog_t::set_message(const char *_message, size_t length) {
	int i;
	message.set_text(_message, length);
	break_positions[0] = 0;
	for (i = 1; i < _T3_WIDGET_MESSAGEDIALOG_MAX_LINES; i++) {
		break_pos_t brk = message.find_next_break_pos(break_positions[i - 1], t3_win_get_width(window) - 4, 0);
		if (brk.pos > 0) {
			break_positions[i] = brk.pos;
		} else {
			break_positions[i] = INT_MAX;
			break;
		}
	}
	height = i + 4;
	set_size(height, t3_win_get_width(window));
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);
	draw_dialog();
}

void message_dialog_t::set_message(const char *_message) {
	set_message(_message, strlen(_message));
}

void message_dialog_t::set_message(const string *_message) {
	set_message(_message->data(), _message->size());
}

bool message_dialog_t::process_key(key_t key) {
	if (key >= 0x20 && key < 0x10ffff)
		key |= EKEY_META;
	return dialog_t::process_key(key);
}

sigc::connection message_dialog_t::connect_activate(const sigc::slot<void> &_slot, size_t idx) {
	if (idx > widgets.size())
		return sigc::connection();
	return ((button_t *) widgets[idx])->connect_activate(_slot);
}

}; // namespace
