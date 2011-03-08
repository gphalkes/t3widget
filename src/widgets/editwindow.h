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
#ifndef T3_WIDGET_EDITWINDOW_H
#define T3_WIDGET_EDITWINDOW_H

namespace t3_widget {
	class edit_window_t;
}; // namespace

#include "window/window.h"
#include "widgets/widgets.h"
#include "widgets/scrollbar.h"
#include "textbuffer.h"
#include "keys.h"
#include "interfaces.h"

namespace t3_widget {

class edit_window_t : public widget_t {
	private:
		t3_window_t *bottomlinewin;
		scrollbar_t *scrollbar;
		text_buffer_t *text;
		int screen_pos; // Cached position of cursor in screen coordinates
		bool focus, need_repaint, hard_cursor;

		static const char *insstring[];
		static bool (text_buffer_t::*proces_char[])(key_t);

		void setActive(bool _active);
		void ensure_cursor_on_screen(void);
		void repaint_screen(void);
		void inc_x(void);
		void next_word(void);
		void dec_x(void);
		void previous_word(void);
		void inc_y(void);
		void dec_y(void);
		void pgdn(void);
		void pgup(void);
		void reset_selection(void);
		void set_selection_mode(key_t key);
		void delete_selection(void);

	public:
		edit_window_t(container_t *parent, text_buffer_t *_text = NULL);
		virtual ~edit_window_t(void);
		virtual void set_text_file(text_buffer_t *_text);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual t3_window_t *get_draw_window(void);

/*		void next_buffer(void);
		void previous_buffer(void);*/

/*		void save(void);
		void save_as(const std::string *name, Encoding *encoding);*/

		void close(bool force);
		void goto_line(int line);

		bool find(const std::string *what, int flags, const line_t *replacement);
		void replace(void);
		void get_dimensions(int *height, int *width, int *top, int *left);
		bool get_selection_lines(int *top, int *bottom);

		void undo(void);
		void redo(void);
		void cut_copy(bool cut);
		void paste(void);
		void select_all(void);
		const text_buffer_t *get_text_file(void);
};

}; // namespace
#endif
