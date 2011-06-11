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
#ifndef T3_WIDGET_FILEPANE_H
#define T3_WIDGET_FILEPANE_H

#include <string>

#include "widgets/widget.h"
#include "contentlist.h"
#include "widgets/scrollbar.h"
#include "widgets/textfield.h"

#define _T3_WDIGET_FP_MAX_COLUMNS 8

namespace t3_widget {

class file_pane_t : public widget_t, public container_t {
	private:
		scrollbar_t scrollbar;
		int height, width;
		size_t top_idx, current;
		file_list_t *file_list;
		bool focus;
		text_field_t *field;
		int column_widths[_T3_WDIGET_FP_MAX_COLUMNS], column_positions[_T3_WDIGET_FP_MAX_COLUMNS], columns_visible, scrollbar_range;
		sigc::connection content_changed_connection;

		void ensure_cursor_on_screen(void);
		void draw_line(int idx, bool selected);
		void update_column_width(int column, int start);
		void update_column_widths(void);
		void content_changed(void);
	public:
		file_pane_t(void);
		void set_text_field(text_field_t *_field);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		void reset(void);
		void set_file_list(file_list_t *_file_list);
		void set_file(const std::string *name);

	T3_WIDGET_SIGNAL(activate, void, const std::string *);
};

}; // namespace

#endif
