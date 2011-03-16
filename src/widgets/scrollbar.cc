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

#include "colorscheme.h"
#include "widgets/scrollbar.h"
#include "log.h"

namespace t3_widget {

scrollbar_t::scrollbar_t(container_t *parent, bool _vertical) :
	length(3), range(1), start(0), used(1), vertical(_vertical)
{
	int width, height;

	if (vertical) {
		width = 1;
		height = length;
	} else {
		width = length;
		height = 1;
	}

	init_window(parent, height, width);
}

bool scrollbar_t::process_key(key_t key) {
	(void) key;
	return false;
}

bool scrollbar_t::set_size(optint height, optint width) {
	bool result = true;
	if (vertical) {
		if (height.is_valid()) {
			length = height;
			result = t3_win_resize(window, length, 1);
		}
	} else {
		if (width.is_valid()) {
			length = width;
			result = t3_win_resize(window, 1, length);
		}
	}
	return result;
}

void scrollbar_t::update_contents(void) {
	int before, slider_size, i;
	double blocks_per_line;

	if (!redraw)
		return;
	redraw = false;

	t3_win_set_default_attrs(window, colors.scrollbar_selected_attrs);

	blocks_per_line = (double) (length - 2) / range;
	slider_size = blocks_per_line * used;
	if (slider_size == 0)
		slider_size = 1;
	/* Recalulate the number of blocks per line, because the slider may actually
	   be larger than it should be. */
	if (range <= used)
		blocks_per_line = strtod("Inf", NULL);
	else
		blocks_per_line = (double) (length - 2 - slider_size) / (range - used);

	before = ceil(blocks_per_line * start);
	if (before >= length - 2)
		before = length - 3;

	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, vertical ? T3_ACS_UARROW : T3_ACS_LARROW, T3_ATTR_ACS | colors.scrollbar_attrs);

	for (i = 1; i < length - 1 && i < before + 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS);
	}
	for (; i < length - 1 && i < before + slider_size + 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, ' ', 0);
	}
	for (; i < length - 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS);
	}

	if (vertical)
		t3_win_set_paint(window, length - 1, 0);
	t3_win_addch(window, vertical ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS | colors.scrollbar_attrs);
}


bool scrollbar_t::accepts_focus(void) {
	return false;
}

void scrollbar_t::set_focus(bool focus) {
	(void) focus;
}

void scrollbar_t::set_parameters(int _range, int _start, int _used) {
	if (range == _range && start == _start && used == _used)
		return;

	redraw = true;
	range = _range;
	start = _start;
	used = _used;
}

}; // namespace
