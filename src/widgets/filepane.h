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
#ifndef FILEPANE_H
#define FILEPANE_H

#include "window/window.h"
#include "widgets/widgets.h"
#include "widgets/contentlist.h"

class file_pane_t : public widget_t {
	private:
		int height, width, top, left;
		size_t top_idx, current;
		t3_window_t *parent;
		file_list_t *file_list;
		bool focus;
		text_field_t *field;

		void ensure_cursor_on_screen(void);
		void draw_line(int idx, bool selected);
	public:
		file_pane_t(t3_window_t *_parent, int _height, int _width, int _top, int _left);
		void set_text_field(text_field_t *_field);
		virtual void process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual bool accepts_enter(void);
		void reset(void);
		void set_file_list(file_list_t *_fileList);
		void set_file(size_t idx);

	SIGNAL(activate, void, const std::string *);
};

#endif
