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
		int get_size(void);

		void set_wrap_width(int width);
		void set_tabsize(int _tabsize);
		void set_text_buffer(text_buffer_t *_text);
};

}; // namespace
#endif
