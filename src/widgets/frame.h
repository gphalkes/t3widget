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
#ifndef FRAME_H
#define FRAME_H

#include "widgets/widgets.h"

class frame_t : public widget_t, public container_t {
	public:
		enum frame_dimension_t {
			AROUND_ALL = 0,
			COVER_BOTTOM = 1,
			COVER_RIGHT = 2,
			COVER_LEFT = 4,
			COVER_TOP = 8
		};

		frame_t(container_t *parent, frame_dimension_t _dimension = AROUND_ALL);
		void set_child(widget_t *_child);
		virtual bool process_key(key_t key);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual bool set_size(optint height, optint width);
		virtual t3_window_t *get_draw_window(void);
 		virtual bool accepts_focus(void);
		virtual bool is_hotkey(key_t key);

	private:
		frame_dimension_t dimension;
		widget_t *child;
};

#endif