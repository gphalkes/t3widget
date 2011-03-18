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
#ifndef T3_WIDGET_INTERFACES_H
#define T3_WIDGET_INTERFACES_H

#include <cstdarg>
#include <list>
#include <window/window.h>

#include "key.h"
#include "util.h"

namespace t3_widget {

class window_component_t {
	protected:
		t3_window_t *window;
	public:
		window_component_t(void) : window(NULL) {}
		virtual t3_window_t *get_draw_window(void) { return window; }
		virtual bool process_key(key_t key) = 0;
		virtual void set_position(optint top, optint left) = 0;
		virtual bool set_size(optint height, optint width) = 0;
		virtual void update_contents(void) = 0;
		virtual void set_focus(bool focus) = 0;
		virtual void show(void) = 0;
		virtual void hide(void) = 0;
};

class container_t : private virtual window_component_t {
	public:
		virtual t3_window_t *get_draw_window(void) { return window_component_t::get_draw_window(); }
};

}; // namespace
#endif
