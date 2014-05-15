/* Copyright (C) 2011-2013 G.P. Halkes
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

#include "widgets/label.h"

#include "main.h"
#include "dialogs/gotodialog.h"
#include "dialogs/finddialog.h"
#include "widgets/editwindow.h"
#include "util.h"
#include "colorscheme.h"
#include "internal.h"
#include "findcontext.h"
#include "wrapinfo.h"
#include "clipboard.h"
#include "log.h"

/* FIXME: implement Ctrl-up and Ctrl-down for shifting the window contents without the cursor. */

using namespace std;
namespace t3_widget {

goto_dialog_t *edit_window_t::goto_dialog;
sigc::connection edit_window_t::goto_connection;
find_dialog_t *edit_window_t::global_find_dialog;
sigc::connection edit_window_t::global_find_dialog_connection;
finder_t edit_window_t::global_finder;
replace_buttons_dialog_t *edit_window_t::replace_buttons;
sigc::connection edit_window_t::replace_buttons_connection;
sigc::connection edit_window_t::init_connected = connect_on_init(sigc::ptr_fun(edit_window_t::init));

const char *edit_window_t::ins_string[] = {"INS", "OVR"};
bool (text_buffer_t::*edit_window_t::proces_char[])(key_t) = { &text_buffer_t::insert_char, &text_buffer_t::overwrite_char};

void edit_window_t::init(bool _init) {
	if (_init) {
		/* Construct these from t3_widget::init, such that the locale is set correctly and
		   gettext therefore returns the correctly localized strings. */
		goto_dialog = new goto_dialog_t();
		global_find_dialog = new find_dialog_t();
		replace_buttons = new replace_buttons_dialog_t();
	} else {
		delete goto_dialog; goto_dialog = NULL;
		delete global_find_dialog; global_find_dialog = NULL;
		delete replace_buttons; replace_buttons = NULL;
	}
}

edit_window_t::edit_window_t(text_buffer_t *_text, const view_parameters_t *params) : impl(new implementation_t()), text(NULL)
{
	/* Register the unbacked window for mouse events, such that we can get focus
	   if the bottom line is clicked. */
	init_unbacked_window(11, 11, true);
	if ((impl->edit_window = t3_win_new(window, 10, 10, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(impl->edit_window);

	if ((impl->indicator_window = t3_win_new(window, 1, 10, 0, 0, 0)) == NULL)
		throw bad_alloc();

	t3_win_set_anchor(impl->indicator_window, window, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	t3_win_show(impl->indicator_window);

	if ((info_window = t3_win_new(window, 1, 1, 0, 0, 1)) == NULL)
		throw bad_alloc();

	t3_win_set_anchor(info_window, window, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	t3_win_show(info_window);

	impl->scrollbar = new scrollbar_t(true);
	set_widget_parent(impl->scrollbar);
	impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	impl->scrollbar->set_size(10, None);
	impl->scrollbar->connect_clicked(sigc::mem_fun(this, &edit_window_t::scrollbar_clicked));
	impl->scrollbar->connect_dragged(sigc::mem_fun(this, &edit_window_t::scrollbar_dragged));

	set_text(_text == NULL ? new text_buffer_t() : _text, params);

	impl->screen_pos = 0;
	impl->focus = false;

	impl->autocomplete_panel = new autocomplete_panel_t(this);
	impl->autocomplete_panel->connect_activate(sigc::mem_fun(this, &edit_window_t::autocomplete_activated));
}

edit_window_t::~edit_window_t(void) {
	delete impl->wrap_info;
}

void edit_window_t::set_text(text_buffer_t *_text, const view_parameters_t *params) {
	if (text == _text)
		return;

	text = _text;
	if (params != NULL) {
		params->apply_parameters(this);
	} else {
		if (impl->wrap_info != NULL) {
			impl->wrap_info->set_text_buffer(text);
			impl->wrap_info->set_wrap_width(t3_win_get_width(impl->edit_window) - 1);
		}
		impl->top_left.line = 0;
		impl->top_left.pos = 0;
		impl->last_set_pos = 0;
	}

	ensure_cursor_on_screen();
	draw_info_window();
	update_repaint_lines(0, INT_MAX);
}

bool edit_window_t::set_size(optint height, optint width) {
	bool result = true;
	//FIXME: these int's are optional!!! Take that into account below!

	if (width != t3_win_get_width(window) || height > t3_win_get_height(window))
		update_repaint_lines(0, INT_MAX);

	result &= t3_win_resize(window, height, width);
	result &= t3_win_resize(impl->edit_window, height - 1, width - 1);
	result &= impl->scrollbar->set_size(height - 1, None);

	if (impl->wrap_type != wrap_type_t::NONE) {
		impl->top_left.pos = impl->wrap_info->calculate_line_pos(impl->top_left.line, 0, impl->top_left.pos);
		impl->wrap_info->set_wrap_width(width - 1);
		impl->top_left.pos = impl->wrap_info->find_line(impl->top_left);
		impl->last_set_pos = impl->wrap_info->calculate_screen_pos();
	}
	ensure_cursor_on_screen();
	return result;
}

void edit_window_t::ensure_cursor_on_screen(void) {
	int width;

	if (text->cursor.pos == text->get_line_max(text->cursor.line))
		width = 1;
	else
		width = text->width_at_cursor();


	if (impl->wrap_type == wrap_type_t::NONE) {
		impl->screen_pos = text->calculate_screen_pos(NULL, impl->tabsize);

		if (text->cursor.line < impl->top_left.line) {
			impl->top_left.line = text->cursor.line;
			update_repaint_lines(0, INT_MAX);
		}

		if (text->cursor.line >= impl->top_left.line + t3_win_get_height(impl->edit_window)) {
			impl->top_left.line = text->cursor.line - t3_win_get_height(impl->edit_window) + 1;
			update_repaint_lines(0, INT_MAX);
		}

		if (impl->screen_pos < impl->top_left.pos) {
			impl->top_left.pos = impl->screen_pos;
			update_repaint_lines(0, INT_MAX);
		}

		if (impl->screen_pos + width > impl->top_left.pos + t3_win_get_width(impl->edit_window)) {
			impl->top_left.pos = impl->screen_pos + width - t3_win_get_width(impl->edit_window);
			update_repaint_lines(0, INT_MAX);
		}
	} else {
		text_coordinate_t bottom;
		int sub_line = impl->wrap_info->find_line(text->cursor);
		impl->screen_pos = impl->wrap_info->calculate_screen_pos();

		if (text->cursor.line < impl->top_left.line || (text->cursor.line == impl->top_left.line && sub_line < impl->top_left.pos)) {
			impl->top_left.line = text->cursor.line;
			impl->top_left.pos = sub_line;
			update_repaint_lines(0, INT_MAX);
		} else {
			bottom = impl->top_left;
			impl->wrap_info->add_lines(bottom, t3_win_get_height(impl->edit_window) - 1);

			while (text->cursor.line > bottom.line) {
				impl->wrap_info->add_lines(impl->top_left, impl->wrap_info->get_line_count(bottom.line) - bottom.pos);
				bottom.line++;
				bottom.pos = 0;
				update_repaint_lines(0, INT_MAX);
			}

			if (text->cursor.line == bottom.line && sub_line > bottom.pos) {
				impl->wrap_info->add_lines(impl->top_left, sub_line - bottom.pos);
				update_repaint_lines(0, INT_MAX);
			}
		}
	}
}

void edit_window_t::repaint_screen(void) {
	text_coordinate_t current_start, current_end;
	text_line_t::paint_info_t info;
	int i;

	t3_win_set_default_attrs(impl->edit_window, attributes.text);

	update_repaint_lines(text->cursor.line, text->cursor.line);

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();

	if (current_end < current_start) {
		current_start = current_end;
		current_end = text->get_selection_start();
	}

	info.size = t3_win_get_width(impl->edit_window);
	info.tabsize = impl->tabsize;
	info.normal_attr = 0;
	info.selected_attr = attributes.text_selected;
	info.flags = impl->show_tabs ? text_line_t::SHOW_TABS : 0;

	if (impl->wrap_type == wrap_type_t::NONE) {
		info.leftcol = impl->top_left.pos;
		info.start = 0;
		info.max = INT_MAX;

		for (i = 0; i < t3_win_get_height(impl->edit_window) && (i + impl->top_left.line) < text->size(); i++) {
			if (impl->top_left.line + i < impl->repaint_min || impl->top_left.line + i > impl->repaint_max)
				continue;

			info.selection_start = impl->top_left.line + i == current_start.line ? current_start.pos : -1;
			if (impl->top_left.line + i >= current_start.line) {
				if (impl->top_left.line + i < current_end.line)
					info.selection_end = INT_MAX;
				else if (impl->top_left.line + i == current_end.line)
					info.selection_end = current_end.pos;
				else
					info.selection_end = -1;
			} else {
				info.selection_end = -1;
			}

			info.cursor = impl->focus && impl->top_left.line + i == text->cursor.line ? text->cursor.pos : -1;
			t3_win_set_paint(impl->edit_window, i, 0);
			t3_win_clrtoeol(impl->edit_window);
			text->paint_line(impl->edit_window, impl->top_left.line + i, &info);
		}
	} else {
		text_coordinate_t end_coord = impl->wrap_info->get_end();
		text_coordinate_t draw_line = impl->top_left;
		info.leftcol = 0;

		for (i = 0; i < t3_win_get_height(impl->edit_window); i++, impl->wrap_info->add_lines(draw_line, 1)) {
			if (draw_line.line < impl->repaint_min || draw_line.line > impl->repaint_max)
				continue;
			info.selection_start = draw_line.line == current_start.line ? current_start.pos : -1;
			if (draw_line.line >= current_start.line) {
				if (draw_line.line < current_end.line)
					info.selection_end = INT_MAX;
				else if (draw_line.line == current_end.line)
					info.selection_end = current_end.pos;
				else
					info.selection_end = -1;
			} else {
				info.selection_end = -1;
			}

			info.cursor = impl->focus && draw_line.line == text->cursor.line ? text->cursor.pos : -1;
			t3_win_set_paint(impl->edit_window, i, 0);
			t3_win_clrtoeol(impl->edit_window);
			impl->wrap_info->paint_line(impl->edit_window, draw_line, &info);

			if (draw_line.line == end_coord.line && draw_line.pos == end_coord.pos) {
				/* Increase i, to make sure this line is not erased. */
				i++;
				break;
			}
		}
	}
	/* Clear the bottom part of the window (if applicable). */
	t3_win_set_paint(impl->edit_window, i, 0);
	t3_win_clrtobot(impl->edit_window);

	impl->repaint_min = text->cursor.line;
	impl->repaint_max = text->cursor.line;
}

void edit_window_t::inc_x(void) {
	if (text->cursor.pos == text->get_line_max(text->cursor.line)) {
		if (text->cursor.line >= text->size() - 1)
			return;

		text->cursor.line++;
		text->cursor.pos = 0;
	} else {
		text->adjust_position(1);
	}
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::next_word(void) {
	text->goto_next_word();
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::dec_x(void) {
	if (text->cursor.pos == 0) {
		if (text->cursor.line == 0)
			return;

		text->cursor.line--;
		text->cursor.pos = text->get_line_max(text->cursor.line);
	} else {
		text->adjust_position(-1);
	}
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::previous_word(void) {
	text->goto_previous_word();
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::inc_y(void) {
	if (impl->wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line + 1 < text->size()) {
			text->cursor.line++;
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, impl->last_set_pos, impl->tabsize);
			ensure_cursor_on_screen();
		} else {
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		}
	} else {
		int new_sub_line = impl->wrap_info->find_line(text->cursor) + 1;
		if (impl->wrap_info->get_line_count(text->cursor.line) == new_sub_line) {
			if (text->cursor.line + 1 < text->size()) {
				text->cursor.line++;
				text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos, 0);
				ensure_cursor_on_screen();
			} else {
				text->cursor.pos = text->get_line_max(text->cursor.line);
				ensure_cursor_on_screen();
				impl->last_set_pos = impl->screen_pos;
			}
		} else {
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos, new_sub_line);
			ensure_cursor_on_screen();
		}
	}
}

void edit_window_t::dec_y(void) {
	if (impl->wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line > 0) {
			text->cursor.line--;
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, impl->last_set_pos, impl->tabsize);
			ensure_cursor_on_screen();
		} else {
			impl->last_set_pos = text->cursor.pos = 0;
			ensure_cursor_on_screen();
		}
	} else {
		int sub_line = impl->wrap_info->find_line(text->cursor);
		if (sub_line > 0) {
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos, sub_line - 1);
			ensure_cursor_on_screen();
		} else if (text->cursor.line > 0) {
			text->cursor.line--;
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos,
				impl->wrap_info->get_line_count(text->cursor.line) - 1);
			ensure_cursor_on_screen();
		} else {
			text->cursor.pos = 0;
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		}
	}
}

void edit_window_t::pgdn(void) {
	bool need_adjust = true;

	if (impl->wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line + t3_win_get_height(impl->edit_window) - 1 < text->size()) {
			text->cursor.line += t3_win_get_height(impl->edit_window) - 1;
		} else {
			text->cursor.line = text->size() - 1;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			need_adjust = false;
		}

		/* If the end of the text is already on the screen, don't change the top line. */
		if (impl->top_left.line + t3_win_get_height(impl->edit_window) < text->size()) {
			impl->top_left.line += t3_win_get_height(impl->edit_window) - 1;
			if (impl->top_left.line + t3_win_get_height(impl->edit_window) > text->size())
				impl->top_left.line = text->size() - t3_win_get_height(impl->edit_window);
			update_repaint_lines(0, INT_MAX);
		}

		if (need_adjust)
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, impl->last_set_pos, impl->tabsize);

	} else {
		text_coordinate_t new_top_left = impl->top_left;
		text_coordinate_t new_cursor(text->cursor.line, impl->wrap_info->find_line(text->cursor));

		if (impl->wrap_info->add_lines(new_cursor, t3_win_get_height(impl->edit_window) - 1)) {
			text->cursor.line = new_cursor.line;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			need_adjust = false;
		} else {
			text->cursor.line = new_cursor.line;
		}

		/* If the end of the text is already on the screen, don't change the top line. */
		if (!impl->wrap_info->add_lines(new_top_left, t3_win_get_height(impl->edit_window))) {
			impl->top_left = new_top_left;
			impl->wrap_info->sub_lines(impl->top_left, 1);
			update_repaint_lines(0, INT_MAX);
		}

		if (need_adjust)
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos, new_cursor.pos);
	}
	ensure_cursor_on_screen();

	if (!need_adjust)
		impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::pgup(void) {
	bool need_adjust = true;

	if (impl->wrap_type == wrap_type_t::NONE) {
		if (impl->top_left.line < t3_win_get_height(impl->edit_window) - 1) {
			if (impl->top_left.line != 0) {
				impl->top_left.line = 0;
				update_repaint_lines(0, INT_MAX);
			}

			if (text->cursor.line < t3_win_get_height(impl->edit_window) - 1) {
				text->cursor.line = 0;
				impl->last_set_pos = text->cursor.pos = 0;
				need_adjust = false;
			} else {
				text->cursor.line -= t3_win_get_height(impl->edit_window) - 1;
			}
		} else {
			text->cursor.line -= t3_win_get_height(impl->edit_window) - 1;
			impl->top_left.line -= t3_win_get_height(impl->edit_window) - 1;
			update_repaint_lines(0, INT_MAX);
		}

		if (need_adjust)
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, impl->last_set_pos, impl->tabsize);
	} else {
		text_coordinate_t new_cursor(text->cursor.line, impl->wrap_info->find_line(text->cursor));

		if (impl->wrap_info->sub_lines(new_cursor, t3_win_get_height(impl->edit_window) - 1)) {
			text->cursor.line = 0;
			text->cursor.pos = 0;
			impl->last_set_pos = 0;
			need_adjust = false;
		} else {
			text->cursor.line = new_cursor.line;
		}

		impl->wrap_info->sub_lines(impl->top_left, t3_win_get_height(impl->edit_window) - 1);
		update_repaint_lines(0, INT_MAX);

		if (need_adjust)
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, impl->last_set_pos, new_cursor.pos);
	}
	ensure_cursor_on_screen();
}

void edit_window_t::home_key(void) {
	const text_line_t *line;
	int pos;

	if (!impl->indent_aware_home) {
		text->cursor.pos = impl->wrap_type == wrap_type_t::NONE ? 0 :
			impl->wrap_info->calculate_line_pos(text->cursor.line, 0, impl->wrap_info->find_line(text->cursor));
		ensure_cursor_on_screen();
		impl->last_set_pos = impl->screen_pos;
		return;
	}

	if (impl->wrap_type != wrap_type_t::NONE) {
		pos = impl->wrap_info->calculate_line_pos(text->cursor.line, 0, impl->wrap_info->find_line(text->cursor));
		if (pos != text->cursor.pos) {
			text->cursor.pos = pos;
			impl->screen_pos = impl->last_set_pos = 0;
			return;
		}
	}
	line = text->get_line_data(text->cursor.line);
	for (pos = 0; pos < line->get_length() && line->is_space(pos); pos = line->adjust_position(pos, 1)) {}

	text->cursor.pos = text->cursor.pos != pos ? pos : 0;
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::end_key(void) {
	if (impl->wrap_type == wrap_type_t::NONE) {
		text->cursor.pos = text->get_line_max(text->cursor.line);
	} else {
		int sub_line = impl->wrap_info->find_line(text->cursor);
		if (sub_line + 1 < impl->wrap_info->get_line_count(text->cursor.line)) {
			int before_pos = text->cursor.pos;
			text->cursor.pos = impl->wrap_info->calculate_line_pos(text->cursor.line, 0, sub_line + 1);
			text->adjust_position(-1);
			if (before_pos == text->cursor.pos)
				text->cursor.pos = text->get_line_max(text->cursor.line);
		} else {
			text->cursor.pos = text->get_line_max(text->cursor.line);
		}
	}
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::reset_selection(void) {
	update_repaint_lines(text->get_selection_start().line, text->get_selection_end().line);
	text->set_selection_mode(selection_mode_t::NONE);
}

bool edit_window_t::set_selection_mode(key_t key) {
	selection_mode_t selection_mode = text->get_selection_mode();
	switch (key & ~(EKEY_CTRL | EKEY_META | EKEY_SHIFT)) {
		case EKEY_END:
		case EKEY_HOME:
		case EKEY_PGUP:
		case EKEY_PGDN:
		case EKEY_LEFT:
		case EKEY_RIGHT:
		case EKEY_UP:
		case EKEY_DOWN:
			if ((selection_mode == selection_mode_t::SHIFT || selection_mode == selection_mode_t::ALL) && !(key & EKEY_SHIFT)) {
				bool done = false;
				if (key == EKEY_LEFT) {
					if (text->get_selection_start() < text->get_selection_end())
						text->cursor = text->get_selection_start();
					else
						text->cursor = text->get_selection_end();
					done = true;
				} else if (key == EKEY_RIGHT) {
					if (text->get_selection_start() < text->get_selection_end())
						text->cursor = text->get_selection_end();
					else
						text->cursor = text->get_selection_start();
					done = true;
				}
				reset_selection();
				return done;
			} else if ((key & EKEY_SHIFT) && selection_mode != selection_mode_t::MARK) {
				text->set_selection_mode(selection_mode_t::SHIFT);
			}
			break;
		default:
			break;
	}
	return false;
}

void edit_window_t::delete_selection(void) {
	text_coordinate_t current_start, current_end;

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();
	text->delete_block(current_start, current_end);

	update_repaint_lines(current_start.line < current_end.line ? current_start.line : current_end.line, INT_MAX);
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
	reset_selection();
}

void edit_window_t::find_activated(find_action_t action, finder_t *_finder) {
	finder_t *local_finder;
	find_result_t result;

	local_finder = impl->finder == NULL ? &global_finder : impl->finder;
	if (_finder != NULL)
		*local_finder = *_finder;

	switch (action) {
		case find_action_t::FIND:
			result.start = text->cursor;

			if (!text->find(local_finder, &result))
				goto not_found;

			text->set_selection_from_find(&result);
			update_repaint_lines(result.start.line, result.end.line);
			ensure_cursor_on_screen();
			if (local_finder->get_flags() & find_flags_t::REPLACEMENT_VALID) {
				replace_buttons_connection.disconnect();
				replace_buttons_connection = replace_buttons->connect_activate(
					sigc::bind(sigc::mem_fun(this, &edit_window_t::find_activated), (finder_t *) NULL));
				replace_buttons->center_over(center_window);
				replace_buttons->show();
			}
			break;
		case find_action_t::REPLACE:
			result.start = text->get_selection_start();
			result.end = text->get_selection_end();
			text->replace(local_finder, &result);
			update_repaint_lines(result.start.line < result.end.line ? result.start.line : result.end.line, INT_MAX);
			/* FALLTHROUGH */
			if (0) {
		case find_action_t::SKIP:
				/* This part is skipped when the action is replace */
				result.start = text->get_selection_start();
			}
			if (!text->find(local_finder, &result)) {
				ensure_cursor_on_screen();
				goto not_found;
			}

			text->set_selection_from_find(&result);
			update_repaint_lines(result.start.line, result.end.line);
			ensure_cursor_on_screen();
			replace_buttons->reshow(action);
			break;
		case find_action_t::REPLACE_ALL: {
			int replacements;
			text_coordinate_t start(0, 0);
			text_coordinate_t eof(INT_MAX, INT_MAX);

			for (replacements = 0; text->find_limited(local_finder, start, eof, &result); replacements++) {
				if (replacements == 0)
					text->start_undo_block();
				text->replace(local_finder, &result);
				start = text->cursor;
			}

			if (replacements == 0)
				goto not_found;

			text->end_undo_block();
			reset_selection();
			ensure_cursor_on_screen();
			update_repaint_lines(0, INT_MAX);
			break;
		}
		case find_action_t::REPLACE_IN_SELECTION: {
			if (text->selection_empty())
				return;

			text_coordinate_t start(text->get_selection_start());
			text_coordinate_t end(text->get_selection_end());
			text_coordinate_t saved_start;
			int replacements;
			int end_line_length;
			int reverse_selection = false;

			if (end < start) {
				start = text->get_selection_end();
				end = text->get_selection_start();
				reverse_selection = true;
			}
			end_line_length = text->get_line_max(end.line);
			saved_start = start;

			for (replacements = 0; text->find_limited(local_finder, start, end, &result); replacements++) {
				if (replacements == 0)
					text->start_undo_block();
				text->replace(local_finder, &result);
				start = text->cursor;
				end.pos -= end_line_length - text->get_line_max(end.line);
				end_line_length = text->get_line_max(end.line);
			}

			if (replacements == 0)
				goto not_found;

			text->end_undo_block();

			text->set_selection_mode(selection_mode_t::NONE);
			if (reverse_selection) {
				text->cursor = end;
				text->set_selection_mode(selection_mode_t::SHIFT);
				text->cursor = saved_start;
				text->set_selection_end();
			} else {
				text->cursor = saved_start;
				text->set_selection_mode(selection_mode_t::SHIFT);
				text->cursor = end;
				text->set_selection_end();
			}

			ensure_cursor_on_screen();
			update_repaint_lines(0, INT_MAX);
			break;
		}
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
	if (set_selection_mode(key))
		return true;

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
			home_key();
			break;
		case EKEY_HOME | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_HOME | EKEY_CTRL:
			impl->screen_pos = impl->last_set_pos = text->cursor.pos = 0;
			text->cursor.line = 0;
			if ((impl->wrap_type == wrap_type_t::NONE && impl->top_left.pos != 0) || impl->top_left.line != 0)
				ensure_cursor_on_screen();
			break;
		case EKEY_END | EKEY_SHIFT:
		case EKEY_END:
			end_key();
			break;
		case EKEY_END | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_END | EKEY_CTRL: {
			text->cursor.line = text->size() - 1;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
			break;
		}
		case EKEY_INS:
			impl->ins_mode ^= 1;
			break;

		case EKEY_DEL:
			if (text->get_selection_mode() == selection_mode_t::NONE) {
				if (text->cursor.pos != text->get_line_max(text->cursor.line)) {
					text->delete_char();

					if (impl->wrap_type != wrap_type_t::NONE)
						ensure_cursor_on_screen();

					update_repaint_lines(text->cursor.line, text->cursor.line);
				} else if (text->cursor.line + 1 < text->size()) {
					text->merge(false);

					if (impl->wrap_type != wrap_type_t::NONE)
						ensure_cursor_on_screen();

					update_repaint_lines(text->cursor.line, INT_MAX);
				}
			} else {
				delete_selection();
			}
			break;

		case EKEY_NL: {
			const string *current_line;
			int i, indent, tabs;
			string space;

			if (text->get_selection_mode() != selection_mode_t::NONE)
				delete_selection();

			update_repaint_lines(text->cursor.line, INT_MAX);
			if (impl->auto_indent) {
				current_line = text->get_line_data(text->cursor.line)->get_data();
				for (i = 0, indent = 0, tabs = 0; i < text->cursor.pos; i++) {
					if ((*current_line)[i] == '\t') {
						indent = 0;
						tabs++;
					} else if ((*current_line)[i] == ' ') {
						indent++;
						if (indent == impl->tabsize) {
							indent = 0;
							tabs++;
						}
					} else {
						break;
					}
				}
				if (impl->tab_spaces)
					space.append(impl->tabsize * tabs, ' ');
				else
					space.append(tabs, '\t');

				if (indent != 0)
					space.append(indent, ' ');

				text->break_line(&space);
			} else {
				text->break_line();
			}
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
			break;
		}
		case EKEY_BS:
			if (text->get_selection_mode() == selection_mode_t::NONE) {
				if (text->cursor.pos <= text->get_line_max(text->cursor.line)) {
					if (text->cursor.pos != 0) {
						text->backspace_char();
						update_repaint_lines(text->cursor.line, text->cursor.line);
					} else if (text->cursor.line != 0) {
						text->merge(true);
						update_repaint_lines(text->cursor.line, text->cursor.line);
					}
				} else {
					ASSERT(0);
				}
				ensure_cursor_on_screen();
				impl->last_set_pos = impl->screen_pos;
			} else {
				delete_selection();
			}
			if (impl->autocomplete_panel->is_shown())
				activate_autocomplete(false);
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

		case 0: /* CTRL-space and others */
			activate_autocomplete(true);
			break;

		case EKEY_CTRL | 't':
			switch (text->get_selection_mode()) {
				case selection_mode_t::MARK:
					reset_selection();
					break;
				case selection_mode_t::NONE:
				case selection_mode_t::ALL:
				case selection_mode_t::SHIFT:
					text->set_selection_mode(selection_mode_t::MARK);
					break;
				default:
					/* Should not happen, but just try to get back to a sane state. */
					reset_selection();
					break;
			}
			break;
		case EKEY_ESC:
			if (text->get_selection_mode() == selection_mode_t::MARK)
				reset_selection();
			break;

		case EKEY_CTRL | 'f':
		case EKEY_CTRL | 'r':
			find_replace(key == (EKEY_CTRL | 'r'));
			break;

		case EKEY_F3:
		case EKEY_META | '3':
			find_next(false);
			break;
		case EKEY_F3 | EKEY_SHIFT:
			find_next(true);
			break;

		case EKEY_F9:
		case EKEY_META | '9':
			insert_special();
			break;

		case EKEY_SHIFT | '\t':
			if (text->get_selection_mode() != selection_mode_t::NONE &&
					text->get_selection_start().line != text->get_selection_end().line)
			{
				unindent_selection();
			} else {
				text->unindent_line(impl->tabsize);
				ensure_cursor_on_screen();
				impl->last_set_pos = impl->screen_pos;
				update_repaint_lines(text->cursor.line, text->cursor.line);
			}
			break;
		case '\t':
			if (text->get_selection_mode() != selection_mode_t::NONE &&
					text->get_selection_start().line != text->get_selection_end().line)
			{
				indent_selection();
			} else {
				string space;
				if (text->get_selection_mode() != selection_mode_t::NONE)
					delete_selection();

				if (impl->tab_spaces)
					space.append(impl->tabsize - (impl->screen_pos % impl->tabsize), ' ');
				else
					space.append(1, '\t');
				text->insert_block(&space);
				ensure_cursor_on_screen();
				impl->last_set_pos = impl->screen_pos;
			}
			break;
		default: {
			int local_insmode;

			if (key < 32)
				return false;

			key &= ~EKEY_PROTECT;
			if (key == 10)
				return false;

			if (key >= EKEY_FIRST_SPECIAL)
				return false;

			local_insmode = impl->ins_mode;
			if (text->get_selection_mode() != selection_mode_t::NONE) {
				delete_selection();
				local_insmode = 0;
			}

			(text->*proces_char[local_insmode])(key);
			ensure_cursor_on_screen();
			update_repaint_lines(text->cursor.line, text->cursor.line);
			impl->last_set_pos = impl->screen_pos;
			if (impl->autocomplete_panel->is_shown())
				activate_autocomplete(false);
			return true;
		}
	}
	return true;
}

void edit_window_t::update_contents(void) {
	text_coordinate_t logical_cursor_pos;
	char info[30];
	int info_width, name_width;
	selection_mode_t selection_mode;

	/* TODO: see if we can optimize this somewhat by not redrawing the whole thing
	   on every key.

	   - cursor-only movements mostly don't require entire redraws [well, that depends:
	      for matching brace/parenthesis it may require more than a single line update]
	*/
	if (!impl->focus && !redraw)
		return;

	selection_mode = text->get_selection_mode();
	if (selection_mode != selection_mode_t::NONE && selection_mode != selection_mode_t::ALL) {
		text->set_selection_end();

		if (selection_mode == selection_mode_t::SHIFT) {
			if (text->selection_empty())
				reset_selection();
		}
	}

	redraw = false;
	repaint_screen();

	t3_win_set_default_attrs(impl->indicator_window, attributes.menubar);
	t3_win_set_paint(impl->indicator_window, 0, 0);
	t3_win_addchrep(impl->indicator_window, ' ', 0, t3_win_get_width(impl->indicator_window));

	if (impl->wrap_type == wrap_type_t::NONE) {
		impl->scrollbar->set_parameters(max(text->size(), impl->top_left.line + t3_win_get_height(impl->edit_window)),
			impl->top_left.line, t3_win_get_height(impl->edit_window));
	} else {
		int i, count = 0;
		for (i = 0; i < impl->top_left.line; i++)
			count += impl->wrap_info->get_line_count(i);
		count += impl->top_left.pos;

		impl->scrollbar->set_parameters(max(impl->wrap_info->get_size(), count + t3_win_get_height(impl->edit_window)),
			count, t3_win_get_height(impl->edit_window));
	}
	impl->scrollbar->update_contents();

	logical_cursor_pos = text->cursor;
	logical_cursor_pos.pos = text->calculate_screen_pos(NULL, impl->tabsize);

	snprintf(info, 29, "L: %-4d C: %-4d %c %s", logical_cursor_pos.line + 1, logical_cursor_pos.pos + 1,
		text->is_modified() ? '*' : ' ', ins_string[impl->ins_mode]);
	info_width = t3_term_strwidth(info);
	t3_win_resize(impl->indicator_window, 1, info_width + 3);
	name_width = t3_win_get_width(window) - t3_win_get_width(impl->indicator_window);

	if (t3_win_get_width(info_window) != name_width && name_width > 0) {
		t3_win_resize(info_window, 1, name_width);
		draw_info_window();
	}

	t3_win_set_paint(impl->indicator_window, 0, t3_win_get_width(impl->indicator_window) - info_width - 1);
	t3_win_addstr(impl->indicator_window, info, 0);
}

void edit_window_t::set_focus(focus_t _focus) {
	if (_focus != impl->focus) {
		impl->focus = _focus;
		impl->autocomplete_panel->hide();
		update_repaint_lines(text->cursor.line, text->cursor.line);
	}
}

void edit_window_t::undo(void) {
	if (text->apply_undo() == 0) {
		update_repaint_lines(0, INT_MAX);
		ensure_cursor_on_screen();
		impl->last_set_pos = impl->screen_pos;
	}
}

void edit_window_t::redo(void) {
	if (text->apply_redo() == 0) {
		update_repaint_lines(0, INT_MAX);
		ensure_cursor_on_screen();
		impl->last_set_pos = impl->screen_pos;
	}
}

void edit_window_t::cut_copy(bool cut) {
	if (text->get_selection_mode() != selection_mode_t::NONE) {
		if (text->selection_empty()) {
			reset_selection();
			return;
		}

		set_clipboard(text->convert_block(text->get_selection_start(), text->get_selection_end()));

		if (cut)
			delete_selection();
		else if (text->get_selection_mode() == selection_mode_t::MARK)
			text->set_selection_mode(selection_mode_t::SHIFT);
	}
}

void edit_window_t::paste(void) {
	WITH_CLIPBOARD_LOCK(
		linked_ptr<string>::t copy_buffer = get_clipboard();
		if (copy_buffer != NULL) {
			if (text->get_selection_mode() == selection_mode_t::NONE) {
				update_repaint_lines(text->cursor.line, INT_MAX);
				text->insert_block(copy_buffer);
			} else {
				text_coordinate_t current_start;
				text_coordinate_t current_end;
				current_start = text->get_selection_start();
				current_end = text->get_selection_end();
				update_repaint_lines(current_start.line < current_end.line ? current_start.line : current_end.line, INT_MAX);
				text->replace_block(current_start, current_end, copy_buffer);
				reset_selection();
			}
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		}
	)
}

void edit_window_t::select_all(void) {
	text->set_selection_mode(selection_mode_t::ALL);
	update_repaint_lines(0, INT_MAX);
}

void edit_window_t::insert_special(void) {
	insert_char_dialog->center_over(center_window);
	insert_char_dialog->reset();
	insert_char_dialog->show();
}

void edit_window_t::indent_selection(void) {
	text->indent_selection(impl->tabsize, impl->tab_spaces);
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
	update_repaint_lines(text->get_selection_start().line , text->get_selection_end().line);
}

void edit_window_t::unindent_selection(void) {
	text->unindent_selection(impl->tabsize);
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
	update_repaint_lines(text->get_selection_start().line , text->get_selection_end().line);
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

	reset_selection();
	text->cursor.line = (line > text->size() ? text->size() : line) - 1;
	if (text->cursor.pos > text->get_line_max(text->cursor.line))
		text->cursor.pos = text->get_line_max(text->cursor.line);
	ensure_cursor_on_screen();
	impl->last_set_pos = impl->screen_pos;
}

void edit_window_t::find_replace(bool replace) {
	find_dialog_t *dialog;
	if (impl->find_dialog == NULL) {
		global_find_dialog_connection.disconnect();
		global_find_dialog_connection = global_find_dialog->connect_activate(
			sigc::mem_fun(this, &edit_window_t::find_activated));
		dialog = global_find_dialog;
	} else {
		dialog = impl->find_dialog;
	}
	dialog->center_over(center_window);
	dialog->set_replace(replace);

	if (!text->selection_empty() && text->get_selection_start().line == text->get_selection_end().line)
		dialog->set_text(text->convert_block(text->get_selection_start(), text->get_selection_end()));
	dialog->show();
}

void edit_window_t::find_next(bool backward) {
	find_result_t result;
	if (text->get_selection_mode() == selection_mode_t::NONE) {
		result.start = text->cursor;

	} else {
		if (text->get_selection_start() < text->get_selection_end())
			result.start = text->get_selection_start();
		else
			result.start = text->get_selection_end();
	}

	if (!text->find(impl->finder != NULL ? impl->finder : &global_finder, &result, backward)) {
		//FIXME: show search string
		message_dialog->set_message("Search string not found");
		message_dialog->center_over(center_window);
		message_dialog->show();
	} else {
		text->set_selection_from_find(&result);
	}
	ensure_cursor_on_screen();
}

text_buffer_t *edit_window_t::get_text(void) const {
	return text;
}

void edit_window_t::set_find_dialog(find_dialog_t *_find_dialog) {
	impl->find_dialog = _find_dialog;
}

void edit_window_t::set_finder(finder_t *_finder) {
	impl->finder = _finder;
}

void edit_window_t::force_redraw(void) {
	widget_t::force_redraw();
	update_repaint_lines(0, INT_MAX);
	draw_info_window();
	ensure_cursor_on_screen();
}

void edit_window_t::set_tabsize(int _tabsize) {
	if (_tabsize == impl->tabsize)
		return;
	impl->tabsize = _tabsize;
	if (impl->wrap_info != NULL)
		impl->wrap_info->set_tabsize(impl->tabsize);
	force_redraw();
}

void edit_window_t::bad_draw_recheck(void) {
	widget_t::force_redraw();
}

void edit_window_t::set_child_focus(window_component_t *target) {
	(void) target;
	set_focus(window_component_t::FOCUS_SET);
}

bool edit_window_t::is_child(window_component_t *widget) {
	return widget == impl->scrollbar;
}

bool edit_window_t::process_mouse_event(mouse_event_t event) {
	if (event.window == impl->edit_window) {
		if (event.button_state & EMOUSE_TRIPLE_CLICKED_LEFT) {
			text->cursor.pos = 0;
			text->set_selection_mode(selection_mode_t::SHIFT);
			text->cursor.pos = text->get_line_max(text->cursor.line);
			text->set_selection_end(true);
		} else if (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) {
			text->goto_previous_word_boundary();
			text->set_selection_mode(selection_mode_t::SHIFT);
			text->goto_next_word_boundary();
			text->set_selection_end(true);
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT) &&
				event.previous_button_state == 0)
		{
			if ((event.modifier_state & EMOUSE_SHIFT) == 0)
				reset_selection();
			else if (text->get_selection_mode() == selection_mode_t::NONE)
				text->set_selection_mode(selection_mode_t::SHIFT);
			text->cursor = xy_to_text_coordinate(event.x, event.y);
			if ((event.modifier_state & EMOUSE_SHIFT) != 0)
				text->set_selection_end();
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_MIDDLE)) {
			reset_selection();
			text->cursor = xy_to_text_coordinate(event.x, event.y);
			WITH_CLIPBOARD_LOCK(
				linked_ptr<string>::t primary = get_primary();
				if (primary != NULL)
					text->insert_block(primary);
			)
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
			scroll(event.button_state & EMOUSE_SCROLL_UP ? -3 : 3);
		} else if ((event.type == EMOUSE_MOTION && (event.button_state & EMOUSE_BUTTON_LEFT)) ||
				(event.type == EMOUSE_BUTTON_RELEASE && (event.previous_button_state & EMOUSE_BUTTON_LEFT))) {
			/* Complex handling here is required to prevent claiming the X11 selection
			   when no text is selected at all. The basic idea however is to start the
			   selection if none has been started yet, move the cursor and move the end
			   of the selection to the new cursor location. */
			text_coordinate_t new_cursor = xy_to_text_coordinate(event.x, event.y);
			if (text->get_selection_mode() == selection_mode_t::NONE && new_cursor != text->cursor)
				text->set_selection_mode(selection_mode_t::SHIFT);
			text->cursor = new_cursor;
			if (text->get_selection_mode() != selection_mode_t::NONE)
				text->set_selection_end(event.type == EMOUSE_BUTTON_RELEASE);
			ensure_cursor_on_screen();
			impl->last_set_pos = impl->screen_pos;
		}
	}
	return true;
}

void edit_window_t::set_wrap(wrap_type_t wrap) {
	if (wrap == impl->wrap_type)
		return;

	if (wrap == wrap_type_t::NONE) {
		impl->top_left.pos = 0;
		if (impl->wrap_info != NULL) {
			delete impl->wrap_info;
			impl->wrap_info = NULL;
		}
	} else {
		//FIXME: differentiate between wrap types
		if (impl->wrap_info == NULL)
			impl->wrap_info = new wrap_info_t(t3_win_get_width(impl->edit_window) - 1, impl->tabsize);
		impl->wrap_info->set_text_buffer(text);
		impl->wrap_info->set_wrap_width(t3_win_get_width(impl->edit_window) - 1);
		impl->top_left.pos = impl->wrap_info->find_line(impl->top_left);
	}
	impl->wrap_type = wrap;
	update_repaint_lines(0, INT_MAX);
	ensure_cursor_on_screen();
}

void edit_window_t::set_tab_spaces(bool _tab_spaces) {
	impl->tab_spaces = _tab_spaces;
}

void edit_window_t::set_auto_indent(bool _auto_indent) {
	impl->auto_indent = _auto_indent;
}

void edit_window_t::set_indent_aware_home(bool _indent_aware_home) {
	impl->indent_aware_home = _indent_aware_home;
}

void edit_window_t::set_show_tabs(bool _show_tabs) {
	impl->show_tabs = _show_tabs;
}

int edit_window_t::get_tabsize(void) {
	return impl->tabsize;
}

wrap_type_t edit_window_t::get_wrap(void) {
	return impl->wrap_type;
}

bool edit_window_t::get_tab_spaces(void) {
	return impl->tab_spaces;
}

bool edit_window_t::get_auto_indent(void) {
	return impl->auto_indent;
}

bool edit_window_t::get_indent_aware_home(void) {
	return impl->indent_aware_home;
}

bool edit_window_t::get_show_tabs(void) {
	return impl->show_tabs;
}

edit_window_t::view_parameters_t *edit_window_t::save_view_parameters(void) {
	return new view_parameters_t(this);
}

void edit_window_t::save_view_parameters(view_parameters_t *params) {
	*params = view_parameters_t(this);
}

void edit_window_t::draw_info_window(void) {}

void edit_window_t::set_autocompleter(autocompleter_t *_autocompleter) {
	impl->autocomplete_panel->hide();
	impl->autocompleter = _autocompleter;
}

void edit_window_t::autocomplete(void) {
	activate_autocomplete(true);
}

void edit_window_t::activate_autocomplete(bool autocomplete_single) {
	if (impl->autocompleter == NULL)
		return;

	text_coordinate_t anchor(text->cursor);
	string_list_base_t *autocomplete_list = impl->autocompleter->build_autocomplete_list(text, &anchor.pos);

	if (autocomplete_list != NULL) {
		if (autocomplete_single && autocomplete_list->size() == 1) {
			impl->autocompleter->autocomplete(text, 0);
			/* Just in case... */
			impl->autocomplete_panel->hide();
			return;
		}

		impl->autocomplete_panel->set_completions(autocomplete_list);
		if (impl->wrap_type == wrap_type_t::NONE) {
			int position = text->calculate_screen_pos(&anchor, impl->tabsize);
			impl->autocomplete_panel->set_position(text->cursor.line - impl->top_left.line + 1,
				position - impl->top_left.pos - 1);
		} else {
			int sub_line = impl->wrap_info->find_line(text->cursor);
			int position = impl->wrap_info->calculate_screen_pos(&anchor);
			int line;

			if (text->cursor.line == impl->top_left.line) {
				line = sub_line - impl->top_left.pos;
			} else {
				line = impl->wrap_info->get_line_count(impl->top_left.line) - impl->top_left.pos + sub_line;
				for (int i = impl->top_left.line + 1; i < text->cursor.line; i++)
					line += impl->wrap_info->get_line_count(i);
			}
			impl->autocomplete_panel->set_position(line + 1, position - 1);
		}
		impl->autocomplete_panel->show();
	} else if (impl->autocomplete_panel->is_shown()) {
		impl->autocomplete_panel->hide();
	}
}

void edit_window_t::autocomplete_activated(void) {
	size_t idx = impl->autocomplete_panel->get_selected_idx();
	impl->autocomplete_panel->hide();
	impl->autocompleter->autocomplete(text, idx);
}

text_coordinate_t edit_window_t::xy_to_text_coordinate(int x, int y) {
	text_coordinate_t coord;
	if (impl->wrap_type == wrap_type_t::NONE) {
		coord.line = y + impl->top_left.line;
		x += impl->top_left.pos;
		if (coord.line >= text->size()) {
			coord.line = text->size() - 1;
			x = INT_MAX;
		}
		if (coord.line < 0) {
			coord.line = 0;
			coord.pos = 0;
		} else {
			coord.pos = text->calculate_line_pos(coord.line, x, impl->tabsize);
		}
	} else {
		coord.line = impl->top_left.line;
		y += impl->top_left.pos;
		while (coord.line < impl->wrap_info->get_size() - 1 && y >= impl->wrap_info->get_line_count(coord.line)) {
			y -= impl->wrap_info->get_line_count(coord.line);
			coord.line++;
		}
		if (y >= impl->wrap_info->get_line_count(coord.line)) {
			y = impl->wrap_info->get_line_count(coord.line) - 1;
			x = INT_MAX;
		}

		coord.pos = impl->wrap_info->calculate_line_pos(coord.line, x, y);
	}
	return coord;
}

void edit_window_t::scroll(int lines) {
	//FIXME: maybe we should use this for pgup/pgdn and up/down as well?
	if (impl->wrap_type == wrap_type_t::NONE) {
		if (lines < 0) {
			if (impl->top_left.line > -lines)
				impl->top_left.line += lines;
			else
				impl->top_left.line = 0;
		} else {
			if (impl->top_left.line + t3_win_get_height(impl->edit_window) <= text->size() - lines)
				impl->top_left.line += lines;
			else if (impl->top_left.line + t3_win_get_height(impl->edit_window) - 1 < text->size())
				impl->top_left.line = text->size() - t3_win_get_height(impl->edit_window);
		}
	} else {
		if (lines < 0)
			impl->wrap_info->sub_lines(impl->top_left, -lines);
		else
			impl->wrap_info->add_lines(impl->top_left, lines);
	}
	update_repaint_lines(0, INT_MAX);
}

void edit_window_t::scrollbar_clicked(scrollbar_t::step_t step) {
	scroll(step == scrollbar_t::BACK_SMALL ? -3 :
		step == scrollbar_t::FWD_SMALL ? 3 :
		step == scrollbar_t::BACK_MEDIUM ? -t3_win_get_height(impl->edit_window) / 2 :
		step == scrollbar_t::FWD_MEDIUM ? t3_win_get_height(impl->edit_window) / 2 :
		step == scrollbar_t::BACK_PAGE ? -(t3_win_get_height(impl->edit_window) - 1) :
		step == scrollbar_t::FWD_PAGE ? (t3_win_get_height(impl->edit_window) - 1) : 0);
}

void edit_window_t::scrollbar_dragged(int start) {
	if (impl->wrap_type == wrap_type_t::NONE) {
		if (start >= 0 && start + t3_win_get_height(impl->edit_window) <= text->size() && start != impl->top_left.line) {
			impl->top_left.line = start;
			update_repaint_lines(0, INT_MAX);
		}
	} else {
		text_coordinate_t new_top_left(0, 0);
		int count = 0;

		if (start < 0 || start + t3_win_get_height(impl->edit_window) > impl->wrap_info->get_size())
			return;

		for (new_top_left.line = 0; new_top_left.line < text->size() && count < start; new_top_left.line++)
			count += impl->wrap_info->get_line_count(new_top_left.line);

		if (count > start) {
			new_top_left.line--;
			count -= impl->wrap_info->get_line_count(new_top_left.line);
			new_top_left.pos = start - count;
		}

		if (new_top_left == impl->top_left || new_top_left.line < 0)
			return;
		impl->top_left = new_top_left;
		update_repaint_lines(0, INT_MAX);
	}
}

void edit_window_t::update_repaint_lines(int start, int end) {
	if (start > end) {
		int tmp = start;
		start = end;
		end = tmp;
	}

	if (start < impl->repaint_min)
		impl->repaint_min = start;
	if (end > impl->repaint_max)
		impl->repaint_max = end;
	redraw = true;
}

//====================== view_parameters_t ========================

edit_window_t::view_parameters_t::view_parameters_t(edit_window_t *view) {
	top_left = view->impl->top_left;
	wrap_type = view->impl->wrap_type;
	if (wrap_type != wrap_type_t::NONE)
		top_left.pos = view->impl->wrap_info->calculate_line_pos(top_left.line, 0, top_left.pos);
	tabsize = view->impl->tabsize;
	tab_spaces = view->impl->tab_spaces;
	ins_mode = view->impl->ins_mode;
	last_set_pos = view->impl->last_set_pos;
	auto_indent = view->impl->auto_indent;
	indent_aware_home = view->impl->indent_aware_home;
	show_tabs = view->impl->show_tabs;
}

edit_window_t::view_parameters_t::view_parameters_t(void) : top_left(0, 0), wrap_type(wrap_type_t::NONE), tabsize(8),
		tab_spaces(false), ins_mode(0), last_set_pos(0), auto_indent(true), indent_aware_home(true), show_tabs(false)
{}

void edit_window_t::view_parameters_t::apply_parameters(edit_window_t *view) const {
	view->impl->top_left = top_left;
	view->impl->tabsize = tabsize;
	view->set_wrap(wrap_type);
	/* view->set_wrap will make sure that view->wrap_info is NULL if
	   wrap_type != NONE. */
	if (view->impl->wrap_info != NULL) {
		view->impl->wrap_info->set_text_buffer(view->text);
		view->impl->top_left.pos = view->impl->wrap_info->find_line(top_left);
	}
	// the calling function will call ensure_cursor_on_screen
	view->impl->tab_spaces = tab_spaces;
	view->impl->ins_mode = ins_mode;
	view->impl->last_set_pos = last_set_pos;
	view->impl->auto_indent = auto_indent;
	view->impl->indent_aware_home = indent_aware_home;
	view->impl->show_tabs = show_tabs;
}


void edit_window_t::view_parameters_t::set_tabsize(int _tabsize) { tabsize = _tabsize; }
void edit_window_t::view_parameters_t::set_wrap(wrap_type_t _wrap_type) { wrap_type = _wrap_type; }
void edit_window_t::view_parameters_t::set_tab_spaces(bool _tab_spaces) { tab_spaces = _tab_spaces; }
void edit_window_t::view_parameters_t::set_auto_indent(bool _auto_indent) { auto_indent = _auto_indent; }
void edit_window_t::view_parameters_t::set_indent_aware_home(bool _indent_aware_home) { indent_aware_home = _indent_aware_home; }
void edit_window_t::view_parameters_t::set_show_tabs(bool _show_tabs) { show_tabs = _show_tabs; }


//====================== autocomplete_panel_t ========================

edit_window_t::autocomplete_panel_t::autocomplete_panel_t(edit_window_t *parent) : popup_t(7, 7), list_pane(NULL) {
	t3_win_set_anchor(window, parent->get_base_window(), 0);

	list_pane = new list_pane_t(false);
	list_pane->set_size(5, 6);
	list_pane->set_position(1, 1);
	list_pane->set_focus(window_component_t::FOCUS_SET);
	list_pane->set_single_click_activate(true);

	push_back(list_pane);
}

bool edit_window_t::autocomplete_panel_t::process_key(key_t key) {
	if (popup_t::process_key(key))
		return true;
	if (key <= 0x20 || key >= EKEY_FIRST_SPECIAL)
		hide();
	return false;
}

void edit_window_t::autocomplete_panel_t::set_position(optint _top, optint _left) {
	int screen_height, screen_width;
	int left, top, absolute_pos;

	get_screen_size(&screen_height, &screen_width);

	top = _top.is_valid() ? (int) _top : t3_win_get_y(window);
	left = _left.is_valid() ? (int) _left : t3_win_get_x(window);

	t3_win_move(window, top, left);

	absolute_pos = t3_win_get_abs_x(window);
	if (absolute_pos + t3_win_get_width(window) > screen_width)
		left -= absolute_pos + t3_win_get_width(window) - screen_width;

	absolute_pos = t3_win_get_abs_y(window);
	if (absolute_pos + t3_win_get_height(window) > screen_height)
		top = top - t3_win_get_height(window) - 2;

	t3_win_move(window, top, left);
	redraw = true;
}

bool edit_window_t::autocomplete_panel_t::set_size(optint height, optint width) {
	bool result;
	result = popup_t::set_size(height, width);
	result &= list_pane->set_size(t3_win_get_height(window) - 2, t3_win_get_width(window) - 1);
	return result;
}

void edit_window_t::autocomplete_panel_t::set_completions(string_list_base_t *completions) {
	int new_width = 1;
	int new_height;

	while (!list_pane->empty()) {
		widget_t *widget = list_pane->back();
		list_pane->pop_back();
		delete widget;
	}

	for (size_t i = 0; i < completions->size(); i++) {
		label_t *label = new label_t((*completions)[i]->c_str());
		list_pane->push_back(label);
		if (label->get_text_width() > new_width)
			new_width = label->get_text_width();
	}

	new_height = list_pane->size() + 2;
	if (new_height > 7)
		new_height = 7;
	else if (new_height < 5)
		new_height = 5;

	set_size(new_height, new_width + 3);
}

size_t edit_window_t::autocomplete_panel_t::get_selected_idx(void) const {
	return list_pane->get_current();
}

void edit_window_t::autocomplete_panel_t::connect_activate(const sigc::slot<void> &slot) {
	list_pane->connect_activate(slot);
}

}; // namespace
