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
#include "widgets/widgets.h"

base_widget_t::~base_widget_t(void) {}

bool base_widget_t::is_hotkey(key_t key) {
	(void) key;
	return false;
}

bool base_widget_t::accepts_focus(void) { return shown; }

widget_t::widget_t(void) {
	window = NULL;
}

widget_t::~widget_t(void) {
	t3_win_del(window);
}

void widget_t::set_show(bool show) {
	if (window == NULL)
		return;

	shown = show;
	if (show)
		t3_win_show(window);
	else
		t3_win_hide(window);
}