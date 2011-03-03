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
		int message_width = message->calculateScreenWidth(0, message->get_length(), 0 /* Tab as control */);
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

bool message_dialog_base_t::resize(optint _height, optint width, optint top, optint left) {
	(void) _height;
	return dialog_t::resize(height, width, top, left);
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
	resize(height, t3_win_get_width(window), None, None);
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);
	draw_dialog();
}


message_dialog_t::message_dialog_t(int width, int top, int left, const char *_title) : message_dialog_base_t(width, top, left, _title) {
	button = new button_t(this, NULL, -1, -1, width / 2, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT), "_OK;oO", true);
	button->resize(None, None, None, (width - button->get_width()) / 2 );
	button->connect_activate(sigc::mem_fun(this, &message_dialog_t::hide));
	widgets.push_back(button);
}

bool message_dialog_t::resize(optint _height, optint width, optint top, optint left) {
	bool result;
	result = message_dialog_base_t::resize(_height, width, top, left);
	button->resize(None, None, None, (t3_win_get_width(window) - button->get_width()) / 2);
	return result;
}

question_dialog_t::question_dialog_t(int width, int top, int left, const char *_title,
		const char *okName, const char *cancelName) : message_dialog_base_t(width, top, left, _title)
{
	ok_button = new button_t(this, NULL, -1, -1, width / 2, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT), okName, true);
	ok_button->connect_activate(sigc::mem_fun(this, &question_dialog_t::hide));
	ok_button->connect_activate(ok.make_slot());
	cancel_button = new button_t(this, ok_button, -1, 0, 2, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), cancelName, true);
	cancel_button->connect_activate(sigc::mem_fun(this, &question_dialog_t::hide));
	cancel_button->connect_activate(cancel.make_slot());
	ok_button->resize(None, None, None, (width - ok_button->get_width() - cancel_button->get_width() - 2) / 2 );

	widgets.push_back(ok_button);
	widgets.push_back(cancel_button);
}

bool question_dialog_t::resize(optint _height, optint width, optint top, optint left) {
	bool result;
	result = message_dialog_base_t::resize(_height, width, top, left);
	ok_button->resize(None, None, None, (width - ok_button->get_width() - cancel_button->get_width() - 2) / 2);
	return result;
}
