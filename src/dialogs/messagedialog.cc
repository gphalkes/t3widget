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
#include "main.h"
#include "dialogs/messagedialog.h"

using namespace std;

message_dialog_base_t::message_dialog_base_t(int width, int top, int left, const char *_title) : dialog_t(5, width, top, left, 0, _title) {
	int i;
	message = NULL;
	for (i = 0; i < MESSAGEDIALOG_MAX_LINES + 1; i++)
		break_positions[i] = INT_MAX;
}

void message_dialog_base_t::draw_dialog(void) {
	line_t::paint_info_t info;
	int i;

	dialog_t::draw_dialog();
	if (message == NULL)
		return;

	info.leftcol = 0;
	info.tabsize = 0;
	info.flags = line_t::SPACECLEAR | line_t::TAB_AS_CONTROL;
	info.selection_start = -1;
	info.selection_end = -1;
	info.cursor = -1;
	info.normal_attr = 0;
	info.selected_attr = 0;

	if (break_positions[1] == INT_MAX) {
		int message_width = message->calculate_screen_width(0, message->get_length(), 0 /* Tab as control */);
		info.start = 0;
		info.max = INT_MAX;
		info.size = message_width;
		t3_win_set_paint(window, 1, (t3_win_get_width(window) - message_width) / 2);
		message->paint_line(window, &info);
		return;
	}


	for (i = 0; i < MESSAGEDIALOG_MAX_LINES && break_positions[i] < INT_MAX; i++) {
		t3_win_set_paint(window, i + 1, 2);
		info.start = break_positions[i];
		info.max = break_positions[i + 1];
		info.size = t3_win_get_width(window) - 4;
		message->paint_line(window, &info);
	}
}

void message_dialog_base_t::set_message(const string *_message) {
	int i;
	if (message != NULL)
		delete message;
	message = new line_t(_message);
	break_positions[0] = 0;
	for (i = 1; i < MESSAGEDIALOG_MAX_LINES; i++) {
		break_pos_t brk = message->findNextBreakPos(break_positions[i - 1], t3_win_get_width(window) - 4, 0);
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


message_dialog_t::message_dialog_t(int width, int top, int left, const char *_title) : message_dialog_base_t(width, top, left, _title) {
	button = new button_t(this, "_OK;oO", true);
	button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	button->set_position(-1, (width - button->get_width()) / 2 );
	button->connect_activate(sigc::mem_fun(this, &message_dialog_t::hide));
	widgets.push_back(button);
}

bool message_dialog_t::set_size(optint _height, optint width) {
	bool result;
	result = message_dialog_base_t::set_size(_height, width);
	button->set_position(None, (t3_win_get_width(window) - button->get_width()) / 2);
	return result;
}

question_dialog_t::question_dialog_t(int width, int top, int left, const char *_title,
		const char *okName, const char *cancelName) : message_dialog_base_t(width, top, left, _title)
{
	ok_button = new button_t(this, okName);
	ok_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	ok_button->connect_activate(sigc::mem_fun(this, &question_dialog_t::hide));
	ok_button->connect_activate(ok.make_slot());
	cancel_button = new button_t(this, cancelName);
	cancel_button->set_anchor(ok_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	cancel_button->set_position(0, 2);
	cancel_button->connect_activate(sigc::mem_fun(this, &question_dialog_t::hide));
	cancel_button->connect_activate(cancel.make_slot());
	ok_button->set_position(-1, (width - ok_button->get_width() - cancel_button->get_width() - 2) / 2 );

	widgets.push_back(ok_button);
	widgets.push_back(cancel_button);
}

bool question_dialog_t::set_size(optint _height, optint width) {
	bool result;
	result = message_dialog_base_t::set_size(_height, width);
	ok_button->set_size(None, (width - ok_button->get_width() - cancel_button->get_width() - 2) / 2);
	return result;
}
