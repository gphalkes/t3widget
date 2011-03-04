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

#include "colorscheme.h"
#include "main.h"
#include "widgets/textfield.h"

using namespace std;

/*FIXME:
	auto-completion
	- when typing directories, the entries in the dir should complete as well
	- pgup pgdn should take steps in the drop-down list
	- pressing esc in drop-down list should close the list
*/

text_field_t::text_field_t(container_t *_parent) : widget_t(parent, 1, 4),
	width(4),
	pos(0),
	screen_pos(0),
	leftcol(0),
	focus(false),
	in_drop_down_list(false),
	dont_select_on_focus(false),
	line(new line_t),
	filter_keys(NULL),
	label(NULL),
	parent(_parent),
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
	selection_mode = SelectionMode::NONE;
	need_repaint = REPAINT_OTHER;
}

void text_field_t::set_selection(key_t key) {
	switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
		case EKEY_END:
		case EKEY_HOME:
		case EKEY_LEFT:
		case EKEY_RIGHT:
			if (selection_mode == SelectionMode::SHIFT && !(key & EKEY_SHIFT)) {
				reset_selection();
			} else if (selection_mode == SelectionMode::NONE && (key & EKEY_SHIFT)) {
				selection_start_pos = pos;
				selection_end_pos = pos;
				selection_mode = SelectionMode::SHIFT;
			}
			break;
		default:
			break;
	}
}

void text_field_t::delete_selection(bool saveToCopybuffer) {
	line_t *result;

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

	result = line->cutLine(start, end);
	if (saveToCopybuffer) {
		if (copyBuffer != NULL)
			delete copyBuffer;
		copyBuffer = result;
	} else {
		delete result;
	}
	pos = start;
	ensure_on_cursor_screen();
	reset_selection();
	need_repaint = REPAINT_EDIT;
}

bool text_field_t::process_key(key_t key) {
	if (in_drop_down_list)
		return drop_down_list->process_key(key);

	set_selection(key);

	switch (key) {
		case EKEY_DOWN:
			if (drop_down_list != NULL && drop_down_list->has_items() && line->get_length() > 0) {
				in_drop_down_list = true;
				drop_down_list->set_focus(true);
			} else {
				move_focus_down();
			}
			break;
		case EKEY_UP:
			move_focus_up();
			break;
		case EKEY_BS:
			if (selection_mode != SelectionMode::NONE) {
				delete_selection(false);
			} else if (pos > 0) {
				int newpos = line->adjust_position(pos, -1);
				line->backspace_char(pos, NULL);
				pos = newpos;
				ensure_on_cursor_screen();
				need_repaint = REPAINT_EDIT;
			}
			break;
		case EKEY_DEL:
			if (selection_mode != SelectionMode::NONE) {
				delete_selection(false);
			} else if (pos < line->get_length()) {
				line->delete_char(pos, NULL);
				need_repaint = REPAINT_EDIT;
			}
			break;
		case EKEY_LEFT:
		case EKEY_LEFT | EKEY_SHIFT:
			if (pos > 0) {
				pos = line->adjust_position(pos, -1);
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_RIGHT:
		case EKEY_RIGHT | EKEY_SHIFT:
			if (pos < line->get_length()) {
				pos = line->adjust_position(pos, 1);
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_RIGHT | EKEY_CTRL:
		case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
			if (pos < line->get_length()) {
				pos = line->get_next_word(pos);
				if (pos < 0)
					pos = line->get_length();
				ensure_on_cursor_screen();
			}
			break;
		case EKEY_LEFT | EKEY_CTRL:
		case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
			if (pos > 0) {
				pos = line->get_previous_word(pos);
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
			pos = line->get_length();
			ensure_on_cursor_screen();
			break;
		case EKEY_CTRL | 'x':
			if (selection_mode != SelectionMode::NONE)
				delete_selection(true);
			break;
		case EKEY_CTRL | 'c':
			if (selection_mode != SelectionMode::NONE) {
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

				if (copyBuffer != NULL)
					delete copyBuffer;

				copyBuffer = line->clone(start, end);
			}
			break;

		case EKEY_CTRL | 'v':
			if (copyBuffer != NULL) {
				line_t *end = NULL;

				if (selection_mode != SelectionMode::NONE)
					delete_selection(false);
				if (pos < line->get_length())
					end = line->break_line(pos);

				line->merge(copyBuffer->clone(0, -1));
				pos = line->get_length();
				if (end != NULL)
					line->merge(end);
				ensure_on_cursor_screen();
				need_repaint = REPAINT_EDIT;
			}
			break;

		case 0: //CTRL-SPACE (and others)
			switch (selection_mode) {
				case SelectionMode::MARK:
					reset_selection();
					break;
				case SelectionMode::NONE:
					selection_start_pos = pos;
					selection_end_pos = pos;
				/* FALLTHROUGH */
				case SelectionMode::SHIFT:
					selection_mode = SelectionMode::MARK;
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
#warning FIXME: activate InsertCharacter dialog
			//executeAction(ActionID::EDIT_INSERT_CHAR);
			break;

		default:
			if (key < 31)
				return false;

			key &= ~EKEY_PROTECT;

			if (key < 0x110000) {
				if (filter_keys != NULL && (find(filter_keys, filter_keys + filter_keys_size, key) == filter_keys + filter_keys_size) == filter_keys_accept)
					return false;

				if (selection_mode != SelectionMode::NONE)
					delete_selection(false);

				if (pos == line->get_length())
					line->append_char(key, NULL);
				else
					line->insert_char(pos, key, NULL);
					#warning FIXME: implement overwrite!
				pos = line->adjust_position(pos, 1);
				ensure_on_cursor_screen();
				need_repaint = REPAINT_EDIT;
			}
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

	need_repaint = REPAINT_RESIZE;
	//FIXME: use return values from different parts as return value!
	return true;
}

void text_field_t::update_selection(void) {
	if (selection_mode == SelectionMode::SHIFT) {
		if (selection_start_pos == pos) {
			reset_selection();
			return;
		}
	}

	selection_end_pos = pos;
}

void text_field_t::update_contents(void) {
	bool hard_cursor;

	if (selection_mode != SelectionMode::NONE)
		update_selection();

	hard_cursor = (selection_mode == SelectionMode::NONE && colors.attr_cursor == 0) ||
			(selection_mode != SelectionMode::NONE && colors.attr_selection_cursor == 0);

	if (need_repaint || (selection_mode != SelectionMode::NONE && focus) || !hard_cursor) {
		line_t::paint_info_t info;

		t3_win_set_paint(window, 0, 0);
		t3_win_addch(window, '[', 0);

		info.start = 0;
		info.leftcol = leftcol;
		info.max = INT_MAX;
		info.size = width - 2;
		info.tabsize = 0;
		info.flags = line_t::SPACECLEAR | line_t::TAB_AS_CONTROL;
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
		info.cursor = focus && !hard_cursor ? screen_pos : -1;
		info.normal_attr = colors.textfield_attrs;
		info.selected_attr = colors.textfield_selected_attrs;

		line->paint_line(window, &info);
		t3_win_addch(window, ']', 0);

		if (drop_down_list != NULL && need_repaint == REPAINT_EDIT) {
			drop_down_list->update_view();
			if (drop_down_list->has_items() && line->get_length() > 0)
				drop_down_list->show();
			else
				drop_down_list->hide();
		}
		need_repaint = NO_REPAINT;
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
			selection_end_pos = pos = line->get_length();
			selection_start_pos = 0;
			selection_mode = SelectionMode::SHIFT;
		}
		dont_select_on_focus = true;
		if (drop_down_list != NULL)
			drop_down_list->update_view();
	} else {
		if (drop_down_list != NULL) {
			drop_down_list->set_focus(false);
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
	if (drop_down_list != NULL)
		drop_down_list->hide();
	widget_t::hide();
}

void text_field_t::ensure_on_cursor_screen(void) {
	int char_width;

	if (pos == line->get_length())
		char_width = 1;
	else
		char_width = line->widthAt(pos);

	screen_pos = line->calculateScreenWidth(0, pos, 0);

	if (screen_pos < leftcol) {
		leftcol = screen_pos;
		need_repaint = REPAINT_OTHER;
	}

	if (screen_pos + char_width > leftcol + width - 2) {
		leftcol = screen_pos - (width - 2) + char_width;
		need_repaint = REPAINT_OTHER;
	}
}

void text_field_t::set_text(const string *text) {
	if (line != NULL)
		delete line;
	line = new line_t(text->data(), text->size());
	set_text_finish();
}

void text_field_t::set_text(const char *text) {
	if (line != NULL)
		delete line;
	line = new line_t(text, strlen(text));
	set_text_finish();
}

void text_field_t::set_text_finish(void) {
	pos = line->get_length();
	ensure_on_cursor_screen();
	need_repaint = REPAINT_SET;
}

void text_field_t::set_key_filter(key_t *keys, size_t nrOfKeys, bool accept) {
	filter_keys = keys;
	filter_keys_size = nrOfKeys;
	filter_keys_accept = accept;
}

const string *text_field_t::get_text(void) const {
	return line->getData();
}

const line_t *text_field_t::get_line(void) const {
	return line;
}

void text_field_t::set_autocomplete(string_list_t *completions) {
	if (drop_down_list == NULL)
		drop_down_list = new drop_down_list_t(parent->get_draw_window(), window, width, this);
	drop_down_list->set_autocomplete(completions);
}


/*==================
  == drop_down_list_t ==
  ==================*/
text_field_t::drop_down_list_t::drop_down_list_t(t3_window_t *_parent, t3_window_t *anchor, int _width, text_field_t *_field) :
	width(_width), field(_field)
{
	if ((window = t3_win_new(_parent, 6, width, 1, 0, -1)) == NULL)
		throw(-1);
	t3_win_set_anchor(window, anchor, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	t3_win_set_default_attrs(window, colors.dialog_attrs);
	focus = false;
	view = NULL;
	current = 0;
}

text_field_t::drop_down_list_t::~drop_down_list_t(void) {
	t3_win_del(window);
}

bool text_field_t::drop_down_list_t::process_key(key_t key) {
	string_list_t *list = view == NULL ? completions : view;
	size_t length = list->get_length();

	switch (key) {
		case EKEY_DOWN:
			if (current + 1 < length) {
				current++;
				if (current - top_idx > 4)
					top_idx++;
				field->set_text(list->get_name(current));
			}
			break;
		case EKEY_UP:
			if (current > 0) {
				current--;
				if (top_idx > current)
					top_idx = current;
				field->set_text(list->get_name(current));
			} else {
				focus = false;
				field->in_drop_down_list = false;
				update_contents();
			}
			break;
		case EKEY_NL:
			focus = false;
			field->in_drop_down_list = false;
			field->process_key(key);
			t3_win_hide(window);
			break;
		default:
			focus = false;
			field->in_drop_down_list = false;
			field->process_key(key);
			break;
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
	update_contents();
	return result;
}

void text_field_t::drop_down_list_t::update_contents(void) {
	size_t i, idx;
//FIXME: make this more general!!!
	file_list_t *list = view == NULL ? (file_list_t *) completions : view;

	if (list == NULL)
			return;

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
	for (i = 0, idx = top_idx; i < 5 && idx < list->get_length(); i++, idx++) {
		line_t::paint_info_t info;
		line_t fileNameLine(list->get_name(idx));
		bool paint_selected = focus && idx == current;

		t3_win_set_paint(window, i, 1);

		info.start = 0;
		info.leftcol = 0;
		info.max = INT_MAX;
		info.size = width - 2;
		info.tabsize = 0;
		info.flags = line_t::SPACECLEAR | line_t::TAB_AS_CONTROL | (paint_selected ? line_t::EXTEND_SELECTION : 0);
		info.selection_start = -1;
		info.selection_end = paint_selected ? INT_MAX : -1;
		info.cursor = -1;
		info.normal_attr = 0;
		info.selected_attr = colors.dialog_selected_attrs;

		fileNameLine.paint_line(window, &info);

		if (list->is_dir(idx)) {
			int length = fileNameLine.get_length();
			if (length < width) {
				t3_win_set_paint(window, i, length + 1);
				t3_win_addch(window, '/', paint_selected ? T3_ATTR_REVERSE : 0);
			}
		}
	}
}

void text_field_t::drop_down_list_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		string_list_t *list = view == NULL ? completions : view;
		current = 0;
		field->set_text(list->get_name(current));
	}
}

void text_field_t::drop_down_list_t::show(void) {
	t3_win_show(window);
}

void text_field_t::drop_down_list_t::hide(void) {
	t3_win_hide(window);
}

void text_field_t::drop_down_list_t::update_view(void) {
	file_name_list_t *file_completions = dynamic_cast<file_name_list_t *>(completions);

	top_idx = current = 0;

	if (view != NULL)
		delete view;
	if (file_completions != NULL) {
		view = new file_name_list_t::file_name_list_view_t(file_completions, field->line->getData());
	} else {
		view = NULL;
	}
}

void text_field_t::drop_down_list_t::set_autocomplete(string_list_t *_completions) {
	completions = _completions;
	if (view != NULL) {
		delete view;
		view = NULL;
	}
}

bool text_field_t::drop_down_list_t::has_items(void) {
	return view == NULL ? false : view->get_length() > 0;
}

void text_field_t::set_label(smart_label_t *_label) {
	label = _label;
}

bool text_field_t::is_hotkey(key_t key) {
	return label == NULL ? false : label->is_hotkey(key);
}
