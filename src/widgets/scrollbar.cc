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
#include <cmath>

#include "colorscheme.h"
#include "widgets/scrollbar.h"
#include "log.h"

namespace t3_widget {

scrollbar_t::scrollbar_t(bool _vertical) :
	length(3), range(1), start(0), used(1), vertical(_vertical), before(0), slider_size(length - 2)
{
	int width, height;

	if (vertical) {
		width = 1;
		height = length;
	} else {
		width = length;
		height = 1;
	}

	init_window(height, width);
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
	redraw = true;
	return result;
}

void scrollbar_t::update_contents(void) {
	int i;
	double blocks_per_line;

	if (!redraw)
		return;
	redraw = false;

	blocks_per_line = (double) (length - 2) / range;
	slider_size = (int) (blocks_per_line * used);
	if (slider_size == 0)
		slider_size = 1;
	/* Rounding errors may cause incorrect slider sizing. This is normally not a
	   problem, except for the case where the slider should be maximal. */
	else if (range <= used)
		slider_size = length - 2;

	/* Recalulate the number of blocks per line, because the slider may actually
	   be larger than it should be. */
	if (range <= used)
		blocks_per_line = strtod("Inf", NULL);
	else
		blocks_per_line = (double) (length - 2 - slider_size) / (range - used + 1);

	before = (int) ceil(blocks_per_line * start);
	if (before >= length - 2)
		before = length - 3;
	if (before > 1 && before + slider_size == length - 2 && used + start < range)
		before--;
	else if (used + start == range)
		before = length - 2 - slider_size;

	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, vertical ? T3_ACS_UARROW : T3_ACS_LARROW, T3_ATTR_ACS | attributes.scrollbar);

	for (i = 1; i < length - 1 && i < before + 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS | attributes.scrollbar);
	}
	for (; i < length - 1 && i < before + slider_size + 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, ' ', attributes.scrollbar);
	}
	for (; i < length - 1; i++) {
		if (vertical)
			t3_win_set_paint(window, i, 0);
		t3_win_addch(window, T3_ACS_CKBOARD, T3_ATTR_ACS | attributes.scrollbar);
	}

	if (vertical)
		t3_win_set_paint(window, length - 1, 0);
	t3_win_addch(window, vertical ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS | attributes.scrollbar);
}


bool scrollbar_t::accepts_focus(void) { return false; }
void scrollbar_t::set_focus(focus_t focus) { (void) focus; }

bool scrollbar_t::process_mouse_event(mouse_event_t event) {
	/* FIXME: allow drag of slider */
	if (event.type == EMOUSE_BUTTON_RELEASE && (event.button_state & EMOUSE_CLICKED_LEFT) != 0) {
		int pos;
		pos = vertical ? event.y : event.x;

		if (pos == 0)
			clicked(BACK_SMALL);
		else if (pos == length - 1)
			clicked(FWD_SMALL);
		else if (pos <= before)
			clicked(BACK_PAGE);
		else if (pos > before + slider_size)
			clicked(FWD_PAGE);
	} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
		clicked((event.button_state & EMOUSE_SCROLL_UP) ? BACK_MEDIUM : FWD_MEDIUM);
	}
	/* We don't take focus, so return false. */
	return false;
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
