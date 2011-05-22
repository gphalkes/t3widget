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

#warning FIXME: last column of edit window is not used!
#warning FIXME: set name_line to a useful value

using namespace std;
namespace t3_widget {

goto_dialog_t *edit_window_t::goto_dialog;
sigc::connection edit_window_t::goto_connection;
find_dialog_t *edit_window_t::global_find_dialog;
sigc::connection edit_window_t::global_find_dialog_connection;
finder_t edit_window_t::global_finder;
replace_buttons_dialog_t *edit_window_t::replace_buttons;
sigc::connection edit_window_t::replace_buttons_connection;
bool edit_window_t::init_connected = connect_on_init(sigc::ptr_fun(edit_window_t::init));

const char *edit_window_t::ins_string[] = {"INS", "OVR"};
bool (text_buffer_t::*edit_window_t::proces_char[])(key_t) = { &text_buffer_t::insert_char, &text_buffer_t::overwrite_char};

void edit_window_t::init(void) {
	/* Construct these from t3_widget::init, such that the locale is set correctly and
	   gettext therefore returns the correctly localized strings. */
	goto_dialog = new goto_dialog_t();
	global_find_dialog = new find_dialog_t();
	replace_buttons = new replace_buttons_dialog_t();
}

edit_window_t::edit_window_t(text_buffer_t *_text) : edit_window(NULL), bottom_line_window(NULL), scrollbar(true),
		find_dialog(NULL), finder(NULL)
{
	init_unbacked_window(11, 11);
	if ((edit_window = t3_win_new(window, 10, 10, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(edit_window);

	if ((bottom_line_window = t3_win_new(window, 1, 11, 0, 0, 0)) == NULL) {
		t3_win_del(edit_window);
		throw bad_alloc();
	}
	t3_win_set_anchor(bottom_line_window, window, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	t3_win_show(bottom_line_window);

	set_widget_parent(&scrollbar);
	scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	scrollbar.set_size(10, None);

	if (_text == NULL)
		text = new text_buffer_t();
	else
		text = _text;

	text->window = this;
	if (text->get_wrap())
		text->rewrap();

	screen_pos = 0;
	focus = 0;

}

edit_window_t::~edit_window_t(void) {
	t3_win_del(edit_window);
	t3_win_del(bottom_line_window);
}

void edit_window_t::set_text(text_buffer_t *_text) {
	if (text == _text)
		return;

	text->window = NULL;
	text = _text;
	text->window = this;
	if (text->get_wrap())
		text->rewrap();
	ensure_cursor_on_screen();
	redraw = true;
}

bool edit_window_t::set_size(optint height, optint width) {
	bool result = true;
	if (width != t3_win_get_width(window) || height > t3_win_get_height(window))
		redraw = true;

	result &= t3_win_resize(window, height, width);
	result &= t3_win_resize(edit_window, height - 1, width - 1);
	result &= t3_win_resize(bottom_line_window, 1, width);
	result &= scrollbar.set_size(height - 1, None);

	if (text->get_wrap()) {
		text->rewrap();
		redraw = true;
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
		redraw = true;
	}

	if (text->cursor.line >= text->topleft.line + t3_win_get_height(edit_window)) {
		text->topleft.line = text->cursor.line - t3_win_get_height(edit_window) + 1;
		redraw = true;
	}

	if (screen_pos < text->topleft.pos) {
		text->topleft.pos = screen_pos;
		redraw = true;
	}

	if (screen_pos + width > text->topleft.pos + t3_win_get_width(edit_window) - 1) {
		text->topleft.pos = screen_pos + width - t3_win_get_width(edit_window) + 1;
		redraw = true;
	}
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
	info.size = t3_win_get_width(edit_window) - 1;
	info.normal_attr = 0;
	info.selected_attr = attributes.text_selected;
	for (i = 0; i < t3_win_get_height(edit_window) && (i + text->topleft.line) < text->get_used_lines(); i++) {
		t3_win_set_paint(edit_window, i, 0);

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

		info.cursor = focus && text->topleft.line + i == text->cursor.line && !hard_cursor ? text->cursor.pos : -1;
		text->paint_line(edit_window, text->topleft.line + i, &info);
	}

	t3_win_clrtobot(edit_window);
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

	if (text->cursor.line + t3_win_get_height(edit_window) - 1 < text->get_used_lines()) {
		text->cursor.line += t3_win_get_height(edit_window) - 1;
	} else {
		text->cursor.line = text->get_used_lines() - 1;
		text->cursor.pos = text->get_line_max(text->cursor.line);
		need_adjust = false;
	}

	/* If the end of the text is already on the screen, don't change the top line. */
	if (text->topleft.line + t3_win_get_height(edit_window) < text->get_used_lines()) {
		text->topleft.line += t3_win_get_height(edit_window) - 1;
		if (text->topleft.line + t3_win_get_height(edit_window) > text->get_used_lines())
			text->topleft.line = text->get_used_lines() - t3_win_get_height(edit_window);
		redraw = true;
	}

	if (need_adjust)
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);

	ensure_cursor_on_screen();

	if (!need_adjust)
		text->last_set_pos = screen_pos;
}

void edit_window_t::pgup(void) {
	bool need_adjust = true;

	if (text->topleft.line < t3_win_get_height(edit_window) - 1) {
		if (text->topleft.line != 0) {
			redraw = true;
			text->topleft.line = 0;
		}

		if (text->cursor.line < t3_win_get_height(edit_window) - 1) {
			text->cursor.line = 0;
			text->last_set_pos = text->cursor.pos = 0;
			need_adjust = false;
		} else {
			text->cursor.line -= t3_win_get_height(edit_window) - 1;
		}
	} else {
		text->cursor.line -= t3_win_get_height(edit_window) - 1;
		text->topleft.line -= t3_win_get_height(edit_window) - 1;
		redraw = true;
	}

	if (need_adjust)
		text->cursor.pos = text->calculate_line_pos(text->cursor.line, text->last_set_pos);

	ensure_cursor_on_screen();
}

void edit_window_t::reset_selection(void) {
	text->selection_mode = selection_mode_t::NONE;
	text->set_selection_start(0, -1);
	text->set_selection_end(0, -1);
	redraw = true;
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

	redraw = true;
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
	reset_selection();
}

void edit_window_t::find_activated(find_action_t action, finder_t *_finder) {
	finder_t *local_finder;

	local_finder = finder == NULL ? &global_finder : finder;
	if (_finder != NULL)
		*local_finder = *_finder;

	switch (action) {
		case find_action_t::FIND:
			if (!text->find(local_finder))
				goto not_found;

			ensure_cursor_on_screen();
			redraw = true;
			if (local_finder->get_flags() & find_flags_t::REPLACEMENT_VALID) {
				replace_buttons_connection.disconnect();
				replace_buttons_connection = replace_buttons->connect_activate(
					sigc::bind(sigc::mem_fun(this, &edit_window_t::find_activated), (finder_t *) NULL));
				replace_buttons->center_over(center_window);
				replace_buttons->show();
			}
			break;
		case find_action_t::REPLACE:
			text->replace(local_finder);
			redraw = true;
		case find_action_t::SKIP:
			if (!text->find(local_finder)) {
				ensure_cursor_on_screen();
				goto not_found;
			}
			redraw = true;
			ensure_cursor_on_screen();
			replace_buttons->reshow(action);
			break;
		case find_action_t::REPLACE_ALL: {
			int replacements;

			for (replacements = 0; text->find(local_finder); replacements++)
				text->replace(local_finder);

			if (replacements == 0)
				goto not_found;
			redraw = true;
			break;
		}
		case find_action_t::REPLACE_IN_SELECTION:
			//FIXME: do the replacement
			break;
		default:
			break;
	}
	return;

not_found:
	//FIXME: show search string
	message_dialog->set_message("Search string not found");
	message_dialog->center_over(center_window);
	message_dialog->show();
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
					redraw = true;
				} else if (text->cursor.line + 1 < text->get_used_lines()) {
					text->merge(false);
					if (text->get_wrap())
						ensure_cursor_on_screen();
					redraw = true;
				}
			} else {
				delete_selection();
			}
			break;

		case EKEY_NL:
			if (text->selection_mode != selection_mode_t::NONE)
				delete_selection();

			text->break_line();
			ensure_cursor_on_screen();
			text->last_set_pos = screen_pos;
			redraw = true;
			break;

		case EKEY_BS:
			if (text->selection_mode == selection_mode_t::NONE) {
				if (text->cursor.pos <= text->get_line_max(text->cursor.line)) {
					if (text->cursor.pos != 0) {
						text->backspace_char();
						redraw = true;
					} else if (text->cursor.line != 0) {
						text->merge(true);
						redraw = true;
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
			goto_line();
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

		case EKEY_CTRL | 'f': {
		case EKEY_CTRL | 'r':
			find_dialog_t *dialog;
			if (find_dialog == NULL) {
				global_find_dialog_connection.disconnect();
				global_find_dialog_connection = global_find_dialog->connect_activate(
					sigc::mem_fun(this, &edit_window_t::find_activated));
				dialog = global_find_dialog;
			} else {
				dialog = find_dialog;
			}
			dialog->center_over(center_window);
			dialog->set_replace(key == (EKEY_CTRL | 'r'));
			//FIXME: set selected text in dialog
			//dialog->set_text(text->get_selected_text());
			dialog->show();
			break;
		}

		case EKEY_F3:
		case EKEY_F3 | EKEY_SHIFT:
			if (!text->find(finder != NULL ? finder : &global_finder, (key & EKEY_SHIFT) != 0)) {
				//FIXME: show search string
				message_dialog->set_message("Search string not found");
				message_dialog->center_over(center_window);
				message_dialog->show();
			}
			break;

		case EKEY_F9:
			insert_special();
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
				redraw = true;
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

	if (!focus && !redraw)
		return;

	if (text->selection_mode != selection_mode_t::NONE && text->selection_mode != selection_mode_t::ALL) {
		text->set_selection_end(text->cursor.line, text->cursor.pos);

		if (text->selection_mode == selection_mode_t::SHIFT) {
			if (text->selection_empty())
				reset_selection();
		}
	}

	hard_cursor = (text->selection_mode == selection_mode_t::NONE && attributes.text_cursor == 0) ||
			(text->selection_mode != selection_mode_t::NONE && attributes.selection_cursor == 0);

	//FIXME: don't want to fully repaint on every key when selecting!!
	if (redraw || text->selection_mode != selection_mode_t::NONE || !hard_cursor) {
		redraw = false;
		repaint_screen();
	}

	t3_win_set_default_attrs(edit_window, attributes.text);
	t3_win_set_default_attrs(bottom_line_window, attributes.menubar);
	t3_win_set_paint(bottom_line_window, 0, 0);
	t3_win_addchrep(bottom_line_window, ' ', 0, t3_win_get_width(bottom_line_window));

	scrollbar.set_parameters(max(text->get_used_lines(), text->topleft.line + t3_win_get_height(edit_window)),
		text->topleft.line, t3_win_get_height(edit_window));
	scrollbar.update_contents();

	text->get_line_info(&logical_cursor_pos);
	snprintf(info, 29, "L: %-4d C: %-4d %c %s", logical_cursor_pos.line + 1, logical_cursor_pos.pos + 1, text->is_modified() ? '*' : ' ', ins_string[text->ins_mode]);
	info_width = t3_term_strwidth(info);
	name_width = t3_win_get_width(bottom_line_window) - info_width - 3;

	if (name_width > 3) {
		/* FIXME: is it really necessary to do this on each key stroke??? */
		t3_win_set_paint(bottom_line_window, 0, 0);
		if (text->name_line.calculate_screen_width(0, text->name_line.get_length(), 1) > name_width) {
			t3_win_addstr(bottom_line_window, "..", attributes.dialog);
			paint_info.start = text->name_line.adjust_position(text->name_line.get_length(), -(name_width - 2));
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

		text->name_line.paint_line(bottom_line_window, &paint_info);
	}

	t3_win_set_paint(bottom_line_window, 0, t3_win_get_width(bottom_line_window) - strlen(info) - 1);
	t3_win_addstr(bottom_line_window, info, 0);
	if (focus) {
		if (hard_cursor) {
			t3_win_set_cursor(edit_window, text->cursor.line - text->topleft.line, screen_pos - text->topleft.pos);
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
			t3_win_set_cursor(edit_window, text->cursor.line - text->topleft.line, screen_pos - text->topleft.pos);
			t3_term_show_cursor();
		} else {
			repaint_screen(); //FXIME: Only for removing cursor
		}
	} else {
		if (!hard_cursor)
			repaint_screen(); //FXIME: Only for removing cursor
	}
}

void edit_window_t::get_dimensions(int *height, int *width, int *top, int *left) {
	*height = t3_win_get_height(edit_window) + 1;
	*width = t3_win_get_width(edit_window);
	*top = t3_win_get_y(edit_window);
	*left = t3_win_get_x(edit_window);
}


void edit_window_t::undo(void) {
	if (text->apply_undo() == 0) {
		reset_selection();
		redraw = true;
		ensure_cursor_on_screen();
		text->last_set_pos = screen_pos;
	}
}

void edit_window_t::redo(void) {
	if (text->apply_redo() == 0) {
		reset_selection();
		redraw = true;
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
		redraw = true;
	}
}

void edit_window_t::select_all(void) {
	text->selection_mode = selection_mode_t::ALL;
	text->set_selection_start(0, 0);
	text->set_selection_end(text->get_used_lines() - 1, text->get_line_max(text->get_used_lines() - 1));
	redraw = true;
}

void edit_window_t::insert_special(void) {
	insert_char_dialog->center_over(center_window);
	insert_char_dialog->reset();
	insert_char_dialog->show();
}

void edit_window_t::goto_line(void) {
	goto_connection.disconnect();
	goto_connection = goto_dialog->connect_activate(sigc::mem_fun1(this, &edit_window_t::goto_line));
	goto_dialog->center_over(center_window);
	goto_dialog->reset();
	goto_dialog->show();
}

void edit_window_t::goto_line(int line) {
	if (line < 1)
		return;

	text->cursor.line = (line > text->get_used_lines() ? text->get_used_lines() : line) - 1;
	if (text->cursor.pos > text->get_line_max(text->cursor.line))
		text->cursor.pos = text->get_line_max(text->cursor.line);
	ensure_cursor_on_screen();
	text->last_set_pos = screen_pos;
}

bool edit_window_t::get_selection_lines(int *top, int *bottom) {
	if (text->selection_mode == selection_mode_t::NONE)
		return false;
	*top = text->selection_start.line - text->topleft.line;
	*bottom = text->selection_end.line - text->topleft.line;
	return true;
}

const text_buffer_t *edit_window_t::get_text(void) {
	return text;
}

void edit_window_t::set_find_dialog(find_dialog_t *_find_dialog) {
	find_dialog = _find_dialog;
}

void edit_window_t::set_finder(finder_t *_finder) {
	finder = _finder;
}

}; // namespace
