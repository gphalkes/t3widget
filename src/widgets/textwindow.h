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
#ifndef T3_WIDGET_TEXTWINDOW_H
#define T3_WIDGET_TEXTWINDOW_H

#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/textbuffer.h>
#include <t3widget/interfaces.h>

namespace t3_widget {

class T3_WIDGET_API text_window_t : public widget_t, public center_component_t, public container_t {
	protected:
		cleanup_ptr<scrollbar_t>::t scrollbar;
		text_buffer_t *text;
		cleanup_ptr<wrap_info_t>::t wrap_info;
		text_coordinate_t top;
		bool focus;

		void inc_y(void);
		void dec_y(void);
		void pgdn(void);
		void pgup(void);

	public:
		text_window_t(text_buffer_t *_text = NULL, bool with_scrollbar = true);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual void focus_set(widget_t *target);
		virtual bool is_child(widget_t *component);

		void set_scrollbar(bool with_scrollbar);
		void set_text(text_buffer_t *_text);
		text_buffer_t *get_text(void);
		void set_tabsize(int size);
		int get_text_height(void);

	T3_WIDGET_SIGNAL(activate, void);
};

}; // namespace
#endif
