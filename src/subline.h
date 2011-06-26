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
#ifndef T3_WIDGET_SUBLINE_H
#define T3_WIDGET_SUBLINE_H

#include <t3widget/textline.h>

namespace t3_widget {

class subtext_line_t {
	private:
		text_line_t *line;
		int start;
		int flags;

	public:
		subtext_line_t(text_line_t *_line, int _start) : line(_line), start(_start), flags(0) {}

		void set_start(int start);
		int get_start(void) const;
		text_line_t *get_line(void) const;
		void set_line(text_line_t *line);
		int get_flags(void) const;
		void set_flags(int flags);
};

}; // namespace
#endif
