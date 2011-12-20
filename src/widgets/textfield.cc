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
#include <cstring>
#include <algorithm>

#include "main.h"
#include "colorscheme.h"
#include "internal.h"
#include "widgets/textfield.h"
#include "clipboard.h"
#include "log.h"

using namespace std;
namespace t3_widget {

/*FIXME:
- auto-completion
	- when typing directories, the entries in the dir should complete as well
	- pgup pgdn should take steps in the drop-down list
	- the first autocompletion option should be filled in, in the text field itself.
	  However, this text should be selected, and it should only be appended if the
	  user added characters.
- undo
*/

text_field_t::text_field_t(void) : widget_t(1, 4),
	pos(0),
	screen_pos(0),
	leftcol(0),
	focus(false),
	in_drop_down_list(false),
	drop_down_list_shown(false),
	dont_select_on_focus(false),
	edited(false),
	line(new text_line_t()),
	filter_keys(NULL),
	label(NULL),
	drop_down_list(NULL)
{
	reset_selection();
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

	result = line->cut_line(start, end);
	if (save_to_copy_buffer)
		set_clipboard(new string(*result->get_data()));

	delete result;

	pos = start;
	ensure_cursor_on_screen();
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
			if (drop_down_list != NULL && !drop_down_list->empty() && line->get_length() > 0) {
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
				int newpos = line->adjust_position(pos, -1);
				line->backspace_char(pos, NULL);
				pos = newpos;
				ensure_cursor_on_screen();
				redraw = true;
				edited = true;
			}
			break;
		case EKEY_DEL:
			if (selection_mode != selection_mode_t::NONE) {
				delete_selection(false);
			} else if (pos < line->get_length()) {
				line->delete_char(pos, NULL);
				redraw = true;
				edited = true;
			}
			break;
		case EKEY_LEFT:
		case EKEY_LEFT | EKEY_SHIFT:
			if (pos > 0) {
				redraw = true;
				pos = line->adjust_position(pos, -1);
				ensure_cursor_on_screen();
			}
			break;
		case EKEY_RIGHT:
		case EKEY_RIGHT | EKEY_SHIFT:
			if (pos < line->get_length()) {
				redraw = true;
				pos = line->adjust_position(pos, 1);
				ensure_cursor_on_screen();
			}
			break;
		case EKEY_RIGHT | EKEY_CTRL:
		case EKEY_RIGHT | EKEY_CTRL | EKEY_SHIFT:
			if (pos < line->get_length()) {
				redraw = true;
				pos = line->get_next_word(pos);
				if (pos < 0)
					pos = line->get_length();
				ensure_cursor_on_screen();
			}
			break;
		case EKEY_LEFT | EKEY_CTRL:
		case EKEY_LEFT | EKEY_CTRL | EKEY_SHIFT:
			if (pos > 0) {
				redraw = true;
				pos = line->get_previous_word(pos);
				if (pos < 0)
					pos = 0;
				ensure_cursor_on_screen();
			}
			break;
		case EKEY_HOME:
		case EKEY_HOME | EKEY_SHIFT:
			redraw = true;
			pos = 0;
			ensure_cursor_on_screen();
			break;
		case EKEY_END:
		case EKEY_END | EKEY_SHIFT:
			redraw = true;
			pos = line->get_length();
			ensure_cursor_on_screen();
			break;
		case EKEY_CTRL | 'x':
		case EKEY_SHIFT | EKEY_DEL:
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

				set_clipboard(new string(*line->get_data(), start, end - start));
			}
			break;

		case EKEY_CTRL | 'v':
		case EKEY_SHIFT | EKEY_INS: {
			linked_ptr<string> copy_buffer = get_clipboard();
			if (copy_buffer != NULL) {
				text_line_t insert_line(copy_buffer);

				if (selection_mode != selection_mode_t::NONE)
					delete_selection(false);

				line->insert(&insert_line, pos);
				pos += insert_line.get_length();
				ensure_cursor_on_screen();
				redraw = true;
				edited = true;
			}
			break;
		}

		case EKEY_CTRL | 't':
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
		case EKEY_META | '9':
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

			if (pos == line->get_length())
				line->append_char(key, NULL);
			else
				line->insert_char(pos, key, NULL);
			pos = line->adjust_position(pos, 1);
			ensure_cursor_on_screen();
			redraw = true;
			edited = true;
	}

	return true;
}

bool text_field_t::set_size(optint height, optint width) {
	(void) height;
	if (width.is_valid() && t3_win_get_width(window) != width) {
		t3_win_resize(window, 1, width);
		if (drop_down_list != NULL)
			drop_down_list->set_size(None, width);
	}

	ensure_cursor_on_screen();

	redraw = true;
	//FIXME: use return values from different parts as return value!
	return true;
}

void text_field_t::update_contents(void) {

	if (drop_down_list != NULL && edited) {
		drop_down_list->update_view();
		if (!drop_down_list->empty() && line->get_length() > 0) {
			drop_down_list_shown = true;
			drop_down_list->show();
		} else {
			drop_down_list_shown = false;
			drop_down_list->hide();
		}
	}

	if (drop_down_list != NULL && !drop_down_list->empty())
		drop_down_list->update_contents();

	if (!redraw)
		return;
	edited = false;
	redraw = false;

	if (selection_mode != selection_mode_t::NONE) {
		if (selection_mode == selection_mode_t::SHIFT && selection_start_pos == pos)
			reset_selection();
		else
			set_selection_end();
	}

	text_line_t::paint_info_t info;

	t3_win_set_default_attrs(window, attributes.dialog);
	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, leftcol == 0 ? '[' : '(', 0);

	info.start = 0;
	info.leftcol = leftcol;
	info.max = INT_MAX;
	info.size = t3_win_get_width(window) - 2;
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
	info.cursor = focus && !in_drop_down_list ? pos : -1;
	info.normal_attr = attributes.dialog;
	info.selected_attr = attributes.dialog_selected;

	line->paint_line(window, &info);
	t3_win_addch(window, line->calculate_screen_width(leftcol, INT_MAX, 0) > t3_win_get_width(window) - 2 ? ')' : ']', 0);
}

void text_field_t::set_focus(bool _focus) {
	focus = _focus;
	redraw = true;
	if (focus) {
		if (!dont_select_on_focus) {
			selection_start_pos = 0;
			selection_mode = selection_mode_t::SHIFT;
			pos = line->get_length();
			set_selection_end();
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

void text_field_t::ensure_cursor_on_screen(void) {
	int width, char_width;

	if (pos == line->get_length())
		char_width = 1;
	else
		char_width = line->width_at(pos);

	screen_pos = line->calculate_screen_width(0, pos, 0);

	if (screen_pos < leftcol) {
		leftcol = screen_pos;
		redraw = true;
	}

	width = t3_win_get_width(window);
	if (screen_pos + char_width > leftcol + width - 2) {
		leftcol = screen_pos - (width - 2) + char_width;
		redraw = true;
	}
}

void text_field_t::set_text(const string *text) {
	set_text(text->data(), text->size());
}

void text_field_t::set_text(const char *text) {
	set_text(text, strlen(text));
}

void text_field_t::set_text(const char *text, size_t size) {
	line->set_text(text, size);
	pos = line->get_length();
	ensure_cursor_on_screen();
	redraw = true;
}

void text_field_t::set_key_filter(key_t *keys, size_t nr_of_keys, bool accept) {
	filter_keys = keys;
	filter_keys_size = nr_of_keys;
	filter_keys_accept = accept;
}

const string *text_field_t::get_text(void) const {
	return line->get_data();
}

void text_field_t::set_autocomplete(string_list_base_t *completions) {
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

void text_field_t::bad_draw_recheck(void) {
	redraw = true;
}

bool text_field_t::process_mouse_event(mouse_event_t event) {
	if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT) && event.previous_button_state == 0) {
		if ((event.modifier_state & EMOUSE_SHIFT) == 0) {
			reset_selection();
		} else if (selection_mode == selection_mode_t::NONE) {
			selection_mode = selection_mode_t::SHIFT;
			selection_start_pos = pos;
		}
		pos = line->calculate_line_pos(0, INT_MAX, event.x - 1 + leftcol, 0);
		if ((event.modifier_state & EMOUSE_SHIFT) != 0)
			set_selection_end();
		ensure_cursor_on_screen();
	} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_MIDDLE)) {
		linked_ptr<string> primary;

		reset_selection();
		pos = line->calculate_line_pos(0, INT_MAX, event.x - 1 + leftcol, 0);

		primary = get_primary();
		if (primary != NULL) {
			text_line_t insert_line(primary);

			line->insert(&insert_line, pos);
			pos += insert_line.get_length();
		}
		ensure_cursor_on_screen();
	} else if ((event.type == EMOUSE_MOTION && (event.button_state & EMOUSE_BUTTON_LEFT)) ||
			(event.type == EMOUSE_BUTTON_RELEASE && (event.previous_button_state & EMOUSE_BUTTON_LEFT))) {
		/* Complex handling here is required to prevent claiming the X11 selection
		   when no text is selected at all. The basic idea however is to start the
		   selection if none has been started yet, move the cursor and move the end
		   of the selection to the new cursor location. */
		int newpos = line->calculate_line_pos(0, INT_MAX, event.x - 1 + leftcol, 0);
		if (selection_mode == selection_mode_t::NONE && newpos != pos) {
			selection_mode = selection_mode_t::SHIFT;
			selection_start_pos = pos;
		}
		pos = newpos;
		if (selection_mode != selection_mode_t::NONE)
			set_selection_end(event.type == EMOUSE_BUTTON_RELEASE);
		ensure_cursor_on_screen();
		redraw = true;
	}
	dont_select_on_focus = true;
	return true;
}

void text_field_t::set_selection_end(bool update_primary) {
	selection_end_pos = pos;
	if (update_primary) {
		size_t start, length;
		if (selection_start_pos < selection_end_pos) {
			start = selection_start_pos;
			length = selection_end_pos - start;
		} else {
			start = selection_end_pos;
			length = selection_start_pos - start;
		}
		set_primary(new string(*line->get_data(), start, length));
	}
}

/*======================
  == drop_down_list_t ==
  ======================*/
text_field_t::drop_down_list_t::drop_down_list_t(text_field_t *_field) :
		top_idx(0), field(_field)
{
	if ((window = t3_win_new(NULL, 6, t3_win_get_width(_field->get_base_window()), 1, 0, INT_MIN)) == NULL)
		throw(-1);
	t3_win_set_anchor(window, field->get_base_window(), T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	focus = false;
	current = 0;
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
				field->redraw = true;
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

bool text_field_t::drop_down_list_t::set_size(optint height, optint width) {
	bool result;

	(void) height;

	result = t3_win_resize(window, 6, width);
	redraw = true;
	return result;
}

void text_field_t::drop_down_list_t::update_contents(void) {
	size_t i, idx;
	file_list_t *file_list;
	int width;

	if (completions == NULL)
		return;

	file_list = dynamic_cast<file_list_t *>(completions());
	width = t3_win_get_width(window);

	/* We don't optimize for the case when nothing changes, because almost all
	   update_contents calls will happen when something has changed. */
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
		text_line_t file_name_line((*completions)[idx]);
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

		file_name_line.paint_line(window, &info);

		if (file_list != NULL && file_list->is_dir(idx)) {
			int length = file_name_line.get_length();
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
		if (field->line->get_length() == 0)
			completions->reset_filter();
		else
			completions->set_filter(sigc::bind(sigc::ptr_fun(string_compare_filter), field->line->get_data()));
	}
}

void text_field_t::drop_down_list_t::set_autocomplete(string_list_base_t *_completions) {
	if (completions != NULL)
		delete completions;

	if (dynamic_cast<file_list_t *>(_completions) != NULL)
		completions = new filtered_file_list_t((file_list_t *) _completions);
	else
		completions = new filtered_string_list_t(_completions);
}

bool text_field_t::drop_down_list_t::empty(void) {
	return completions->size() == 0;
}

void text_field_t::drop_down_list_t::force_redraw(void) {}

}; // namespace
