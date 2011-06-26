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
#ifndef T3_WIDGET_TEXTWINDOW_H
#define T3_WIDGET_TEXTWINDOW_H

#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/textbuffer.h>
#include <t3widget/interfaces.h>

namespace t3_widget {

class T3_WIDGET_API text_window_t : public text_buffer_window_t, public center_component_t, public container_t {
	protected:
		scrollbar_t scrollbar;
		text_buffer_t *text;

		void inc_y(void);
		void dec_y(void);
		void pgdn(void);
		void pgup(void);

	public:
		text_window_t(text_buffer_t *_text = NULL);
		virtual void set_text(text_buffer_t *_text);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);

		virtual int get_text_width(void);
		text_buffer_t *get_text(void);
};

}; // namespace
#endif
