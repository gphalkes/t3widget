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
#include "widgets/widget.h"

using namespace std;
namespace t3_widget {

bool widget_t::is_hotkey(key_t key) {
	(void) key;
	return false;
}

bool widget_t::accepts_focus(void) { return true; }

widget_t::widget_t(container_t *parent, int height, int width) {
	init_window(parent, height, width);
}

widget_t::widget_t(void) : window(NULL) {}

void widget_t::init_window(container_t *parent, int height, int width) {
	if ((window = t3_win_new(parent->get_draw_window(), height, width, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
	/* Make sure the colors are initialized, because everything else expects
	   them to be. */
	init_colors();
}

widget_t::~widget_t(void) {
	t3_win_del(window);
}

void widget_t::set_anchor(window_component_t *anchor, int relation) {
	t3_win_set_anchor(window, anchor->get_draw_window(), relation);
}

void widget_t::set_position(optint top, optint left) {
	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);

	t3_win_move(window, top, left);
}

void widget_t::show(void) {
	t3_win_show(window);
}

void widget_t::hide(void) {
	t3_win_hide(window);
}

}; // namespace
