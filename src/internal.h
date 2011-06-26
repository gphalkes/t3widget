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

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <string>
#include <sigc++/sigc++.h>
#include <t3widget/widget_api.h>

namespace t3_widget {

class text_line_t;
T3_WIDGET_LOCAL extern text_line_t *copy_buffer;

#ifdef _T3_WIDGET_DEBUG
#define ASSERT(_x) do { if (!(_x)) { \
	fprintf(stderr, "%s:%d: libt3widget: Assertion failed: %s\n", __FILE__, __LINE__, #_x); abort(); \
}} while (0)
#else
#define ASSERT(_x)
#endif

#define ESCAPE_UNICODE (1<<29)
#define ESCAPE_REPLACEMENT (1<<30)

//FIXME: do proper gettext stuff instead of this temporary wrapper
#define _(_x) _x

T3_WIDGET_LOCAL int parse_escape(const std::string &str, const char **error_message, size_t &read_position,
	size_t max_read_position, bool replacements = false);
T3_WIDGET_LOCAL bool parse_escapes(std::string &str, const char **error_message, bool replacements = false);

/* Key handling routines. */
class complex_error_t;
T3_WIDGET_LOCAL complex_error_t init_keys(const char *term, bool separate_keypad);
T3_WIDGET_LOCAL void cleanup_keys(void);
T3_WIDGET_LOCAL void deinit_keys(void);
T3_WIDGET_LOCAL void reinit_keys(void);
T3_WIDGET_LOCAL void insert_protected_key(key_t key);

}; // namespace
#endif
