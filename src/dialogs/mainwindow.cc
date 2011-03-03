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
#include "mainwindow.h"


main_window_t::main_window_t(void) : dialog_t() {
	int height, width;
	t3_term_get_size(&height, &width);
	if ((window = t3_win_new_unbacked(NULL, height, width, 0, 0, -1, NULL, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
}

bool main_window_t::resize(optint _height, optint _width, optint _top, optint _left) {
	int height, width;

	(void) _height;
	(void) _width;
	(void) _top;
	(void) _left;

	t3_term_get_size(&height, &width);
	return t3_win_resize(window, height, width);
}
