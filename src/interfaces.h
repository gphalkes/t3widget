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
#ifndef INTERFACES_H
#define INTERFACES_H

#include <cstdarg>
#include <list>

using namespace std;

#include "keys.h"
#include "util.h"
#include "window/window.h"

class window_component_t {
	public:
		virtual void process_key(key_t key) = 0;
		virtual bool resize(optint height, optint width, optint top, optint left) = 0;
		virtual void update_contents(void) = 0;
		virtual void set_focus(bool focus) = 0;
		virtual void set_show(bool show) = 0;
		virtual t3_window_t *get_draw_window(void) = 0;
};

class container_t {
	public:
		virtual t3_window_t *get_draw_window(void) = 0;
};

typedef list<window_component_t *> window_components_t;

class bullet_status_t {
	public:
		virtual bool get_bullet_status(void) = 0;
};

#endif
