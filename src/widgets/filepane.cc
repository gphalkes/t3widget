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
#include "colorscheme.h"
#include "widgets/filepane.h"
#include "log.h"

using namespace std;
//FIXME: we could use some optimization for update_column_widths. Current use is simple but calls to often.
file_pane_t::file_pane_t(container_t *parent) : widget_t(parent, 3, 3), scrollbar(this, false), height(1),
	width(1),top_idx(0), current(0), file_list(NULL), focus(false), redraw(true), field(NULL), columns_visible(0)
{
	t3_win_set_default_attrs(window, colors.dialog_attrs);
	scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	scrollbar.set_position(0, 1);
}

void file_pane_t::set_text_field(text_field_t *_field) {
	field = _field;
}

void file_pane_t::ensure_cursor_on_screen(void) {
	size_t old_top_idx = top_idx;

	if (file_list == NULL)
		return;

	while (current >= top_idx + columns_visible * height)
		top_idx += height;
	while (current < top_idx && top_idx > (size_t) height)
		top_idx -= height;
	if (top_idx > current)
		top_idx = 0;
	if (top_idx != old_top_idx) {
		update_column_widths();
		ensure_cursor_on_screen();
		scrollbar.set_parameters(file_list->get_length(), top_idx, columns_visible * height);
	}
}

bool file_pane_t::process_key(key_t key) {
	switch (key) {
		case EKEY_DOWN:
			if (current + 1 >= file_list->get_length())
				return true;
			current++;
			break;
		case EKEY_UP:
			if (current == 0)
				return true;
			current--;
			break;
		case EKEY_RIGHT:
			if (current + height >= file_list->get_length())
				current = file_list->get_length() - 1;
			else
				current += height;
			break;
		case EKEY_LEFT:
			if (current < (size_t) height)
				current = 0;
			else
				current -= height;
			break;
		case EKEY_END:
			current = file_list->get_length() - 1;
			break;
		case EKEY_HOME:
			current = 0;
			break;
		case EKEY_PGDN:
			if (current + 2 * height >= file_list->get_length()) {
				current = file_list->get_length() - 1;
			} else {
				current += 2 * height;
				top_idx += 2 * height;
			}
			break;
		case EKEY_PGUP:
			if (current < (size_t) 2 * height) {
				current = 0;
			} else {
				current -= 2 * height;
				top_idx -= 2 * height;
			}
			break;
		case EKEY_NL:
			activate(file_list->get_name(current));
			return true;
		default:
			return false;
	}
	if (field)
		field->set_text(file_list->get_name(current)->c_str());
	ensure_cursor_on_screen();
	return true;
}

bool file_pane_t::set_size(optint _height, optint _width) {
	bool result;
	if (_height.is_valid())
		height = _height - 2;
	if (_width.is_valid())
		width = _width - 2;
	result = t3_win_resize(window, height + 2, width + 2);
	result &= scrollbar.set_size(None, width);
	update_column_widths();
	ensure_cursor_on_screen();
	if (file_list != NULL)
		scrollbar.set_parameters(file_list->get_length(), top_idx, columns_visible * height);
	redraw = true;
	return result;
}

void file_pane_t::draw_line(int idx, bool selected) {
	if ((size_t) idx < top_idx || (size_t) idx > file_list->get_length())
		return;

	int column;
	line_t line(file_list->get_name(idx));
	bool is_dir = file_list->is_dir(idx);
	line_t::paint_info_t info;

	idx -= top_idx;
	column = idx / height;
	idx %= height;

	t3_win_set_paint(window, idx + 1, column_positions[column] + 1);
	t3_win_addch(window, is_dir ? '/' : ' ', selected ? colors.dialog_selected_attrs : 0);

	info.start = 0;
	info.leftcol = 0;
	info.max = INT_MAX;
	info.size = column_widths[column];
	info.tabsize = 0;
	info.flags = line_t::SPACECLEAR | line_t::TAB_AS_CONTROL | (selected ? line_t::EXTEND_SELECTION : 0);
	info.selection_start = -1;
	info.selection_end = selected ? INT_MAX : -1;
	info.cursor = -1;
	info.normal_attr = colors.dialog_attrs;
	info.selected_attr = colors.dialog_selected_attrs;

	line.paint_line(window, &info);
}

void file_pane_t::update_contents(void) {
	size_t max_idx, i;

	if (!redraw || file_list == NULL)
		return;

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	for (i = top_idx, max_idx = min(top_idx + columns_visible * height, file_list->get_length()); i < max_idx; i++)
		draw_line(i, focus && i == current);

	t3_win_box(window, 0, 0, height + 2, width + 2, 0);
	scrollbar.update_contents();
}

void file_pane_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		t3_term_hide_cursor();
		if (file_list != NULL)
			draw_line(current, true);
	} else {
		if (file_list != NULL)
			draw_line(current, false);
	}
}

bool file_pane_t::accepts_enter(void) { return true; }

t3_window_t *file_pane_t::get_draw_window(void) { return window; }

void file_pane_t::reset(void) {
	top_idx = 0;
	current = 0;
}

void file_pane_t::set_file_list(file_list_t *_file_list) {
	if (file_list != NULL)
		content_changed_connection.disconnect();

	file_list = _file_list;
	content_changed_connection = file_list->connect_content_changed(sigc::mem_fun(this, &file_pane_t::content_changed));
	top_idx = 0;
	content_changed();
	update_contents();
}

void file_pane_t::set_file(size_t idx) {
	current = idx < file_list->get_length() ? idx : 0;
	ensure_cursor_on_screen();
	/* if (field)
		field->set_text(idx == 0 ? "" : file_list->get_name(current)->c_str()); */
}

void file_pane_t::update_column_width(int column, int start) {
	column_widths[column] = 0;
	for (int i = 0; i < height && start + i < (int) file_list->get_length(); i++)
		column_widths[column] = max(column_widths[column], t3_term_strwidth(file_list->get_name(i + start)->c_str()));
}

void file_pane_t::update_column_widths(void) {
	int i, sum_width;

	if (file_list == NULL)
		return;

	for (i = 0, sum_width = 0; i < _FP_MAX_COLUMNS && sum_width < width &&
			top_idx + i * height < file_list->get_length(); i++)
	{
		update_column_width(i, top_idx + i * height);
		sum_width += column_widths[i] + 2;
	}

	columns_visible = i;
	if (sum_width > width) {
		if (i > 1) {
			sum_width -= column_widths[i - 1] + 2;
			columns_visible = i - 1;
		} else {
			column_widths[0] = width;
		}
	}

	for (i = 0; i < columns_visible; i++)
		column_widths[i] += (width - sum_width) / columns_visible;
	column_positions[0] = 0;
	for (i = 1; i < columns_visible; i++)
		column_positions[i] = column_positions[i - 1] + column_widths[i - 1] + 2;
}

void file_pane_t::content_changed(void) {
	top_idx = 0;
	update_column_widths();
	scrollbar.set_parameters(file_list->get_length(), 0, columns_visible * height);
	redraw = true;
}
