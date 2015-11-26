/* Copyright (C) 2011-2013 G.P. Halkes
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
#include "main.h"
#include "mainwindow.h"

using namespace std;
namespace t3_widget {

main_window_base_t::main_window_base_t(void) : dialog_t() {
	int height, width;
	t3_term_get_size(&height, &width);

	/* If the main window was created before init, we may get funny results. To
	   prevent crashes, correct them here. */
	if (height <= 0 || width <= 0) {
		height = 24;
		width = 80;
	}

	if ((window = t3_win_new_unbacked(NULL, height, width, 0, 0, INT_MAX)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
	connect_resize(signals::mem_fun(this, &main_window_base_t::set_size_real));
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

void main_window_base_t::update_contents(void) {
	redraw = false;
	dialog_t::update_contents();
}

void main_window_base_t::show(void) {
	dialog_base_t::show();

	dialog_t::active_dialogs.push_front(this);
	if (dialog_t::active_dialogs.back() == this)
		set_focus(window_component_t::FOCUS_SET);
}

void main_window_base_t::close(void) {}

void main_window_base_t::set_size_real(int height, int width) {
	t3_win_resize(window, height, width);
	set_size(height, width);
}

}; // namespace
