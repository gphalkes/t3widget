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
#include <cmath>

#include "widgets/widgets.h"

scrollbar_t::scrollbar_t(container_t *parent, window_component_t *anchor, int _top, int _left, int relation, int _length, bool _vertical) :
	top(_top), left(_left), length(_length), vertical(_vertical)
{
	int width, height;

	if (vertical) {
		width = 1;
		height = length;
	} else {
		width = length;
		height = 1;
	}

	if ((window = t3_win_new_relative(parent->get_draw_window(), height, width, top, left, 0, anchor->get_draw_window(), relation)) == NULL)
		throw(-1);
	t3_win_set_default_attrs(window, 0/*FIXME: option.scrollbar_selected_attrs */);

	range = 1;
	start = 0;
	used = 1;
	update_contents();
	t3_win_show(window);
}

void scrollbar_t::process_key(key_t key) {
	(void) key;
}

bool scrollbar_t::resize(optint _height, optint _width, optint _top, optint _left) {
	if (vertical) {
		if (_height.is_valid()) {
			length = _height;
			t3_win_resize(window, length, 1);
		}
	} else {
		if (_width.is_valid()) {
			length = _width;
			t3_win_resize(window, 1, length);
		}
	}

	if (_top.is_valid() || _left.is_valid()) {
		if (_top.is_valid())
			top = _top;
		if (_left.is_valid())
			left = left;
		t3_win_move(window, top, left);
	}
	//FIXME make resize dependent on results of resize operation
	return true;
}

void scrollbar_t::update_contents(void) {
	int before, slider_size, i;

	double blocks_per_line = (double) (t3_win_get_height(window) - 2) / range;
	slider_size = blocks_per_line * used;
	if (slider_size == 0)
		slider_size = 1;
	/* Recalulate the number of blocks per line, because the slider may actually
	   be larger than it should be. */
	if (range <= used)
		blocks_per_line = strtod("Inf", NULL);
	else
		blocks_per_line = (double) (t3_win_get_height(window) - 2 - slider_size) / (range - used);

	before = ceil(blocks_per_line * start);
	if (before >= t3_win_get_height(window) - 2)
		before = t3_win_get_height(window) - 3;

	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, T3_ACS_UARROW, T3_ATTR_ACS | colors.scrollbar_attrs);

	for (i = 1; i < t3_win_get_height(window) - 1 && i < before + 1; i++) {
		t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS);
	}
	for (; i < t3_win_get_height(window) - 1 && i < before + slider_size + 1; i++) {
		t3_win_set_paint(window, i, 0);
		t3_win_addch(window, ' ', 0);
	}
	for (; i < t3_win_get_height(window) - 1; i++) {
		t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS);
	}

	t3_win_set_paint(window, t3_win_get_height(window) - 1, 0);
	t3_win_addch(window, T3_ACS_DARROW, T3_ATTR_ACS | colors.scrollbar_attrs);
}


bool scrollbar_t::accepts_focus(void) {
	return false;
}

void scrollbar_t::set_focus(bool focus) {
	(void) focus;
}

void scrollbar_t::set_parameters(int _range, int _start, int _used) {
	range = _range;
	start = _start;
	used = _used;
}