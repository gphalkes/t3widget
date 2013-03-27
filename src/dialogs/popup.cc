/* Copyright (C) 2013 G.P. Halkes
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
#include "popup.h"

namespace t3_widget {

/** Base class for dialogs. */
popup_t::popup_t(int height, int width, bool shadow, bool _draw) : dialog_base_t(height, width, shadow), draw(_draw) {}

void popup_t::update_contents(void) {
	if (!draw)
		redraw = false;
	dialog_base_t::update_contents();
}

} // namespace
