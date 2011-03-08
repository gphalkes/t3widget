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

using namespace std;
namespace t3_widget {

main_window_base_t::main_window_base_t(void) : dialog_t() {
	if ((window = t3_win_new_unbacked(NULL, 25, 80, 0, 0, INT_MAX)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
	connect_resize(sigc::mem_fun(this, &main_window_base_t::set_size_real));
}

bool main_window_base_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void main_window_base_t::set_position(optint top, optint left) {
	(void) top;
	(void) left;
}

void main_window_base_t::set_size_real(int height, int width) {
	t3_win_resize(window, height, width);
	set_size(height, width);
}

}; // namespace
