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
#ifndef T3_WIDGET_INTERNAL_H
#define T3_WIDGET_INTERNAL_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <string>
#include <sigc++/sigc++.h>
#include <t3widget/key.h>

#include "widget_api.h"
#include "main.h"
#include "log.h"

#ifdef HAS_STRDUP
#define _t3_widget_strdup strdup
#else
T3_WIDGET_LOCAL char *_t3_widget_strdup(const char *str);
#endif

namespace t3_widget {

T3_WIDGET_LOCAL extern init_parameters_t *init_params;

#ifdef _T3_WIDGET_DEBUG
#define ASSERT(_x) do { if (!(_x)) { \
	fprintf(stderr, "%s:%d: libt3widget: Assertion failed: %s\n", __FILE__, __LINE__, #_x); abort(); \
}} while (0)
#else
#define ASSERT(_x)
#endif

/** Mask for return values of #parse_escape, indicating that the escape value was a Unicode character. */
#define ESCAPE_UNICODE (1<<29)
/** Mask for return values of #parse_escape, indicating that the escape value was a replacement marker. */
#define ESCAPE_REPLACEMENT (1<<30)

//FIXME: do proper gettext stuff instead of this temporary wrapper
#define _(_x) _x

/** Parse a single escape sequence in a string.
    @param str The string to parse the escape from.
    @param error_message Pointer to storage for an error message.
    @param read_position The index in @p str to start reading, updated by parse_escape.
	@param replacements A boolean indicating whether replacement markers should be parsed as such.
*/
T3_WIDGET_LOCAL int parse_escape(const std::string &str, const char **error_message, size_t &read_position,
	bool replacements = false);
/** Convert escapes in a string to associated values.
    @param str The string to parse, updated by this function.
    @param error_message Pointer to storage for an error message.
	@param replacements A boolean indicating whether replacement markers should be parsed as such.
*/
T3_WIDGET_LOCAL bool parse_escapes(std::string &str, const char **error_message, bool replacements = false);

/* Key handling routines. */
class complex_error_t;
/** Initialize the key handling code. */
T3_WIDGET_LOCAL complex_error_t init_keys(const char *term, bool separate_keypad);
/** Clean-up any data allocated for the key handling code. */
T3_WIDGET_LOCAL void cleanup_keys(void);
/** Switch to the default keypad mode to allow other applications to function. */
T3_WIDGET_LOCAL void deinit_keys(void);
/** Switch back to best keypad mode after using #deinit_keys. */
T3_WIDGET_LOCAL void reinit_keys(void);
/** Insert a key to the queue, marked to ensure it is not interpreted by any widget except text widgets. */
T3_WIDGET_LOCAL void insert_protected_key(t3_widget::key_t key);

enum {
	CLASS_WHITESPACE,
	CLASS_ALNUM,
	CLASS_GRAPH,
	CLASS_OTHER
};

/** Get the character class associated with the character at a specific position in a string. */
T3_WIDGET_LOCAL int get_class(const std::string *str, int pos);
}; // namespace
#endif
