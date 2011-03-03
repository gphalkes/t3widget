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

using namespace std;

#warning FIXME: should use container_t for parent and allow anchor
file_pane_t::file_pane_t(t3_window_t *_parent, int _height, int _width, int _top, int _left) : height(_height - 2),
	width(_width - 2), top(_top), left(_left), top_idx(0), parent(_parent), file_list(NULL)
{
	window = t3_win_new(parent, height, width, top + 1, left + 1, 0);
	t3_win_set_default_attrs(window, colors.dialog_attrs);
	field = NULL;
	focus = false;
	current = 0;
	t3_win_show(window);
}

void file_pane_t::set_text_field(text_field_t *_field) {
	field = _field;
}

void file_pane_t::ensure_cursor_on_screen(void) {
	while (current >= top_idx + 2 * height)
		top_idx += height;
	while (current < top_idx && top_idx > (size_t) height)
		top_idx -= height;
	if (top_idx > current)
		top_idx = 0;
}

void file_pane_t::process_key(key_t key) {
	switch (key) {
		case EKEY_DOWN:
			if (current + 1 >= file_list->get_length())
				return;
			current++;
			break;
		case EKEY_UP:
			if (current == 0)
				return;
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
			return;
		default:
			return;
	}
	if (field)
		field->set_text(file_list->get_name(current)->c_str());
	ensure_cursor_on_screen();
}

bool file_pane_t::resize(optint _height, optint _width, optint _top, optint _left) {
	if (_height.is_valid())
		height = _height - 2;
	if (_width.is_valid())
		width = _width - 2;
	if (_top.is_valid())
		top = _top;
	if (_left.is_valid())
		left = _left;
	t3_win_resize(window, height, width);
	t3_win_move(window, top + 1, left + 1);

	ensure_cursor_on_screen();
	return true;
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

	if (column > 1)
		return;

	t3_win_set_paint(window, idx, (width >> 1) * column);
	t3_win_addch(window, is_dir ? '/' : ' ', selected ? colors.dialog_selected_attrs : 0);

	info.start = 0;
	info.leftcol = 0;
	info.max = INT_MAX;
	info.size = width / 2 - 2;
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

	if (file_list == NULL)
		return;

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	for (i = top_idx, max_idx = min(top_idx + 2 * height, file_list->get_length()); i < max_idx; i++)
		draw_line(i, focus && i == current);

	t3_win_box(parent, top, left, height + 2, width + 2, 0);
}

void file_pane_t::set_focus(bool _focus) {
	focus = _focus;
	if (focus) {
		t3_term_hide_cursor();
		draw_line(current, true);
	} else {
		draw_line(current, false);
	}
}

bool file_pane_t::accepts_enter(void) { return true; }

void file_pane_t::reset(void) {
	top_idx = 0;
	current = 0;
}

void file_pane_t::set_file_list(file_list_t *_fileList) {
	file_list = _fileList;
	update_contents();
}

void file_pane_t::set_file(size_t idx) {
	current = idx < file_list->get_length() ? idx : 0;
	ensure_cursor_on_screen();
	/* if (field)
		field->set_text(idx == 0 ? "" : file_list->get_name(current)->c_str()); */
}
