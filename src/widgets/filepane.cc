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
#include "colorscheme.h"
#include "widgets/filepane.h"
#include "log.h"

using namespace std;
namespace t3_widget {

//FIXME: we could use some optimization for update_column_widths. Current use is simple but calls to often.
file_pane_t::file_pane_t(void) : widget_t(2, 1), scrollbar(false), top_idx(0), current(0), file_list(NULL),
		focus(false), field(NULL), columns_visible(0), scrollbar_range(1)
{
	set_widget_parent(&scrollbar);
	scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
}

file_pane_t::~file_pane_t(void) {
	content_changed_connection.disconnect();
}

void file_pane_t::set_text_field(text_field_t *_field) {
	field = _field;
}

void file_pane_t::ensure_cursor_on_screen(void) {
	size_t old_top_idx = top_idx;
	int height;

	if (file_list == NULL)
		return;

	height = t3_win_get_height(window) - 1;

	while (current >= top_idx + columns_visible * height)
		top_idx += height;
	while (current < top_idx && top_idx > (size_t) height)
		top_idx -= height;
	if (top_idx > current)
		top_idx = 0;
	if (top_idx != old_top_idx) {
		update_column_widths();
		ensure_cursor_on_screen();
		lprintf("setting scrollbar params: %d, %zd, %d\n", scrollbar_range, top_idx, columns_visible * height);
		scrollbar.set_parameters(scrollbar_range, top_idx, columns_visible * height);
	}
}

bool file_pane_t::process_key(key_t key) {
	int height;
	switch (key) {
		case EKEY_DOWN:
			if (current + 1 >= file_list->size())
				return true;
			current++;
			redraw = true;
			break;
		case EKEY_UP:
			if (current == 0)
				return true;
			current--;
			redraw = true;
			break;
		case EKEY_RIGHT:
			height = t3_win_get_height(window) - 1;
			if (current + height >= file_list->size()) {
				if (file_list->size() == 0)
					return true;
				current = file_list->size() - 1;
			} else {
				current += height;
			}
			redraw = true;
			break;
		case EKEY_LEFT:
			height = t3_win_get_height(window) - 1;
			if (current < (size_t) height)
				current = 0;
			else
				current -= height;
			redraw = true;
			break;
		case EKEY_END:
			current = file_list->size() - 1;
			redraw = true;
			break;
		case EKEY_HOME:
			current = 0;
			redraw = true;
			break;
		case EKEY_PGDN:
			height = t3_win_get_height(window) - 1;
			if (current + 2 * height >= file_list->size()) {
				current = file_list->size() - 1;
			} else {
				current += 2 * height;
				top_idx += 2 * height;
			}
			redraw = true;
			break;
		case EKEY_PGUP:
			height = t3_win_get_height(window) - 1;
			if (current < (size_t) 2 * height) {
				current = 0;
			} else {
				current -= 2 * height;
				if (top_idx < (size_t) 2 * height)
					top_idx = 0;
				else
					top_idx -= 2 * height;
			}
			redraw = true;
			break;
		case EKEY_NL:
			activate(file_list->get_fs_name(current));
			return true;
		default:
			return false;
	}
	if (file_list->size() != 0) {
		if (field)
			field->set_text((*file_list)[current]->c_str());
		ensure_cursor_on_screen();
	}
	return true;
}

bool file_pane_t::set_size(optint height, optint width) {
	bool result;
	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_height(window);
	result = t3_win_resize(window, height, width);
	result &= scrollbar.set_size(None, width);

	height = height - 1;
	if (file_list != NULL && file_list->size() != 0) {
		update_column_widths();
		scrollbar_range = ((file_list->size() + height - 1) / height) * height;
		ensure_cursor_on_screen();
		scrollbar.set_parameters(scrollbar_range, top_idx, columns_visible * height);
	}
	redraw = true;
	return result;
}

void file_pane_t::draw_line(int idx, bool selected) {
	if ((size_t) idx < top_idx || (size_t) idx >= file_list->size())
		return;

	int column;
	int height = t3_win_get_height(window) - 1;
	text_line_t line((*file_list)[idx]);
	bool is_dir = file_list->is_dir(idx);
	text_line_t::paint_info_t info;

	idx -= top_idx;
	column = idx / height;
	idx %= height;

	t3_win_set_paint(window, idx, column_positions[column]);
	t3_win_addch(window, is_dir ? '/' : ' ', selected ? attributes.dialog_selected : 0);

	info.start = 0;
	info.leftcol = 0;
	info.max = INT_MAX;
	info.size = column_widths[column];
	info.tabsize = 0;
	info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL | (selected ? text_line_t::EXTEND_SELECTION : 0);
	info.selection_start = -1;
	info.selection_end = selected ? INT_MAX : -1;
	info.cursor = -1;
	info.normal_attr = attributes.dialog;
	info.selected_attr = attributes.dialog_selected;

	line.paint_line(window, &info);
}

void file_pane_t::update_contents(void) {
	size_t max_idx, i;

	if (!redraw)
		return;
	redraw = false;

	t3_win_set_default_attrs(window, attributes.dialog);

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	if (file_list == NULL)
		return;

	for (i = top_idx, max_idx = min(top_idx + columns_visible * (t3_win_get_height(window) - 1), file_list->size()); i < max_idx; i++)
		draw_line(i, focus && i == current);

	scrollbar.update_contents();
}

void file_pane_t::set_focus(bool _focus) {
	focus = _focus;
	if (file_list != NULL)
		draw_line(current, focus);
}

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
	redraw = true;
}

void file_pane_t::set_file(const string *name) {
	for (current = 0; current < file_list->size(); current++) {
		if (name->compare(*(*file_list)[current]) == 0)
			break;
	}
	if (current == file_list->size())
		current = 0;

	ensure_cursor_on_screen();
}

void file_pane_t::update_column_width(int column, int start) {
	int height = t3_win_get_height(window) - 1;
	column_widths[column] = 0;
	for (int i = 0; i < height && start + i < (int) file_list->size(); i++)
		column_widths[column] = max(column_widths[column], t3_term_strwidth((*file_list)[i + start]->c_str()));
}

void file_pane_t::update_column_widths(void) {
	int i, sum_width;
	int height = t3_win_get_height(window) - 1;
	int width = t3_win_get_width(window);

	if (file_list == NULL)
		return;

	for (i = 0, sum_width = 0; i < _T3_WDIGET_FP_MAX_COLUMNS && sum_width < width &&
			top_idx + i * height < file_list->size(); i++)
	{
		update_column_width(i, top_idx + i * height);
		sum_width += column_widths[i] + 1;
	}

	columns_visible = i;
	if (sum_width > width) {
		if (i > 1) {
			sum_width -= column_widths[i - 1] + 1;
			columns_visible = i - 1;
		} else {
			column_widths[0] = width;
			sum_width = width;
		}
	}
	if (columns_visible == 0)
		columns_visible = 1;

	for (i = 0; i < columns_visible; i++)
		column_widths[i] += (width - sum_width) / columns_visible;
	sum_width += columns_visible * ((width - sum_width) / columns_visible);
	for (i = 0; i < columns_visible; i++)
		column_widths[i]++;
	column_positions[0] = 0;
	for (i = 1; i < columns_visible; i++)
		column_positions[i] = column_positions[i - 1] + column_widths[i - 1] + 1;
}

void file_pane_t::content_changed(void) {
	int height = t3_win_get_height(window) - 1;

	top_idx = 0;
	update_column_widths();
	scrollbar_range = ((file_list->size() + height - 1) / height) * height;
	scrollbar.set_parameters(scrollbar_range, 0, columns_visible * height);
	redraw = true;
}

}; // namespace
