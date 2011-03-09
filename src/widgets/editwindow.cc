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

#include "editwindow.h"
#include "util.h"
#include "colorscheme.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

goto_dialog_t edit_window_t::goto_dialog;
sigc::connection edit_window_t::goto_connection;

const char *edit_window_t::insstring[] = {"INS", "OVR"};
bool (text_buffer_t::*edit_window_t::proces_char[])(key_t) = { &text_buffer_t::insert_char, &text_buffer_t::overwrite_char};

edit_window_t::edit_window_t(container_t *parent, text_buffer_t *_text) : widget_t(parent, 10, 10) {
	if ((bottomlinewin = t3_win_new(parent->get_draw_window(), 1, 11, 0, 0, 0)) == NULL) {
		t3_win_del(window);
		throw bad_alloc();
	}
	t3_win_set_anchor(bottomlinewin, window, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	scrollbar = new scrollbar_t(parent, true);
	scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	scrollbar->set_size(10, None);

	if (_text == NULL)
		text = new text_buffer_t();
	else
		text = _text;

	text->window = this;
	if (text->get_wrap())
		text->rewrap();

	t3_win_set_default_attrs(window, colors.text_attrs);
	t3_win_set_default_attrs(bottomlinewin, colors.menubar_attrs);
	screen_pos = 0;
	hard_cursor = (text->selection_mode == selection_mode_t::NONE && colors.attr_cursor == 0) ||
			(text->selection_mode != selection_mode_t::NONE && colors.attr_selection_cursor == 0);
	focus = 0;
	need_repaint = true;

	t3_win_show(bottomlinewin);
}

edit_window_t::~edit_window_t(void) {
/* 	#ifdef DEBUG
	text->dump_undo();
	#endif */
	//~ unshow_file();
	t3_win_del(window);
	t3_win_del(bottomlinewin);
	delete scrollbar;
	//FIXME: implement proper clean-up
}

void edit_window_t::set_text_file(text_buffer_t *_text) {
	if (text == _text)
		return;

/*	unshow_file();
	if (_text->window != NULL) {
		text->window = NULL;
		text = NULL;
		_text->window->next_buffer();
		if (_text->window != NULL) {
			text_buffer_t *replacement = new text_buffer_t();
			_text->window->set_text_file(replacement);
		}
	}*/

	text = _text;
	text->window = this;
	if (text->get_wrap())
		text->rewrap();
	ensure_cursor_on_screen();
	need_repaint = true;
}

bool edit_window_t::set_size(optint height, optint width) {
	if (width > t3_win_get_width(window) || height > t3_win_get_height(window) + 1)
		need_repaint = true;

	if (!t3_win_resize(window, height - 1, width - 1))
		return false;
	if (!t3_win_resize(bottomlinewin, 1, width))
		return false;
	if (!scrollbar->set_size(height - 1, None))
		return false;

	t3_win_set_paint(bottomlinewin, 0, 0);
	t3_win_addchrep(bottomlinewin, ' ', colors.dialog_attrs, width);

	if (text->get_wrap()) {
		text->rewrap();
		need_repaint = true;
	}
	ensure_cursor_on_screen();
	return true;
}


void edit_window_t::ensure_cursor_on_screen(void) {
	int width;

	if (text->cursor.pos == text->get_line_max(text->cursor.line))
		width = 1;
	else
		width = text->width_at_cursor();

	screen_pos = text->calculate_screen_pos();

	if (text->cursor.line < text->topleft.line) {
		text->topleft.line = text->cursor.line;
		need_repaint = true;
	}

	if (text->cursor.line >= text->topleft.line + t3_win_get_height(window)) {
		text->topleft.line = text->cursor.line - t3_win_get_height(window) + 1;
		need_repaint = true;
	}

	if (screen_pos < text->topleft.pos) {
		text->topleft.pos = screen_pos;
		need_repaint = true;
	}

	if (screen_pos + width > text->topleft.pos + t3_win_get_width(window) - 1) {
		text->topleft.pos = screen_pos + width - t3_win_get_width(window) + 1;
		need_repaint = true;
	}
}

bool edit_window_t::find(const string *what, int flags, const text_line_t *replacement) {
	int start_screen_pos;

	if (!text->find(what, flags, replacement))
		return false;

	start_screen_pos = text->calculate_screen_pos(&text->selection_start);
	if (text->topleft.pos > start_screen_pos) {
		text->topleft.pos = start_screen_pos;
		need_repaint = true;
	}
	if (text->topleft.line > text->selection_start.line) {
		text->topleft.line = text->selection_start.line;
		need_repaint = true;
	}
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
	return true;
}

void edit_window_t::replace(void) {
	text->replace();
	reset_selection();
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

void edit_window_t::repaint_screen(void) {
	text_coordinate_t current_start, current_end;
	text_line_t::paint_info_t info;
	int i;

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();

	if (current_end.line < current_start.line || (current_end.line == current_start.line &&
			current_end.pos < current_start.pos)) {
		current_start = current_end;
		current_end = text->get_selection_start();
	}

	info.leftcol = text->topleft.pos;
	info.size = t3_win_get_width(window) - 1;
	info.normal_attr = 0;
	info.selected_attr = colors.text_selected_attrs;
	for (i = 0; i < t3_win_get_height(window) && (i + text->topleft.line) < text->get_used_lines(); i++) {
		t3_win_set_paint(window, i, 0);

		info.selection_start = text->topleft.line + i == current_start.line ? current_start.pos : -1;
		if (text->topleft.line + i >= current_start.line) {
			if (text->topleft.line + i < current_end.line)
				info.selection_end = INT_MAX;
			else if (text->topleft.line + i == current_end.line)
				info.selection_end = current_end.pos;
			else
				info.selection_end = -1;
		} else {
			info.selection_end = -1;
		}

		info.cursor = text->topleft.line + i == text->cursor.line && !hard_cursor ? text->cursor.pos : -1;
		text->paint_line(window, text->topleft.line + i, &info);
	}

	t3_win_clrtobot(window);
}

void edit_window_t::inc_x(void) {
	if (text->cursor.pos == text->get_line_max(text->cursor.line)) {
		if (text->cursor.line >= text->get_used_lines() - 1)
			return;

		text->cursor.line++;
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, 0);
	} else {
		text->adjust_position(1);
	}
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

void edit_window_t::next_word(void) {
	text->get_next_word();
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

void edit_window_t::dec_x(void) {
	if (screen_pos == 0) {
		if (text->cursor.line == 0)
			return;

		text->cursor.line--;
		text->cursor.pos = text->get_line_max(text->cursor.line);
	} else {
		text->adjust_position(-1);
	}
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

void edit_window_t::previous_word(void) {
	text->get_previous_word();
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

void edit_window_t::inc_y(void) {
	if (text->cursor.line + 1 < text->get_used_lines()) {
		text->cursor.line++;
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);
		ensure_cursor_on_screen();
	} else {
		text->cursor.pos = text->get_line_max(text->cursor.line);
		ensure_cursor_on_screen();
		text->last_set_pos = screen_pos;
	}
}

void edit_window_t::dec_y(void) {
	if (text->cursor.line > 0) {
		text->cursor.line--;
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);
		ensure_cursor_on_screen();
	} else {
		text->last_set_pos = text->cursor.pos = 0;
		ensure_cursor_on_screen();
	}
}

void edit_window_t::pgdn(void) {
	bool need_adjust = true;

	if (text->cursor.line + t3_win_get_height(window) - 1 < text->get_used_lines()) {
		text->cursor.line += t3_win_get_height(window) - 1;
	} else {
		text->cursor.line = text->get_used_lines() - 1;
		text->cursor.pos = text->get_line_max(text->cursor.line);
		need_adjust = false;
	}

	/* If the end of the text is already on the screen, don't change the top line. */
	if (text->topleft.line + t3_win_get_height(window) < text->get_used_lines()) {
		text->topleft.line += t3_win_get_height(window) - 1;
		if (text->topleft.line + t3_win_get_height(window) > text->get_used_lines())
			text->topleft.line = text->get_used_lines() - t3_win_get_height(window);
		need_repaint = true;
	}

	if (need_adjust)
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);

	ensure_cursor_on_screen();

	if (!need_adjust)
		text->last_set_pos = screen_pos;
}

void edit_window_t::pgup(void) {
	bool need_adjust = true;

	if (text->topleft.line < t3_win_get_height(window) - 1) {
		if (text->topleft.line != 0) {
			need_repaint = true;
			text->topleft.line = 0;
		}

		if (text->cursor.line < t3_win_get_height(window) - 1) {
			text->cursor.line = 0;
			text->last_set_pos = text->cursor.pos = 0;
			need_adjust = false;
		} else {
			text->cursor.line -= t3_win_get_height(window) - 1;
		}
	} else {
		text->cursor.line -= t3_win_get_height(window) - 1;
		text->topleft.line -= t3_win_get_height(window) - 1;
		need_repaint = true;
	}

	if (need_adjust)
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);

	ensure_cursor_on_screen();
}

void edit_window_t::reset_selection(void) {
	text->selection_mode = selection_mode_t::NONE;
	text->set_selection_start(0, -1);
	text->set_selection_end(0, -1);
	need_repaint = true;
}

void edit_window_t::set_selection_mode(key_t key) {
	switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
		case EKEY_END:
		case EKEY_HOME:
		case EKEY_PGUP:
		case EKEY_PGDN:
		case EKEY_LEFT:
		case EKEY_RIGHT:
		case EKEY_UP:
		case EKEY_DOWN:
			if ((text->selection_mode == selection_mode_t::SHIFT || text->selection_mode == selection_mode_t::ALL) && !(key & EKEY_SHIFT)) {
				reset_selection();
			} else if ((text->selection_mode == selection_mode_t::NONE || text->selection_mode == selection_mode_t::ALL) &&
					(key & EKEY_SHIFT)) {
				text->set_selection_start(text->cursor.line, text->cursor.pos);
				text->set_selection_end(text->cursor.line, text->cursor.pos);
				text->selection_mode = selection_mode_t::SHIFT;
			}
			break;
		default:
			break;
	}
}

void edit_window_t::delete_selection(void) {
	text_coordinate_t current_start, current_end;

	text->delete_selection();

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();

	if ((current_end.line < current_start.line) || (current_end.line == current_start.line && current_end.pos < current_start.pos))
		text->cursor = current_end;
	else
		text->cursor = current_start;

	need_repaint = true;
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
	reset_selection();
}

//FIXME: make every action into a separate function for readability
bool edit_window_t::process_key(key_t key) {
	set_selection_mode(key);

	switch (key) {
		case EKEY_RIGHT | EKEY_SHIFT:
		case EKEY_RIGHT:
			inc_x();
			break;
		case EKEY_RIGHT | EKEY_CTRL:
		case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
			next_word();
			break;
		case EKEY_LEFT | EKEY_SHIFT:
		case EKEY_LEFT:
			dec_x();
			break;
		case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_LEFT | EKEY_CTRL:
			previous_word();
			break;
		case EKEY_DOWN | EKEY_SHIFT:
		case EKEY_DOWN:
			inc_y();
			break;
		case EKEY_UP | EKEY_SHIFT:
		case EKEY_UP:
			dec_y();
			break;
		case EKEY_PGUP | EKEY_SHIFT:
		case EKEY_PGUP:
			pgup();
			break;
		case EKEY_PGDN | EKEY_SHIFT:
		case EKEY_PGDN:
			pgdn();
			break;
		case EKEY_HOME | EKEY_SHIFT:
		case EKEY_HOME:
			screen_pos = text->last_set_pos = 0;
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, 0);
			if (text->topleft.pos != 0)
				ensure_cursor_on_screen();
			break;
		case EKEY_HOME | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_HOME | EKEY_CTRL:
			screen_pos = text->last_set_pos = text->cursor.pos = 0;
			text->cursor.line = 0;
			if (text->topleft.pos != 0 || text->topleft.line != 0)
				ensure_cursor_on_screen();
			break;
		case EKEY_END | EKEY_SHIFT:
		case EKEY_END:
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			text->last_set_pos = screen_pos;
			break;
		case EKEY_END | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_END | EKEY_CTRL: {
			text->cursor.line = text->get_used_lines() - 1;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			text->last_set_pos = screen_pos;
			break;
		}
		case EKEY_INS:
			text->ins_mode ^= 1;
			break;

		/* Below this line all the keys modify the text. */
		case EKEY_DEL:
			if (text->selection_mode == selection_mode_t::NONE) {
				if (text->cursor.pos != text->get_line_max(text->cursor.line)) {
					text->delete_char();
					if (text->get_wrap())
						ensure_cursor_on_screen();
					need_repaint = true;
				} else if (text->cursor.line + 1 < text->get_used_lines()) {
					text->merge(false);
					if (text->get_wrap())
						ensure_cursor_on_screen();
					need_repaint = true;
				}
			} else {
				delete_selection();
			}
			break;

		case EKEY_NL:
			if (text->selection_mode != selection_mode_t::NONE)
				delete_selection();

			if (text->cursor.pos == text->get_line_max(text->cursor.line))
				text->new_line();
			else
				text->break_line();

			text->cursor.line++;
			text->cursor.pos = 0;
			ensure_cursor_on_screen();
			text->last_set_pos = 0;
			need_repaint = true;
			break;

		case EKEY_BS:
			if (text->selection_mode == selection_mode_t::NONE) {
				if (text->cursor.pos <= text->get_line_max(text->cursor.line)) {
					if (text->cursor.pos != 0) {
						text->backspace_char();
						need_repaint = true;
					} else if (text->cursor.line != 0) {
						text->merge(true);
						need_repaint = true;
					}
				} else {
					ASSERT(0);
				}
				ensure_cursor_on_screen();
				text->last_set_pos = screen_pos;
			} else {
				delete_selection();
			}
			break;
/*
		case EKEY_CTRL | 'q':
			executeAction(ActionID::FILE_EXIT);
			break;
		case EKEY_CTRL | 'o':
			executeAction(ActionID::FILE_OPEN);
			break;
*/
		case EKEY_CTRL | 'c':
		case EKEY_INS | EKEY_CTRL:
			cut_copy(false);
			break;
		case EKEY_CTRL | 'x':
		case EKEY_DEL | EKEY_SHIFT:
			cut_copy(true);
			break;
		case EKEY_CTRL | 'v':
		case EKEY_INS | EKEY_SHIFT:
			paste();
			break;

		case EKEY_CTRL | 'y':
			redo();
			break;
		case EKEY_CTRL | 'z':
			undo();
			break;

		case EKEY_CTRL | 'a':
			select_all();
			break;

		case EKEY_CTRL | 'g':
			goto_connection.disconnect();
			goto_connection = goto_dialog.connect_activate(sigc::mem_fun(this, &edit_window_t::goto_line));
			goto_dialog.set_position(t3_win_get_abs_y(window) + t3_win_get_height(window) / 2,
				t3_win_get_abs_x(window) + t3_win_get_width(window) / 2);
			goto_dialog.show();
			break;

		case 0: //CTRL-SPACE (and others)
			switch (text->selection_mode) {
				case selection_mode_t::MARK:
					reset_selection();
					break;
				case selection_mode_t::NONE:
				case selection_mode_t::ALL:
					text->set_selection_start(text->cursor.line, text->cursor.pos);
					text->set_selection_end(text->cursor.line, text->cursor.pos);
				/* FALLTHROUGH */
				case selection_mode_t::SHIFT:
					text->selection_mode = selection_mode_t::MARK;
					break;
				default:
					/* Should not happen, but just try to get back to a sane state. */
					reset_selection();
					break;
			}
			break;
		case EKEY_ESC:
			if (text->selection_mode == selection_mode_t::MARK)
				reset_selection();
			break;
/*
		case EKEY_CTRL | 's':
			//executeAction(FILE_SAVE);
			save(); // Spare the trip through all the components, and just call save here
			break;

		case EKEY_F6:
			next_buffer();
			break;
		case EKEY_F6 | EKEY_SHIFT:
			previous_buffer();
			break;

		case EKEY_CTRL | 'n':
			executeAction(ActionID::FILE_NEW);
			break;

		case EKEY_CTRL | 'f':
			executeAction(ActionID::SEARCH_SEARCH);
			break;
		case EKEY_CTRL | 'r':
			executeAction(ActionID::SEARCH_REPLACE);
			break;
		case EKEY_F3:
			executeAction(ActionID::SEARCH_AGAIN);
			break;
		case EKEY_F3 | EKEY_SHIFT:
			executeAction(ActionID::SEARCH_AGAIN_BACKWARD);
			break;

		case EKEY_CTRL | 'w':
			executeAction(ActionID::FILE_CLOSE);
			break;
*/
		case EKEY_F9:
			insert_char_dialog.set_position(t3_win_get_abs_y(window) + t3_win_get_height(window) / 2,
				t3_win_get_abs_x(window) + t3_win_get_width(window) / 2);
			insert_char_dialog.show();
			break;

		default:
			if (key < 32 && key != '\t')
				break;

			key &= ~EKEY_PROTECT;
			if (key == 10)
				break;

			if (key < 0x110000) {
				int local_insmode = text->ins_mode;
				if (text->selection_mode != selection_mode_t::NONE) {
					delete_selection();
					local_insmode = 0;
				}

				if (text->cursor.pos == text->get_line_max(text->cursor.line))
					text->append_char(key);
				else
					(text->*proces_char[local_insmode])(key);
				ensure_cursor_on_screen();
				need_repaint = true;
				text->last_set_pos = screen_pos;
			}
			break;
	}
	return true;
}

void edit_window_t::update_contents(void) {
	text_coordinate_t logical_cursor_pos;
	char info[30];
	int info_width, name_width;
	text_line_t::paint_info_t paint_info;

	if (text->selection_mode != selection_mode_t::NONE && text->selection_mode != selection_mode_t::ALL) {
		text->set_selection_end(text->cursor.line, text->cursor.pos);

		if (text->selection_mode == selection_mode_t::SHIFT) {
			if (text->selection_empty())
				reset_selection();
		}
	}

	hard_cursor = (text->selection_mode == selection_mode_t::NONE && colors.attr_cursor == 0) ||
			(text->selection_mode != selection_mode_t::NONE && colors.attr_selection_cursor == 0);

	//FIXME: don't want to fully repaint on every key when selecting!!
	if (need_repaint || text->selection_mode != selection_mode_t::NONE || !hard_cursor) {
		need_repaint = false;
		repaint_screen();
	}

	scrollbar->set_parameters(max(text->get_used_lines(), text->topleft.line + t3_win_get_height(window)),
		text->topleft.line, t3_win_get_height(window));
	scrollbar->update_contents();

	text->get_line_info(&logical_cursor_pos);
	snprintf(info, 29, "L: %-4d C: %-4d %c %s", logical_cursor_pos.line + 1, logical_cursor_pos.pos + 1, text->is_modified() ? '*' : ' ', insstring[text->ins_mode]);
	info_width = t3_term_strwidth(info);
	name_width = t3_win_get_width(bottomlinewin) - info_width - 3;

	/* FIXME: is it really necessary to do this on each key stroke??? */
	t3_win_set_paint(bottomlinewin, 0, 0);
	if (text->name_line->calculate_screen_width(0, text->name_line->get_length(), 1) > name_width) {
		t3_win_addstr(bottomlinewin, "..", colors.dialog_attrs);
		paint_info.start = text->name_line->adjust_position(text->name_line->get_length(), -(name_width - 2));
		paint_info.size = name_width - 2;
	} else {
		paint_info.start = 0;
		paint_info.size = name_width;
	}
	paint_info.leftcol = 0;
	paint_info.max = INT_MAX;
	paint_info.tabsize = 1;
	paint_info.flags = text_line_t::TAB_AS_CONTROL | text_line_t::SPACECLEAR;
	paint_info.selection_start = -1;
	paint_info.selection_end = -1;
	paint_info.cursor = -1;
	paint_info.normal_attr = 0;
	paint_info.selected_attr = 0;

	text->name_line->paint_line(bottomlinewin, &paint_info);

	t3_win_set_paint(bottomlinewin, 0, t3_win_get_width(bottomlinewin) - strlen(info) - 1);
	t3_win_addstr(bottomlinewin, info, colors.dialog_attrs);
	if (focus) {
		if (hard_cursor) {
			t3_win_set_cursor(window, text->cursor.line - text->topleft.line, screen_pos - text->topleft.pos);
			t3_term_show_cursor();
		} else {
			t3_term_hide_cursor();
		}
	}
}

void edit_window_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		if (hard_cursor) {
			t3_win_set_cursor(window, text->cursor.line - text->topleft.line, screen_pos - text->topleft.pos);
			t3_term_show_cursor();
		} else {
			repaint_screen(); //Only for removing cursor
		}
	}
}

t3_window_t *edit_window_t::get_draw_window(void) {
	return window;
}

/* void edit_window_t::next_buffer(void) {
	OpenFiles::iterator current;
	for (current = openFiles.begin(); current != openFiles.end() && *current != text; current++) {}

	for (OpenFiles::iterator iter = current; iter != openFiles.end(); iter++) {
		if ((*iter)->window == NULL) {
			set_text_file(*iter);
			return;
		}
	}

	for (OpenFiles::iterator iter = openFiles.begin(); iter != current; iter++) {
		if ((*iter)->window == NULL) {
			set_text_file(*iter);
			return;
		}
	}
}

void edit_window_t::previous_buffer(void) {
	OpenFiles::reverse_iterator current;
	for (current = openFiles.rbegin(); current != openFiles.rend() && *current != text; current++) {}

	for (OpenFiles::reverse_iterator iter = current; iter != openFiles.rend(); iter++) {
		if ((*iter)->window == NULL) {
			set_text_file(*iter);
			return;
		}
	}

	for (OpenFiles::reverse_iterator iter = openFiles.rbegin(); iter != current; iter++) {
		if ((*iter)->window == NULL) {
			set_text_file(*iter);
			return;
		}
	}
} */

void edit_window_t::get_dimensions(int *height, int *width, int *top, int *left) {
	*height = t3_win_get_height(window) + 1;
	*width = t3_win_get_width(window);
	*top = t3_win_get_y(window);
	*left = t3_win_get_x(window);
}

/*void edit_window_t::unshow_file(void) {
	text->window = NULL;
	if (text->name == NULL && !text->is_modified() && is_backup_file) {
		OpenFiles::iterator current;
		for (current = openFiles.begin(); current != openFiles.end() && *current != text; current++) {}
		openFiles.erase(current);
		delete text;
	}
}*/

//FIXME split and remove
void edit_window_t::undo(void) {
	if (text->apply_undo() == 0) {
		need_repaint = true;
		ensure_cursor_on_screen();
		text->last_set_pos = screen_pos;
	}
}

void edit_window_t::redo(void) {
	if (text->apply_redo() == 0) {
		need_repaint = true;
		ensure_cursor_on_screen();
		text->last_set_pos = screen_pos;
	}
}

void edit_window_t::cut_copy(bool cut) {
	if (text->selection_mode != selection_mode_t::NONE) {
		if (text->selection_empty()) {
			reset_selection();
			return;
		}

		if (copy_buffer != NULL)
			delete copy_buffer;

		copy_buffer = text->convert_selection();
/*		#ifdef DEBUG
		lprintf("Copy buffer contents: '");
		ldumpstr(copy_buffer->getData()->data(), copy_buffer->get_length());
		lprintf("'\n");
		#else
		lprintf("Copy buffer contents ommited because not compiled in debug mode\n");
		#endif*/

		if (cut)
			delete_selection();
		else if (text->selection_mode == selection_mode_t::MARK)
			reset_selection();
	}
}

void edit_window_t::paste(void) {
	if (copy_buffer != NULL) {
		if (text->selection_mode == selection_mode_t::NONE) {
			text->insert_block(copy_buffer);
		} else {
			text->replace_selection(copy_buffer);
			reset_selection();
		}
		ensure_cursor_on_screen();
		text->last_set_pos = screen_pos;
		need_repaint = true;
	}
}

void edit_window_t::select_all(void) {
	text->selection_mode = selection_mode_t::ALL;
	text->set_selection_start(0, 0);
	text->set_selection_end(text->get_used_lines() - 1, text->get_line_max(text->get_used_lines() - 1));
	need_repaint = true;
}
/*
void edit_window_t::save(void) {
	//FIXME: check mem alloc errors
	(*new SaveState(text))();
}

void edit_window_t::save_as(const string *name, Encoding *encoding) {
	//FIXME: check mem alloc errors (also strdup!)
	(*new SaveState(text, strdup(name->c_str()), encoding))();
}

void edit_window_t::close(bool force) {
	OpenFiles::iterator current, closed;

	if (text->is_modified() && !force) {
		activate_window(WindowID::CLOSE_CONFIRM, text->name);
		return;
	}

	for (current = openFiles.begin(); current != openFiles.end() && *current != text; current++) {}
	recentFiles.push_front(*current);
	closed = openFiles.erase(current);

	for (current = closed; current != openFiles.end() && (*current)->window != NULL; current++) {}
	//FIXME: go back, rather than to front if we reached the end
	if (current == openFiles.end())
		for (current = openFiles.begin(); current != closed && (*current)->window != NULL; current++) {}
	if (current != openFiles.end() && (*current)->window == NULL) {
		(*current)->window = this;
		this->text = *current;
	} else {
		text = new text_buffer_t();
		text->window = this;
		is_backup_file = true;
	}
	ensure_cursor_on_screen();
	need_repaint = true;
}
*/
void edit_window_t::goto_line(int line) {
	if (line < 1)
		return;

	text->cursor.line = (line > text->get_used_lines() ? text->get_used_lines() : line) - 1;
	if (text->cursor.pos > text->get_line_max(text->cursor.line))
		text->cursor.pos = text->get_line_max(text->cursor.line);
	text->last_set_pos = text->cursor.pos;
	ensure_cursor_on_screen();
}

bool edit_window_t::get_selection_lines(int *top, int *bottom) {
	if (text->selection_mode == selection_mode_t::NONE)
		return false;
	*top = text->selection_start.line - text->topleft.line;
	*bottom = text->selection_end.line - text->topleft.line;
	return true;
}

const text_buffer_t *edit_window_t::get_text_file(void) {
	return text;
}

}; // namespace
