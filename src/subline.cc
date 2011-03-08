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
#include "subline.h"

namespace t3_widget {

/* set the start member of the subtext_line_t, and return if it was changed. */
void subtext_line_t::set_start(int _start) { start = _start; }
int subtext_line_t::get_start(void) const { return start; }
text_line_t *subtext_line_t::get_line(void) const { return line; }
void subtext_line_t::set_line(text_line_t *_line) { line = _line; }
int subtext_line_t::get_flags(void) const { return flags; }
void subtext_line_t::set_flags(int _flags) { flags = _flags; }

}; // namespace
