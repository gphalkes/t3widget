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

#include "widgets/label.h"

#include "main.h"
#include "dialogs/gotodialog.h"
#include "dialogs/finddialog.h"
#include "editwindow.h"
#include "util.h"
#include "colorscheme.h"
#include "internal.h"
#include "findcontext.h"
#include "wrapinfo.h"
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

void edit_window_t::init(void) {
	/* Construct these from t3_widget::init, such that the locale is set correctly and
	   gettext therefore returns the correctly localized strings. */
	goto_dialog = new goto_dialog_t();
	global_find_dialog = new find_dialog_t();
	replace_buttons = new replace_buttons_dialog_t();
}

edit_window_t::edit_window_t(text_buffer_t *_text, const view_parameters_t *params) : tab_spaces(false),
		find_dialog(NULL), finder(NULL), wrap_type(wrap_type_t::NONE), wrap_info(NULL), ins_mode(0),
		last_set_pos(0), auto_indent(true), indent_aware_home(true), autocompleter(NULL),
		autocomplete_panel(NULL), autocomplete_panel_shown(false), text(NULL)
{
	/* Register the unbacked window for mouse events, such that we can get focus
	   if the bottom line is clicked. */
	init_unbacked_window(11, 11, true);
	if ((edit_window = t3_win_new(window, 10, 10, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(edit_window);

	if ((indicator_window = t3_win_new(window, 1, 10, 0, 0, 0)) == NULL)
		throw bad_alloc();

	t3_win_set_anchor(indicator_window, window, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	t3_win_show(indicator_window);

	if ((info_window = t3_win_new(window, 1, 1, 0, 0, 1)) == NULL)
		throw bad_alloc();

	t3_win_set_anchor(info_window, window, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	t3_win_show(info_window);

	scrollbar = new scrollbar_t(true);
	set_widget_parent(scrollbar);
	scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	scrollbar->set_size(10, None);

	set_text(_text == NULL ? new text_buffer_t() : _text, params);

	screen_pos = 0;
	focus = false;

	autocomplete_panel = new autocomplete_panel_t(this);
}

edit_window_t::~edit_window_t(void) {
	delete wrap_info;
}

void edit_window_t::set_text(text_buffer_t *_text, const view_parameters_t *params) {
	if (text == _text)
		return;

	text = _text;
	if (params != NULL) {
		params->apply_parameters(this);
	} else {
		if (wrap_info != NULL) {
			wrap_info->set_text_buffer(text);
			wrap_info->set_wrap_width(t3_win_get_width(edit_window) - 1);
		}
		top_left.line = 0;
		top_left.pos = 0;
		last_set_pos = 0;
	}

	ensure_cursor_on_screen();
	draw_info_window();
	redraw = true;
}

bool edit_window_t::set_size(optint height, optint width) {
	bool result = true;
	//FIXME: these int's are optional!!! Take that into account below!

	if (width != t3_win_get_width(window) || height > t3_win_get_height(window))
		redraw = true;

	result &= t3_win_resize(window, height, width);
	result &= t3_win_resize(edit_window, height - 1, width - 1);
	result &= scrollbar->set_size(height - 1, None);

	if (wrap_type != wrap_type_t::NONE) {
		top_left.pos = wrap_info->calculate_line_pos(top_left.line, 0, top_left.pos);
		wrap_info->set_wrap_width(width - 1);
		top_left.pos = wrap_info->find_line(top_left);
		last_set_pos = wrap_info->calculate_screen_pos();
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


	if (wrap_type == wrap_type_t::NONE) {
		screen_pos = text->calculate_screen_pos(NULL, tabsize);

		if (text->cursor.line < top_left.line) {
			top_left.line = text->cursor.line;
			redraw = true;
		}

		if (text->cursor.line >= top_left.line + t3_win_get_height(edit_window)) {
			top_left.line = text->cursor.line - t3_win_get_height(edit_window) + 1;
			redraw = true;
		}

		if (screen_pos < top_left.pos) {
			top_left.pos = screen_pos;
			redraw = true;
		}

		if (screen_pos + width > top_left.pos + t3_win_get_width(edit_window)) {
			top_left.pos = screen_pos + width - t3_win_get_width(edit_window);
			redraw = true;
		}
	} else {
		text_coordinate_t bottom;
		int sub_line = wrap_info->find_line(text->cursor);
		screen_pos = wrap_info->calculate_screen_pos();

		if (text->cursor.line < top_left.line || (text->cursor.line == top_left.line && sub_line < top_left.pos)) {
			top_left.line = text->cursor.line;
			top_left.pos = sub_line;
			redraw = true;
		} else {
			bottom = top_left;
			wrap_info->add_lines(bottom, t3_win_get_height(edit_window) - 1);

			while (text->cursor.line > bottom.line) {
				wrap_info->add_lines(top_left, wrap_info->get_line_count(bottom.line) - bottom.pos);
				bottom.line++;
				bottom.pos = 0;
				redraw = true;
			}

			if (text->cursor.line == bottom.line && sub_line > bottom.pos) {
				wrap_info->add_lines(top_left, sub_line - bottom.pos);
				redraw = true;
			}
		}
	}
}

void edit_window_t::repaint_screen(void) {
	text_coordinate_t current_start, current_end;
	text_line_t::paint_info_t info;
	int i;

	t3_win_set_default_attrs(edit_window, attributes.text);

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();

	if (current_end < current_start) {
		current_start = current_end;
		current_end = text->get_selection_start();
	}

	info.size = t3_win_get_width(edit_window);
	info.tabsize = tabsize;
	info.normal_attr = 0;
	info.selected_attr = attributes.text_selected;

	if (wrap_type == wrap_type_t::NONE) {
		info.leftcol = top_left.pos;
		for (i = 0; i < t3_win_get_height(edit_window) && (i + top_left.line) < text->size(); i++) {
			info.selection_start = top_left.line + i == current_start.line ? current_start.pos : -1;
			if (top_left.line + i >= current_start.line) {
				if (top_left.line + i < current_end.line)
					info.selection_end = INT_MAX;
				else if (top_left.line + i == current_end.line)
					info.selection_end = current_end.pos;
				else
					info.selection_end = -1;
			} else {
				info.selection_end = -1;
			}

			info.cursor = focus && top_left.line + i == text->cursor.line ? text->cursor.pos : -1;
			t3_win_set_paint(edit_window, i, 0);
			t3_win_clrtoeol(edit_window);
			text->paint_line(edit_window, top_left.line + i, &info);
		}
	} else {
		text_coordinate_t end_coord = wrap_info->get_end();
		text_coordinate_t draw_line = top_left;
		info.leftcol = 0;
		for (i = 0; i < t3_win_get_height(edit_window); i++, wrap_info->add_lines(draw_line, 1)) {
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

			info.cursor = focus && draw_line.line == text->cursor.line ? text->cursor.pos : -1;
			t3_win_set_paint(edit_window, i, 0);
			t3_win_clrtoeol(edit_window);
			wrap_info->paint_line(edit_window, draw_line, &info);

			if (draw_line.line == end_coord.line && draw_line.pos == end_coord.pos)
				break;
		}
	}
	t3_win_clrtobot(edit_window);
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
	last_set_pos = screen_pos;
}

void edit_window_t::next_word(void) {
	text->goto_next_word();
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
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
	last_set_pos = screen_pos;
}

void edit_window_t::previous_word(void) {
	text->goto_previous_word();
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
}

void edit_window_t::inc_y(void) {
	if (wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line + 1 < text->size()) {
			text->cursor.line++;
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, last_set_pos, tabsize);
			ensure_cursor_on_screen();
		} else {
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			last_set_pos = screen_pos;
		}
	} else {
		int new_sub_line = wrap_info->find_line(text->cursor) + 1;
		if (wrap_info->get_line_count(text->cursor.line) == new_sub_line) {
			if (text->cursor.line + 1 < text->size()) {
				text->cursor.line++;
				text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos, 0);
				ensure_cursor_on_screen();
			} else {
				text->cursor.pos = text->get_line_max(text->cursor.line);
				ensure_cursor_on_screen();
				last_set_pos = screen_pos;
			}
		} else {
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos, new_sub_line);
			ensure_cursor_on_screen();
		}
	}
}

void edit_window_t::dec_y(void) {
	if (wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line > 0) {
			text->cursor.line--;
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, last_set_pos, tabsize);
			ensure_cursor_on_screen();
		} else {
			last_set_pos = text->cursor.pos = 0;
			ensure_cursor_on_screen();
		}
	} else {
		int sub_line = wrap_info->find_line(text->cursor);
		if (sub_line > 0) {
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos, sub_line - 1);
			ensure_cursor_on_screen();
		} else if (text->cursor.line > 0) {
			text->cursor.line--;
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos,
				wrap_info->get_line_count(text->cursor.line) - 1);
			ensure_cursor_on_screen();
		} else {
			text->cursor.pos = 0;
			ensure_cursor_on_screen();
			last_set_pos = screen_pos;
		}
	}
}

void edit_window_t::pgdn(void) {
	bool need_adjust = true;

	if (wrap_type == wrap_type_t::NONE) {
		if (text->cursor.line + t3_win_get_height(edit_window) - 1 < text->size()) {
			text->cursor.line += t3_win_get_height(edit_window) - 1;
		} else {
			text->cursor.line = text->size() - 1;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			need_adjust = false;
		}

		/* If the end of the text is already on the screen, don't change the top line. */
		if (top_left.line + t3_win_get_height(edit_window) < text->size()) {
			top_left.line += t3_win_get_height(edit_window) - 1;
			if (top_left.line + t3_win_get_height(edit_window) > text->size())
				top_left.line = text->size() - t3_win_get_height(edit_window);
			redraw = true;
		}

		if (need_adjust)
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, last_set_pos, tabsize);

	} else {
		text_coordinate_t new_top_left = top_left;
		text_coordinate_t new_cursor(text->cursor.line, wrap_info->find_line(text->cursor));

		if (wrap_info->add_lines(new_cursor, t3_win_get_height(edit_window) - 1)) {
			text->cursor.line = new_cursor.line;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			need_adjust = false;
		} else {
			text->cursor.line = new_cursor.line;
		}

		/* If the end of the text is already on the screen, don't change the top line. */
		if (!wrap_info->add_lines(new_top_left, t3_win_get_height(edit_window))) {
			top_left = new_top_left;
			wrap_info->sub_lines(top_left, 1);
		}

		if (need_adjust)
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos, new_cursor.pos);
	}
	ensure_cursor_on_screen();

	if (!need_adjust)
		last_set_pos = screen_pos;
}

void edit_window_t::pgup(void) {
	bool need_adjust = true;

	if (wrap_type == wrap_type_t::NONE) {
		if (top_left.line < t3_win_get_height(edit_window) - 1) {
			if (top_left.line != 0) {
				redraw = true;
				top_left.line = 0;
			}

			if (text->cursor.line < t3_win_get_height(edit_window) - 1) {
				text->cursor.line = 0;
				last_set_pos = text->cursor.pos = 0;
				need_adjust = false;
			} else {
				text->cursor.line -= t3_win_get_height(edit_window) - 1;
			}
		} else {
			text->cursor.line -= t3_win_get_height(edit_window) - 1;
			top_left.line -= t3_win_get_height(edit_window) - 1;
			redraw = true;
		}

		if (need_adjust)
			text->cursor.pos = text->calculate_line_pos(text->cursor.line, last_set_pos, tabsize);
	} else {
		text_coordinate_t new_cursor(text->cursor.line, wrap_info->find_line(text->cursor));

		if (wrap_info->sub_lines(new_cursor, t3_win_get_height(edit_window) - 1)) {
			text->cursor.line = 0;
			text->cursor.pos = 0;
			last_set_pos = 0;
			need_adjust = false;
		} else {
			text->cursor.line = new_cursor.line;
		}

		wrap_info->sub_lines(top_left, t3_win_get_height(edit_window) - 1);

		if (need_adjust)
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, last_set_pos, new_cursor.pos);
	}
	ensure_cursor_on_screen();
}

void edit_window_t::home(void) {
	const text_line_t *line;
	int pos;

	if (!indent_aware_home) {
		text->cursor.pos = wrap_type == wrap_type_t::NONE ? 0 :
			wrap_info->calculate_line_pos(text->cursor.line, 0, wrap_info->find_line(text->cursor));
		ensure_cursor_on_screen();
		last_set_pos = screen_pos;
		return;
	}

	if (wrap_type != wrap_type_t::NONE) {
		pos = wrap_info->calculate_line_pos(text->cursor.line, 0, wrap_info->find_line(text->cursor));
		if (pos != text->cursor.pos) {
			text->cursor.pos = pos;
			screen_pos = last_set_pos = 0;
			return;
		}
	}
	line = text->get_line_data(text->cursor.line);
	for (pos = 0; pos < line->get_length() && line->is_space(pos); pos = line->adjust_position(pos, 1)) {}

	text->cursor.pos = text->cursor.pos != pos ? pos : 0;
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
}

void edit_window_t::end(void) {
	if (wrap_type == wrap_type_t::NONE) {
		text->cursor.pos = text->get_line_max(text->cursor.line);
	} else {
		int sub_line = wrap_info->find_line(text->cursor);
		if (sub_line + 1 < wrap_info->get_line_count(text->cursor.line)) {
			int before_pos = text->cursor.pos;
			text->cursor.pos = wrap_info->calculate_line_pos(text->cursor.line, 0, sub_line + 1);
			text->adjust_position(-1);
			if (before_pos == text->cursor.pos)
				text->cursor.pos = text->get_line_max(text->cursor.line);
		} else {
			text->cursor.pos = text->get_line_max(text->cursor.line);
		}
	}
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
}

void edit_window_t::reset_selection(void) {
	text->set_selection_mode(selection_mode_t::NONE);
	redraw = true;
}

void edit_window_t::set_selection_mode(key_t key) {
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
				reset_selection();
			} else if ((key & EKEY_SHIFT) && selection_mode != selection_mode_t::MARK) {
				text->set_selection_mode(selection_mode_t::SHIFT);
			}
			break;
		default:
			break;
	}
}

void edit_window_t::delete_selection(void) {
	text_coordinate_t current_start, current_end;

	current_start = text->get_selection_start();
	current_end = text->get_selection_end();
	text->delete_block(current_start, current_end);

	redraw = true;
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
	reset_selection();
}

void edit_window_t::find_activated(find_action_t action, finder_t *_finder) {
	finder_t *local_finder;
	find_result_t result;

	local_finder = finder == NULL ? &global_finder : finder;
	if (_finder != NULL)
		*local_finder = *_finder;

	switch (action) {
		case find_action_t::FIND:
			if (!text->find(local_finder, &result))
				goto not_found;

			text->set_selection_from_find(&result);
			redraw = true;
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
			result.line = text->get_selection_start().line;
			result.start = text->get_selection_start().pos;
			result.end = text->get_selection_end().pos;
			text->replace(local_finder, &result);
			redraw = true;
			/* FALLTHROUGH */
		case find_action_t::SKIP:
			if (!text->find(local_finder, &result)) {
				ensure_cursor_on_screen();
				goto not_found;
			}

			text->set_selection_from_find(&result);
			redraw = true;
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
			redraw = true;
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
			redraw = true;
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
	if (autocomplete_panel_shown) {
		if (key == EKEY_ESC) {
			hide_autocomplete();
		} else if (key == EKEY_NL) {
			size_t idx = autocomplete_panel->get_selected_idx();
			hide_autocomplete();
			autocompleter->autocomplete(text, idx);
			return true;
		} else if (autocomplete_panel->process_key(key)) {
			return true;
		} else if ((key <= 0x20 || key >= 0x110000) && key != EKEY_BS) {
			hide_autocomplete();
		}
	}

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
			home();
			break;
		case EKEY_HOME | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_HOME | EKEY_CTRL:
			screen_pos = last_set_pos = text->cursor.pos = 0;
			text->cursor.line = 0;
			if ((wrap_type == wrap_type_t::NONE && top_left.pos != 0) || top_left.line != 0)
				ensure_cursor_on_screen();
			break;
		case EKEY_END | EKEY_SHIFT:
		case EKEY_END:
			end();
			break;
		case EKEY_END | EKEY_CTRL | EKEY_SHIFT:
		case EKEY_END | EKEY_CTRL: {
			text->cursor.line = text->size() - 1;
			text->cursor.pos = text->get_line_max(text->cursor.line);
			ensure_cursor_on_screen();
			last_set_pos = screen_pos;
			break;
		}
		case EKEY_INS:
			ins_mode ^= 1;
			break;

		case EKEY_DEL:
			if (text->get_selection_mode() == selection_mode_t::NONE) {
				if (text->cursor.pos != text->get_line_max(text->cursor.line)) {
					text->delete_char();

					if (wrap_type != wrap_type_t::NONE)
						ensure_cursor_on_screen();

					redraw = true;
				} else if (text->cursor.line + 1 < text->size()) {
					text->merge(false);

					if (wrap_type != wrap_type_t::NONE)
						ensure_cursor_on_screen();

					redraw = true;
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

			if (auto_indent) {
				current_line = text->get_line_data(text->cursor.line)->get_data();
				for (i = 0, indent = 0, tabs = 0; i < text->cursor.pos; i++) {
					if ((*current_line)[i] == '\t') {
						indent = 0;
						tabs++;
					} else if ((*current_line)[i] == ' ') {
						indent++;
						if (indent == tabsize) {
							indent = 0;
							tabs++;
						}
					} else {
						break;
					}
				}
				if (tab_spaces)
					space.append(tabsize * tabs, ' ');
				else
					space.append(tabs, '\t');

				if (indent != 0)
					space.append(indent, ' ');

				text->break_line(&space);
			} else {
				text->break_line();
			}
			ensure_cursor_on_screen();
			last_set_pos = screen_pos;
			redraw = true;
			break;
		}
		case EKEY_BS:
			if (text->get_selection_mode() == selection_mode_t::NONE) {
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
				last_set_pos = screen_pos;
			} else {
				delete_selection();
			}
			if (autocomplete_panel_shown)
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
				text->unindent_line(tabsize);
				ensure_cursor_on_screen();
				last_set_pos = screen_pos;
				redraw = true;
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

				if (tab_spaces)
					space.append(tabsize - (screen_pos % tabsize), ' ');
				else
					space.append(1, '\t');
				text->insert_block(&space);
				ensure_cursor_on_screen();
			}
			break;
		default: {
			int local_insmode;

			if (key < 32)
				return false;

			key &= ~EKEY_PROTECT;
			if (key == 10)
				return false;

			if (key >= 0x110000)
				return false;

			local_insmode = ins_mode;
			if (text->get_selection_mode() != selection_mode_t::NONE) {
				delete_selection();
				local_insmode = 0;
			}

			(text->*proces_char[local_insmode])(key);
			ensure_cursor_on_screen();
			redraw = true;
			last_set_pos = screen_pos;
			if (autocomplete_panel_shown)
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

	   - cursor-only movements mostly don't require entire redraws

	*/

	if (autocomplete_panel_shown)
		autocomplete_panel->update_contents();

	if (!focus && !redraw)
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

	t3_win_set_default_attrs(indicator_window, attributes.menubar);
	t3_win_set_paint(indicator_window, 0, 0);
	t3_win_addchrep(indicator_window, ' ', 0, t3_win_get_width(indicator_window));

	if (wrap_type == wrap_type_t::NONE) {
		scrollbar->set_parameters(max(text->size(), top_left.line + t3_win_get_height(edit_window)),
			top_left.line, t3_win_get_height(edit_window));
	} else {
		int i, count = 0;
		for (i = 0; i < top_left.line; i++)
			count += wrap_info->get_line_count(i);
		count += top_left.pos;

		scrollbar->set_parameters(max(wrap_info->get_size(), count + t3_win_get_height(edit_window)),
			count, t3_win_get_height(edit_window));
	}
	scrollbar->update_contents();

	logical_cursor_pos = text->cursor;
	logical_cursor_pos.pos = text->calculate_screen_pos(NULL, tabsize);

	snprintf(info, 29, "L: %-4d C: %-4d %c %s", logical_cursor_pos.line + 1, logical_cursor_pos.pos + 1,
		text->is_modified() ? '*' : ' ', ins_string[ins_mode]);
	info_width = t3_term_strwidth(info);
	t3_win_resize(indicator_window, 1, info_width + 3);
	name_width = t3_win_get_width(window) - t3_win_get_width(indicator_window);

	if (t3_win_get_width(info_window) != name_width && name_width > 0) {
		t3_win_resize(info_window, 1, name_width);
		draw_info_window();
	}

	t3_win_set_paint(indicator_window, 0, t3_win_get_width(indicator_window) - info_width - 1);
	t3_win_addstr(indicator_window, info, 0);
}

void edit_window_t::set_focus(bool _focus) {
	focus = _focus;
	hide_autocomplete();
	redraw = true; //FXIME: Only for painting/removing cursor
}

void edit_window_t::undo(void) {
	if (text->apply_undo() == 0) {
		redraw = true;
		ensure_cursor_on_screen();
		last_set_pos = screen_pos;
	}
}

void edit_window_t::redo(void) {
	if (text->apply_redo() == 0) {
		redraw = true;
		ensure_cursor_on_screen();
		last_set_pos = screen_pos;
	}
}

void edit_window_t::cut_copy(bool cut) {
	if (text->get_selection_mode() != selection_mode_t::NONE) {
		if (text->selection_empty()) {
			reset_selection();
			return;
		}

		if (copy_buffer != NULL)
			delete copy_buffer;

		copy_buffer = text->convert_selection();

		if (cut)
			delete_selection();
		else if (text->get_selection_mode() == selection_mode_t::MARK)
			text->set_selection_mode(selection_mode_t::SHIFT);
	}
}

void edit_window_t::paste(void) {
	if (copy_buffer != NULL) {
		if (text->get_selection_mode() == selection_mode_t::NONE) {
			text->insert_block(copy_buffer);
		} else {
			text->replace_block(text->get_selection_start(), text->get_selection_end(), copy_buffer);
			reset_selection();
		}
		ensure_cursor_on_screen();
		last_set_pos = screen_pos;
		redraw = true;
	}
}

void edit_window_t::select_all(void) {
	text->set_selection_mode(selection_mode_t::ALL);
	redraw = true;
}

void edit_window_t::insert_special(void) {
	insert_char_dialog->center_over(center_window);
	insert_char_dialog->reset();
	insert_char_dialog->show();
}

void edit_window_t::indent_selection(void) {
	text->indent_selection(tabsize, tab_spaces);
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
	redraw = true;
}

void edit_window_t::unindent_selection(void) {
	text->unindent_selection(tabsize);
	ensure_cursor_on_screen();
	last_set_pos = screen_pos;
	redraw = true;
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
	last_set_pos = screen_pos;
}

void edit_window_t::find_replace(bool replace) {
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
	dialog->set_replace(replace);
	//FIXME: set selected text in dialog
	//dialog->set_text(text->get_selected_text());
	dialog->show();
}

void edit_window_t::find_next(bool backward) {
	find_result_t result;
	if (!text->find(finder != NULL ? finder : &global_finder, &result, backward)) {
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
	find_dialog = _find_dialog;
}

void edit_window_t::set_finder(finder_t *_finder) {
	finder = _finder;
}

void edit_window_t::force_redraw(void) {
	widget_t::force_redraw();
	draw_info_window();
	ensure_cursor_on_screen();
}

void edit_window_t::set_tabsize(int _tabsize) {
	if (_tabsize == tabsize)
		return;
	tabsize = _tabsize;
	if (wrap_info != NULL)
		wrap_info->set_tabsize(tabsize);
	force_redraw();
}

void edit_window_t::bad_draw_recheck(void) {
	widget_t::force_redraw();
}

void edit_window_t::focus_set(widget_t *target) {
	(void) target;
	set_focus(true);
}

bool edit_window_t::is_child(widget_t *widget) {
	return widget == scrollbar;
}

bool edit_window_t::process_mouse_event(mouse_event_t event) {
	if (event.window == edit_window) {
		if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT) && event.previous_button_state == 0) {
			reset_selection();
			text->cursor = xy_to_text_coordinate(event.x, event.y);
			ensure_cursor_on_screen();
		} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
			if (wrap_type == wrap_type_t::NONE) {
				if (event.button_state & EMOUSE_SCROLL_UP) {
					if (top_left.line > 3)
						top_left.line -= 3;
					else
						top_left.line = 0;
				} else {
					if (top_left.line + t3_win_get_height(edit_window) <= text->size() - 3)
						top_left.line += 3;
					else if (top_left.line + t3_win_get_height(edit_window) - 1 < text->size())
						top_left.line = text->size() - t3_win_get_height(edit_window);
				}
			}

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
				text->set_selection_end();
			ensure_cursor_on_screen();
		}
	}
	return true;
}

void edit_window_t::set_wrap(wrap_type_t wrap) {
	if (wrap == wrap_type)
		return;

	if (wrap == wrap_type_t::NONE) {
		top_left.pos = 0;
		if (wrap_info != NULL) {
			delete wrap_info;
			wrap_info = NULL;
		}
	} else {
		//FIXME: differentiate between wrap types
		if (wrap_info == NULL)
			wrap_info = new wrap_info_t(t3_win_get_width(edit_window) - 1, tabsize);
		wrap_info->set_text_buffer(text);
		wrap_info->set_wrap_width(t3_win_get_width(edit_window) - 1);
		top_left.pos = wrap_info->find_line(top_left);
	}
	wrap_type = wrap;
	ensure_cursor_on_screen();
}

void edit_window_t::set_tab_spaces(bool _tab_spaces) {
	tab_spaces = _tab_spaces;
}

void edit_window_t::set_auto_indent(bool _auto_indent) {
	auto_indent = _auto_indent;
}

void edit_window_t::set_indent_aware_home(bool _indent_aware_home) {
	indent_aware_home = _indent_aware_home;
}

int edit_window_t::get_tabsize(void) {
	return tabsize;
}

wrap_type_t edit_window_t::get_wrap(void) {
	return wrap_type;
}

bool edit_window_t::get_tab_spaces(void) {
	return tab_spaces;
}

bool edit_window_t::get_auto_indent(void) {
	return auto_indent;
}

bool edit_window_t::get_indent_aware_home(void) {
	return indent_aware_home;
}

edit_window_t::view_parameters_t *edit_window_t::save_view_parameters(void) {
	return new view_parameters_t(this);
}

void edit_window_t::save_view_parameters(view_parameters_t *params) {
	*params = view_parameters_t(this);
}

void edit_window_t::draw_info_window(void) {}

void edit_window_t::set_autocompleter(autocompleter_t *_autocompleter) {
	hide_autocomplete();
	autocompleter = _autocompleter;
}

void edit_window_t::hide_autocomplete(void) {
	if (autocomplete_panel_shown) {
		autocomplete_panel->hide();
		autocomplete_panel_shown = false;
	}
}

void edit_window_t::autocomplete(void) {
	activate_autocomplete(true);
}

void edit_window_t::activate_autocomplete(bool autocomplete_single) {
	if (autocompleter == NULL)
		return;

	text_coordinate_t anchor(text->cursor);
	string_list_base_t *autocomplete_list = autocompleter->build_autocomplete_list(text, &anchor.pos);

	if (autocomplete_list != NULL) {
		if (autocomplete_single && autocomplete_list->size() == 1) {
			autocompleter->autocomplete(text, 0);
			/* Just in case... */
			hide_autocomplete();
			return;
		}

		autocomplete_panel->set_completions(autocomplete_list);
		if (wrap_type == wrap_type_t::NONE) {
			int position = text->calculate_screen_pos(&anchor, tabsize);
			autocomplete_panel->set_position(text->cursor.line - top_left.line + 1,
				position - top_left.pos - 1);
		} else {
			int sub_line = wrap_info->find_line(text->cursor);
			int position = wrap_info->calculate_screen_pos(&anchor);
			int line;

			if (text->cursor.line == top_left.line) {
				line = sub_line - top_left.pos;
			} else {
				line = wrap_info->get_line_count(top_left.line) - top_left.pos + sub_line;
				for (int i = top_left.line + 1; i < text->cursor.line; i++)
					line += wrap_info->get_line_count(i);
			}
			autocomplete_panel->set_position(line + 1, position - 1);
		}
		autocomplete_panel->show();
		autocomplete_panel_shown = true;
	} else if (autocomplete_panel_shown) {
		hide_autocomplete();
	}
}

text_coordinate_t edit_window_t::xy_to_text_coordinate(int x, int y) {
	text_coordinate_t coord;
	if (wrap_type == wrap_type_t::NONE) {
		coord.line = y + top_left.line;
		x += top_left.pos;
		if (coord.line >= text->size()) {
			coord.line = text->size() - 1;
			x = INT_MAX;
		}
		coord.pos = text->calculate_line_pos(coord.line, x, tabsize);
	} else {
		coord.line = top_left.line;
		y += top_left.pos;
		while (coord.line < wrap_info->get_size() - 1 && y >= wrap_info->get_line_count(coord.line)) {
			y -= wrap_info->get_line_count(coord.line);
			coord.line++;
		}
		if (y >= wrap_info->get_line_count(coord.line)) {
			y = wrap_info->get_line_count(coord.line) - 1;
			x = INT_MAX;
		}

		coord.pos = wrap_info->calculate_line_pos(coord.line, x, y);
	}
	return coord;
}

//====================== view_parameters_t ========================

edit_window_t::view_parameters_t::view_parameters_t(edit_window_t *view) {
	top_left = view->top_left;
	wrap_type = view->wrap_type;
	if (wrap_type != wrap_type_t::NONE)
		top_left.pos = view->wrap_info->calculate_line_pos(top_left.line, 0, top_left.pos);
	tabsize = view->tabsize;
	tab_spaces = view->tab_spaces;
	ins_mode = view->ins_mode;
	last_set_pos = view->last_set_pos;
	auto_indent = view->auto_indent;
	indent_aware_home = view->indent_aware_home;
}

edit_window_t::view_parameters_t::view_parameters_t(void) : top_left(0, 0), wrap_type(wrap_type_t::NONE), tabsize(8),
		tab_spaces(false), ins_mode(0), last_set_pos(0), auto_indent(true), indent_aware_home(true)
{}

void edit_window_t::view_parameters_t::apply_parameters(edit_window_t *view) const {
	view->top_left = top_left;
	view->tabsize = tabsize;
	view->set_wrap(wrap_type);
	/* view->set_wrap will make sure that view->wrap_info is NULL if
	   wrap_type != NONE. */
	if (view->wrap_info != NULL) {
		view->wrap_info->set_text_buffer(view->text);
		view->top_left.pos = view->wrap_info->find_line(top_left);
	}
	// the calling function will call ensure_cursor_on_screen
	view->tab_spaces = tab_spaces;
	view->ins_mode = ins_mode;
	view->last_set_pos = last_set_pos;
	view->auto_indent = auto_indent;
	view->indent_aware_home = indent_aware_home;
}


void edit_window_t::view_parameters_t::set_tabsize(int _tabsize) { tabsize = _tabsize; }
void edit_window_t::view_parameters_t::set_wrap(wrap_type_t _wrap_type) { wrap_type = _wrap_type; }
void edit_window_t::view_parameters_t::set_tab_spaces(bool _tab_spaces) { tab_spaces = _tab_spaces; }
void edit_window_t::view_parameters_t::set_auto_indent(bool _auto_indent) { auto_indent = _auto_indent; }
void edit_window_t::view_parameters_t::set_indent_aware_home(bool _indent_aware_home) { indent_aware_home = _indent_aware_home; }


//====================== autocomplete_panel_t ========================

edit_window_t::autocomplete_panel_t::autocomplete_panel_t(edit_window_t *parent) : list_pane(false) {
	if ((window = t3_win_new(parent->get_base_window(), 7, 7, 0, 0, -10)) == NULL)
		throw bad_alloc();
	if ((shadow_window = t3_win_new(parent->get_base_window(), 7, 7, 1, 1, -9)) == NULL)
		throw bad_alloc();
	t3_win_set_anchor(shadow_window, window, 0);

	list_pane.set_size(5, 6);
	list_pane.set_position(1, 1);
	list_pane.set_focus(true);
	set_widget_parent(&list_pane);
}

bool edit_window_t::autocomplete_panel_t::process_key(key_t key) {
	return list_pane.process_key(key);
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
	(void) height;
	result = t3_win_resize(window, height, width);
	result &= t3_win_resize(shadow_window, height, width);
	result &= list_pane.set_size(height - 2, width - 1);
	redraw = true;
	return result;
}

void edit_window_t::autocomplete_panel_t::update_contents(void) {
	if (redraw) {
		t3_win_set_default_attrs(window, attributes.dialog);
		t3_win_set_default_attrs(shadow_window, attributes.shadow);

		t3_win_set_paint(window, 0, 0);
		t3_win_clrtobot(window);
		t3_win_box(window, 0, 0, t3_win_get_height(window), t3_win_get_width(window), 0);

		int x = t3_win_get_width(shadow_window) - 1;
		for (int i = t3_win_get_height(shadow_window) - 1; i > 0; i--) {
			t3_win_set_paint(shadow_window, i - 1, x);
			t3_win_addch(shadow_window, ' ', 0);
		}
		t3_win_set_paint(shadow_window, t3_win_get_height(shadow_window) - 1, 0);
		t3_win_addchrep(shadow_window, ' ', 0, t3_win_get_width(shadow_window));

		redraw = false;
	}
	list_pane.update_contents();
}

void edit_window_t::autocomplete_panel_t::set_focus(bool _focus) {
	(void) _focus;
	//ignore?
}

void edit_window_t::autocomplete_panel_t::show(void) {
	t3_win_show(window);
	t3_win_show(shadow_window);
	list_pane.set_focus(true);
}

void edit_window_t::autocomplete_panel_t::hide(void) {
	t3_win_hide(window);
	t3_win_hide(shadow_window);
}

void edit_window_t::autocomplete_panel_t::force_redraw(void) {
	list_pane.force_redraw();
	redraw = true;
}

void edit_window_t::autocomplete_panel_t::focus_set(widget_t *target) {
	(void) target;
}

bool edit_window_t::autocomplete_panel_t::is_child(widget_t *widget) {
	return widget == &list_pane || list_pane.is_child(widget);
}

void edit_window_t::autocomplete_panel_t::set_completions(string_list_base_t *completions) {
	int new_width = 1;
	int new_height;

	while (!list_pane.empty()) {
		widget_t *widget = list_pane.back();
		list_pane.pop_back();
		delete widget;
	}

	for (size_t i = 0; i < completions->size(); i++) {
		label_t *label = new label_t((*completions)[i]->c_str());
		list_pane.push_back(label);
		if (label->get_text_width() > new_width)
			new_width = label->get_text_width();
	}

	new_height = list_pane.size() + 2;
	if (new_height > 7)
		new_height = 7;
	else if (new_height < 5)
		new_height = 5;

	set_size(new_height, new_width + 3);
}

size_t edit_window_t::autocomplete_panel_t::get_selected_idx(void) const {
	return list_pane.get_current();
}

}; // namespace
