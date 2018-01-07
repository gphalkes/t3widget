/* Copyright (C) 2011-2012 G.P. Halkes
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
#include "widgets/separator.h"

namespace t3_widget {

separator_t::separator_t(bool _horizontal) : widget_t(1, 1, false), horizontal(_horizontal) {}

bool separator_t::set_size(optint height, optint width) {
	bool result = true;
	if (horizontal) {
		if (width.is_valid())
			result = t3_win_resize(window, 1, width);
	} else {
		if (height.is_valid())
			result = t3_win_resize(window, height, 1);
	}

	return result;
}

bool separator_t::process_key(key_t key) { (void) key; return false; }

void separator_t::update_contents(void) {
	t3_win_set_default_attrs(window, attributes.dialog);
	if (horizontal) {
		t3_win_set_paint(window, 0, 0);
		t3_win_addchrep(window, T3_ACS_HLINE, T3_ATTR_ACS, t3_win_get_width(window));
	} else {
		int i, height = t3_win_get_height(window);
		for (i = 0; i < height; i++) {
			t3_win_set_paint(window, i, 0);
			t3_win_addch(window, T3_ACS_VLINE, T3_ATTR_ACS);
		}
	}
}

void separator_t::set_focus(focus_t focus) { (void) focus; }
bool separator_t::accepts_focus(void) { return false; }

}; // namespace
