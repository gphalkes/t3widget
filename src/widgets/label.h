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
#ifndef LABEL_H
#define LABEL_H

#include "widgets/widgets.h"

class label_t : public widget_t {
	public:
		enum align_t {
			ALIGN_LEFT,
			ALIGN_RIGHT,
			ALIGN_LEFT_UNDERFLOW,
			ALIGN_RIGHT_UNDERFLOW
		};

	private:
		const char *text;
		int width, text_width;
		align_t align;
		bool redraw;

	public:
		label_t(window_component_t *parent, window_component_t *anchor,
			int top, int left, optint _width, int relation, const char *_text);

		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint _width, optint top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);


		void set_align(align_t _align);
};

#endif

