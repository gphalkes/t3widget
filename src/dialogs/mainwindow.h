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
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "interfaces.h"
#include "dialogs/dialogs.h"

class main_window_t : public window_component_t {
	private:
		friend class dialog_t;

		t3_window_t *window;
		main_window_t(void);

	public:
		virtual void process_key(key_t key) { (void) key; }
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void update_contents(void) {}
		virtual void set_focus(bool focus) { (void) focus; }
		virtual void set_show(bool show) { (void) show; }
		virtual t3_window_t *get_draw_window(void) { return window; }
		virtual ~main_window_t(void);
};

#endif
