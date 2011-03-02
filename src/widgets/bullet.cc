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
#include "widgets/bullet.h"

bullet_t::bullet_t(container_t *parent, window_component_t *anchor, int _top, int _left, int relation,
	bullet_status_t *_source) : source(_source), top(_top), left(_left), focus(false)
{
	if ((window = t3_win_new_relative(parent->get_draw_window(), 1, 1, top, left, 0, anchor->get_draw_window(), relation)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
	update_contents();
}

void bullet_t::process_key(key_t key) { (void) key; }
bool bullet_t::resize(optint height, optint width, optint _top, optint _left) {
	(void) height;
	(void) width;

	if (_top.is_valid())
		top = _top;
	if (_left.is_valid())
		left = _left;
	t3_win_move(window, top, left);
	return true;
}

void bullet_t::update_contents(void) {
	t3_win_set_paint(window, 0, 0);
	if (source->get_bullet_status())
		t3_win_addch(window, T3_ACS_DIAMOND, T3_ATTR_ACS | (focus ? T3_ATTR_REVERSE : 0)); //FIXME: use proper attributes
	else
		t3_win_addch(window, ' ', focus ? T3_ATTR_REVERSE : 0);
}

void bullet_t::set_focus(bool _focus) {
	focus = _focus;
}

