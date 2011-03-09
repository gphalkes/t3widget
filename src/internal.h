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
#ifndef T3_WIDGET_INTERNAL_H
#define T3_WIDGET_INTERNAL_H

#include <string>

namespace t3_widget {

class text_line_t;
extern text_line_t *copy_buffer;

#ifdef _T3_WIDGET_DEBUG
#define ASSERT(_x) do { if (!(_x)) { fprintf(stderr, "%s:%d: libt3widget: Assertion failed: %s\n", __FILE__, __LINE__, #_x); abort(); }} while (0)
#else
#define ASSERT(_x)
#endif

#define ESCAPE_UNICODE (1<<29)
#define ESCAPE_REPLACEMENT (1<<30)

int parse_escape(const std::string &str, const char **error_message, size_t &read_position, size_t max_read_position, bool replacements = false);
bool parse_escapes(std::string &str, const char **error_message, bool replacements);
}; // namespace
#endif
