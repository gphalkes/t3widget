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
#include "label.h"
#include "window/window.h"

using namespace std;

//FIXME: maybe we should allow scrolling with the left and right keys

label_t::label_t(container_t *parent, window_component_t *anchor,
	int top, int left, optint _width, int relation, const char *_text) : text(_text), align(ALIGN_LEFT)
{
	text_width = t3_term_strwidth(text);
	width = _width.is_valid() ? (int) _width : text_width;

	if ((window = t3_win_new_relative(parent->get_draw_window(), 1, width, top, left, 0, anchor->get_draw_window(), relation)) == NULL)
		throw bad_alloc();
	redraw = true;
	t3_win_show(window);
	update_contents();
}


void label_t::process_key(key_t key) {
	(void) key;
}

bool label_t::resize(optint height, optint _width, optint top, optint left) {
	(void) height;
	(void) _width;

	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);
	t3_win_move(window, top, left);

	if (_width.is_valid() && width != _width) {
		width = _width;
		t3_win_resize(window, 1, width);
		redraw = true;
		update_contents();
	}
	return true;
}

void label_t::update_contents(void) {
	if (!redraw)
		return;
	redraw = false;

	line_t *line = new line_t(text);
	line_t::paint_info_t paint_info;

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtoeol(window);
	t3_win_set_paint(window, 0, width > text_width && (align == ALIGN_RIGHT || align == ALIGN_RIGHT_UNDERFLOW) ? width - text_width : 0);

	paint_info.start = 0;
	if (width < text_width && (align == ALIGN_LEFT_UNDERFLOW || align == ALIGN_RIGHT_UNDERFLOW)) {
		paint_info.leftcol = text_width - width + 2;
		paint_info.size = width - 2;
		t3_win_addstr(window, "..", 0);
	} else {
		paint_info.leftcol = 0;
		paint_info.size = width;
	}
	paint_info.max = INT_MAX;
	paint_info.tabsize = 0;
	paint_info.flags = line_t::TAB_AS_CONTROL;
	paint_info.selection_start = -1;
	paint_info.selection_end = -1;
	paint_info.cursor = -1;
	paint_info.normal_attr = 0;
	paint_info.selected_attr = 0;

	line->paint_line(window, &paint_info);
	delete line;
}

void label_t::set_focus(bool focus) {
	redraw = true;
	t3_win_set_default_attrs(window, focus ? T3_ATTR_REVERSE : 0);  //FIXME: use proper option.xxx attributes
	if (focus)
		t3_term_hide_cursor();
}

void label_t::set_align(label_t::align_t _align) {
	align = _align;
}
