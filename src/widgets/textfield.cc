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
#include <algorithm>

#include "main.h"
#include "colorscheme.h"
#include "internal.h"
#include "widgets/textfield.h"

#include "log.h"

using namespace std;
namespace t3_widget {

/*FIXME:
	auto-completion
	- when typing directories, the entries in the dir should complete as well
	- pgup pgdn should take steps in the drop-down list
	- pressing esc in drop-down list should close the list
*/

text_field_t::text_field_t(void) : widget_t(1, 4),
	width(4),
	pos(0),
	screen_pos(0),
	leftcol(0),
	focus(false),
	in_drop_down_list(false),
	drop_down_list_shown(false),
	dont_select_on_focus(false),
	edited(false),
	filter_keys(NULL),
	label(NULL),
	drop_down_list(NULL)
{
	reset_selection();
}

text_field_t::~text_field_t(void) {
	delete drop_down_list;
}

void text_field_t::reset_selection(void) {
	selection_start_pos = -1;
	selection_end_pos = -1;
	selection_mode = selection_mode_t::NONE;
	redraw = true;
}

void text_field_t::set_selection(key_t key) {
	switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
		case EKEY_END:
		case EKEY_HOME:
		case EKEY_LEFT:
		case EKEY_RIGHT:
			if (selection_mode == selection_mode_t::SHIFT && !(key & EKEY_SHIFT)) {
				reset_selection();
			} else if (selection_mode == selection_mode_t::NONE && (key & EKEY_SHIFT)) {
				selection_start_pos = pos;
				selection_end_pos = pos;
				selection_mode = selection_mode_t::SHIFT;
			}
			break;
		default:
			break;
	}
}

void text_field_t::delete_selection(bool save_to_copy_buffer) {
	text_line_t *result;

	int start, end;
	if (selection_start_pos == selection_end_pos) {
		reset_selection();
		return;
	} else if (selection_start_pos < selection_end_pos) {
		start = selection_start_pos;
		end = selection_end_pos;
	} else {
		start = selection_end_pos;
		end = selection_start_pos;
	}

	result = line.cut_line(start, end);
	if (save_to_copy_buffer) {
		if (copy_buffer != NULL)
			delete copy_buffer;
		copy_buffer = result;
	} else {
		delete result;
	}
	pos = start;
	ensure_on_cursor_screen();
	reset_selection();
	redraw = true;
	edited = true;
}

bool text_field_t::process_key(key_t key) {
	if (in_drop_down_list)
		return drop_down_list->process_key(key);

	set_selection(key);

	switch (key) {
		case EKEY_DOWN:
			if (drop_down_list != NULL && drop_down_list->has_items() && line.get_length() > 0) {
				in_drop_down_list = true;
				drop_down_list_shown = true;
				drop_down_list->set_focus(true);
				drop_down_list->show();
			} else {
				move_focus_down();
			}
			break;
		case EKEY_UP:
			move_focus_up();
			break;
		case EKEY_BS:
			if (selection_mode != selection_mode_t::NONE) {
				delete_selection(false);
			} else if (pos > 0) {
				int newpos = line.adjust_position(pos, -1);
				line.backspace_char(pos, NULL);
				pos = newpos;
				ensure_on_cursor_screen();
				redraw = true;
				edited = true;
			}
			break;
		case EKEY_DEL:
			if (selection_mode != selection_mode_t::NONE) {
				delete_selection(false);
			} else if (pos < line.get_length()) {
				line.delete_char(pos, NULL);
				redraw = true;
				edited = true;
			}
			break;
		case EKEY_LEFT:
		case EKEY_LEFT | EKEY_SHIFT:
			if (pos > 0) {
				pos = line.adjust_position(pos, -1);
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_RIGHT:
		case EKEY_RIGHT | EKEY_SHIFT:
			if (pos < line.get_length()) {
				pos = line.adjust_position(pos, 1);
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_RIGHT | EKEY_CTRL:
		case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
			if (pos < line.get_length()) {
				pos = line.get_next_word(pos);
				if (pos < 0)
					pos = line.get_length();
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_LEFT | EKEY_CTRL:
		case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
			if (pos > 0) {
				pos = line.get_previous_word(pos);
				if (pos < 0)
					pos = 0;
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_HOME:
		case EKEY_HOME | EKEY_SHIFT:
			pos = 0;
			ensure_on_cursor_screen();
			break;
		case EKEY_END:
		case EKEY_END | EKEY_SHIFT:
			pos = line.get_length();
			ensure_on_cursor_screen();
			break;
		case EKEY_CTRL | 'x':
			if (selection_mode != selection_mode_t::NONE)
				delete_selection(true);
			break;
		case EKEY_CTRL | 'c':
			if (selection_mode != selection_mode_t::NONE) {
				int start, end;
				if (selection_start_pos == selection_end_pos) {
					reset_selection();
					break;
				} else if (selection_start_pos < selection_end_pos) {
					start = selection_start_pos;
					end = selection_end_pos;
				} else {
					start = selection_end_pos;
					end = selection_start_pos;
				}

				if (copy_buffer != NULL)
					delete copy_buffer;

				copy_buffer = line.clone(start, end);
			}
			break;

		case EKEY_CTRL | 'v':
			if (copy_buffer != NULL) {
				text_line_t *end = NULL;

				if (selection_mode != selection_mode_t::NONE)
					delete_selection(false);
				if (pos < line.get_length())
					end = line.break_line(pos);

				line.merge(copy_buffer->clone(0, -1));
				pos = line.get_length();
				if (end != NULL)
					line.merge(end);
				ensure_on_cursor_screen();
				redraw = true;
				edited = true;
			}
			break;

		case 0: //CTRL-SPACE (and others)
			switch (selection_mode) {
				case selection_mode_t::MARK:
					reset_selection();
					break;
				case selection_mode_t::NONE:
					selection_start_pos = pos;
					selection_end_pos = pos;
				/* FALLTHROUGH */
				case selection_mode_t::SHIFT:
					selection_mode = selection_mode_t::MARK;
					break;
				default:
					/* Should not happen, but just try to get back to a sane state. */
					reset_selection();
					break;
			}
			break;

		case EKEY_NL:
			activate();
			break;

		case EKEY_F9:
			dont_select_on_focus = true;
			insert_char_dialog->center_over(center_window);
			insert_char_dialog->reset();
			insert_char_dialog->show();
			break;

		case EKEY_HOTKEY:
			return true;

		case EKEY_ESC:
			if (drop_down_list_shown) {
				drop_down_list_shown = false;
				drop_down_list->hide();
				return true;
			}
			return false;

		default:
			if (key < 31)
				return false;

			key &= ~EKEY_PROTECT;


			if (key > 0x10ffff)
				return false;

			if (filter_keys != NULL &&
					(find(filter_keys, filter_keys + filter_keys_size, key) == filter_keys + filter_keys_size) == filter_keys_accept)
				return false;

			if (selection_mode != selection_mode_t::NONE)
				delete_selection(false);

			if (pos == line.get_length())
				line.append_char(key, NULL);
			else
				line.insert_char(pos, key, NULL);
			pos = line.adjust_position(pos, 1);
			ensure_on_cursor_screen();
			redraw = true;
			edited = true;
	}

	return true;
}

bool text_field_t::set_size(optint height, optint _width) {
	(void) height;
	if (_width.is_valid() && width != _width) {
		width = _width;
		t3_win_resize(window, 1, width);
		if (drop_down_list != NULL)
			drop_down_list->set_size(None, width);
	}

	ensure_on_cursor_screen();

	redraw = true;
	//FIXME: use return values from different parts as return value!
	return true;
}

void text_field_t::update_selection(void) {
	if (selection_mode == selection_mode_t::SHIFT) {
		if (selection_start_pos == pos) {
			reset_selection();
			return;
		}
	}

	selection_end_pos = pos;
}

void text_field_t::update_contents(void) {
	bool hard_cursor;

	if (selection_mode != selection_mode_t::NONE)
		update_selection();

	hard_cursor = !in_drop_down_list && ((selection_mode == selection_mode_t::NONE && attributes.text_cursor == 0) ||
			(selection_mode != selection_mode_t::NONE && attributes.selection_cursor == 0));

	if (redraw || (selection_mode != selection_mode_t::NONE && focus) || !hard_cursor) {
		text_line_t::paint_info_t info;

		t3_win_set_default_attrs(window, attributes.dialog);
		t3_win_set_paint(window, 0, 0);
		t3_win_addch(window, '[', 0);

		info.start = 0;
		info.leftcol = leftcol;
		info.max = INT_MAX;
		info.size = width - 2;
		info.tabsize = 0;
		info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL;
		if (!focus) {
			info.selection_start = -1;
			info.selection_end = -1;
		} else if (selection_start_pos < selection_end_pos) {
			info.selection_start = selection_start_pos;
			info.selection_end = selection_end_pos;
		} else {
			info.selection_start = selection_end_pos;
			info.selection_end = selection_start_pos;
		}
		info.cursor = focus && !hard_cursor && !in_drop_down_list ? screen_pos : -1;
		info.normal_attr = attributes.text;
		info.selected_attr = attributes.text_selected;

		line.paint_line(window, &info);
		t3_win_addch(window, ']', 0);

		if (drop_down_list != NULL && edited) {
			drop_down_list->update_view();
			if (drop_down_list->has_items() && line.get_length() > 0) {
				drop_down_list_shown = true;
				drop_down_list->show();
			} else {
				drop_down_list_shown = false;
				drop_down_list->hide();
			}
		}
		edited = false;
		redraw = false;
	}

	if (drop_down_list != NULL && drop_down_list->has_items())
		drop_down_list->update_contents();

	if (focus) {
		if (hard_cursor) {
			t3_term_show_cursor();
			t3_win_set_cursor(window, 0, screen_pos - leftcol + 1);
		} else {
			t3_term_hide_cursor();
		}
	}
}

void text_field_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		if (!dont_select_on_focus) {
			selection_end_pos = pos = line.get_length();
			selection_start_pos = 0;
			selection_mode = selection_mode_t::SHIFT;
		}
		dont_select_on_focus = false;
		if (drop_down_list != NULL)
			drop_down_list->update_view();
	} else {
		if (drop_down_list != NULL) {
			drop_down_list->set_focus(false);
			drop_down_list_shown = false;
			drop_down_list->hide();
		}
		in_drop_down_list = false;
		lose_focus();
	}
}

void text_field_t::show(void) {
	in_drop_down_list = false;
	widget_t::show();
}

void text_field_t::hide(void) {
	if (drop_down_list != NULL) {
		drop_down_list_shown = false;
		drop_down_list->hide();
	}
	widget_t::hide();
}

void text_field_t::ensure_on_cursor_screen(void) {
	int char_width;

	if (pos == line.get_length())
		char_width = 1;
	else
		char_width = line.width_at(pos);

	screen_pos = line.calculate_screen_width(0, pos, 0);

	if (screen_pos < leftcol) {
		leftcol = screen_pos;
		redraw = true;
	}

	if (screen_pos + char_width > leftcol + width - 2) {
		leftcol = screen_pos - (width - 2) + char_width;
		redraw = true;
	}
}

void text_field_t::set_text(const string *text) {
	line.set_text(text);
	set_text_finish();
}

void text_field_t::set_text(const char *text) {
	line.set_text(text, strlen(text));
	set_text_finish();
}

void text_field_t::set_text_finish(void) {
	pos = line.get_length();
	ensure_on_cursor_screen();
	redraw = true;
}

void text_field_t::set_key_filter(key_t *keys, size_t nrOfKeys, bool accept) {
	filter_keys = keys;
	filter_keys_size = nrOfKeys;
	filter_keys_accept = accept;
}

const string *text_field_t::get_text(void) const {
	return line.get_data();
}

const text_line_t *text_field_t::get_line(void) const {
	return &line;
}

void text_field_t::set_autocomplete(string_list_t *completions) {
	if (drop_down_list == NULL)
		drop_down_list = new drop_down_list_t(this);
	drop_down_list->set_autocomplete(completions);
}

void text_field_t::set_label(smart_label_t *_label) {
	label = _label;
}

bool text_field_t::is_hotkey(key_t key) {
	return label == NULL ? false : label->is_hotkey(key);
}

/*======================
  == drop_down_list_t ==
  ======================*/
text_field_t::drop_down_list_t::drop_down_list_t(text_field_t *_field) :
	width(t3_win_get_width(_field->get_draw_window())), top_idx(0), field(_field), completions(NULL)
{
	if ((window = t3_win_new(NULL, 6, width, 1, 0, INT_MIN)) == NULL)
		throw(-1);
	t3_win_set_anchor(window, field->get_draw_window(), T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	focus = false;
	current = 0;
}

text_field_t::drop_down_list_t::~drop_down_list_t(void) {
	if (completions != NULL)
		delete completions;
	t3_win_del(window);
}

bool text_field_t::drop_down_list_t::process_key(key_t key) {
	size_t length = completions->size();

	switch (key) {
		case EKEY_DOWN:
			if (current + 1 < length) {
				current++;
				if (current - top_idx > 4)
					top_idx++;
				field->set_text((*completions)[current]);
			}
			break;
		case EKEY_UP:
			if (current > 0) {
				current--;
				if (top_idx > current)
					top_idx = current;
				field->set_text((*completions)[current]);
			} else {
				focus = false;
				field->in_drop_down_list = false;
				update_contents();
			}
			break;
		case EKEY_NL:
			focus = false;
			field->in_drop_down_list = false;
			t3_win_hide(window);
			return field->process_key(key);
		case EKEY_ESC:
			field->in_drop_down_list = false;
			field->drop_down_list_shown = false;
			t3_win_hide(window);
			break;
		default:
			focus = false;
			field->in_drop_down_list = false;
			return field->process_key(key);
	}
	return true;
}

void text_field_t::drop_down_list_t::set_position(optint top, optint left) {
	(void) top;
	(void) left;
}

bool text_field_t::drop_down_list_t::set_size(optint height, optint _width) {
	bool result;

	(void) height;

	width = _width;

	result = t3_win_resize(window, 6, width);
	redraw = true;
	return result;
}

void text_field_t::drop_down_list_t::update_contents(void) {
	size_t i, idx;
	file_list_t *file_list;

	if (completions == NULL)
		return;

	file_list = dynamic_cast<file_list_t *>(completions);

	t3_win_set_default_attrs(window, attributes.dialog);
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);
	for (i = 0; i < 5; i++) {
		t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_VLINE, T3_ATTR_ACS);
		t3_win_set_paint(window, i, width - 1);
		t3_win_addch(window, T3_ACS_VLINE, T3_ATTR_ACS);
	}
	t3_win_set_paint(window, 5, 0);
	t3_win_addch(window, T3_ACS_LLCORNER, T3_ATTR_ACS);
	t3_win_addchrep(window, T3_ACS_HLINE, T3_ATTR_ACS, width - 2);
	t3_win_addch(window, T3_ACS_LRCORNER, T3_ATTR_ACS);
	for (i = 0, idx = top_idx; i < 5 && idx < completions->size(); i++, idx++) {
		text_line_t::paint_info_t info;
		text_line_t fileNameLine((*completions)[idx]);
		bool paint_selected = focus && idx == current;

		t3_win_set_paint(window, i, 1);

		info.start = 0;
		info.leftcol = 0;
		info.max = INT_MAX;
		info.size = width - 2;
		info.tabsize = 0;
		info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL | (paint_selected ? text_line_t::EXTEND_SELECTION : 0);
		info.selection_start = -1;
		info.selection_end = paint_selected ? INT_MAX : -1;
		info.cursor = -1;
		info.normal_attr = 0;
		info.selected_attr = attributes.dialog_selected;

		fileNameLine.paint_line(window, &info);

		if (file_list != NULL && file_list->is_dir(idx)) {
			int length = fileNameLine.get_length();
			if (length < width) {
				t3_win_set_paint(window, i, length + 1);
				t3_win_addch(window, '/', paint_selected ? attributes.dialog_selected : 0);
			}
		}
	}
}

void text_field_t::drop_down_list_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		current = 0;
		field->set_text((*completions)[current]);
		t3_term_hide_cursor();
	}
}

void text_field_t::drop_down_list_t::show(void) {
	t3_win_show(window);
}

void text_field_t::drop_down_list_t::hide(void) {
	t3_win_hide(window);
}

void text_field_t::drop_down_list_t::update_view(void) {
	top_idx = current = 0;

	if (completions != NULL) {
		if (field->line.get_length() == 0)
			completions->reset_filter();
		else
			completions->set_filter(sigc::bind(sigc::ptr_fun(string_compare_filter), field->line.get_data()));
	}
}

void text_field_t::drop_down_list_t::set_autocomplete(string_list_t *_completions) {
	if (completions != NULL)
		delete completions;

	if (dynamic_cast<file_list_t *>(_completions) != NULL)
		completions = new filtered_file_list_t((file_list_t *) _completions);
	else
		completions = new filtered_string_list_t(_completions);
}

bool text_field_t::drop_down_list_t::has_items(void) {
	return completions->size() > 0;
}

}; // namespace
