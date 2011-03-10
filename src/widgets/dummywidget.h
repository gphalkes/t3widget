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
#ifndef T3_WIDGET_DUMMYWIDGET_H
#define T3_WIDGET_DUMMYWIDGET_H

#include "widgets/widget.h"

namespace t3_widget {

class dummy_widget_t : public widget_t {
	public:
		virtual bool process_key(key_t key);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);

		virtual t3_window_t *get_draw_window(void);
};

}; // namespace
#endif
