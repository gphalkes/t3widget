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

using namespace std;

bullet_t::bullet_t(container_t *parent, bullet_status_t *_source) : widget_t(parent, 1, 1), source(_source), focus(false) {}

bool bullet_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

bool bullet_t::process_key(key_t key) { (void) key; return false; }

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

