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
#include "interfaces.h"
#include "widgets/widget.h"
#include "main.h"
using namespace std;

namespace t3_widget {

window_component_t::window_component_t(void) : window(NULL) {}
window_component_t::~window_component_t(void) {}
t3_window_t *window_component_t::get_base_window(void) { return window; }

bool container_t::set_widget_parent(widget_t *widget) {
	return t3_win_set_parent(widget->get_base_window(), window);
}

void container_t::unset_widget_parent(widget_t *widget) {
	t3_win_set_parent(widget->get_base_window(), widget_t::default_parent);
}

center_component_t::center_component_t(void) : center_window(this) {}
void center_component_t::set_center_window(window_component_t *_center_window) { center_window = _center_window; }


list<bad_draw_recheck_t *> bad_draw_recheck_t::to_signal;
sigc::connection bad_draw_recheck_t::initialized =
	connect_terminal_settings_changed(sigc::ptr_fun(bad_draw_recheck_t::bad_draw_recheck_all));

void bad_draw_recheck_t::bad_draw_recheck_all(void) {
	for (list<bad_draw_recheck_t *>::iterator iter = to_signal.begin(); iter != to_signal.end(); iter++)
		(*iter)->bad_draw_recheck();
}

bad_draw_recheck_t::bad_draw_recheck_t(void) {
	to_signal.push_back(this);
}

bad_draw_recheck_t::~bad_draw_recheck_t(void) {
	for (list<bad_draw_recheck_t *>::iterator iter = to_signal.begin(); iter != to_signal.end(); iter++) {
		if (*iter == this) {
			to_signal.erase(iter);
			return;
		}
	}
}

}; // namespace
