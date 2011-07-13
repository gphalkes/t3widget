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
#ifndef T3_WIDGET_WRAPINFO_H
#define T3_WIDGET_WRAPINFO_H

#include <vector>

#include <t3widget/util.h>
#include <t3widget/textbuffer.h>

namespace t3_widget {

typedef std::vector<int> wrap_points_t;
typedef std::vector<wrap_points_t *> wrap_data_t;

/** Class holding information about wrapping a text_buffer_t.

    This class is required by edit_window_t and text_buffer_t to present the
    user with a wrapped text. Except for in the function find_line, it uses the
    text_coordinate_t class in a special way: the @c pos field is used to store
    the index in the array of wrap points for the line indicated by the @c line
    field.
*/
class wrap_info_t {
	private:
		wrap_data_t wrap_data;
		text_buffer_t *text;
		int size,
			tabsize,
			wrap_width;
		sigc::connection rewrap_connection;

		void delete_lines(int first, int last);
		void insert_lines(int first, int last);
		void rewrap_line(int line, int pos, bool force);
		void rewrap_all(void);
		void rewrap(rewrap_type_t type, int a, int b);

	public:
		wrap_info_t(int width, int tabsize = 8);
		~wrap_info_t(void);
		int get_size(void) const;

		void set_wrap_width(int width);
		void set_tabsize(int _tabsize);
		void set_text_buffer(text_buffer_t *_text);

		bool add_lines(text_coordinate_t &coord, int count) const;
		bool sub_lines(text_coordinate_t &coord, int count) const;
		int get_line_count(int line) const;
		text_coordinate_t get_end(void) const;
		int find_line(text_coordinate_t coord) const;
		int calculate_screen_pos(void) const;
		int calculate_line_pos(int line, int pos, int subline) const;
		void paint_line(t3_window_t *win, text_coordinate_t line, text_line_t::paint_info_t *info) const;
};

}; // namespace
#endif
