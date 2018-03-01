/* Copyright (C) 2011-2012,2018 G.P. Halkes
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

#include <algorithm>
#include <memory>
#include <string>
#include <t3widget/key.h>

#ifdef HAS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "log.h"
#include "main.h"
#include "signals.h"
#include "string_view.h"
#include "util.h"
#include "widget_api.h"

namespace t3_widget {

T3_WIDGET_LOCAL extern init_parameters_t *init_params;
T3_WIDGET_LOCAL extern bool disable_primary_selection;

T3_WIDGET_LOCAL void stop_clipboard();

#ifdef _T3_WIDGET_DEBUG
#define ASSERT(_x)                                                                            \
  do {                                                                                        \
    if (!(_x)) {                                                                              \
      fprintf(stderr, "%s:%d: libt3widget: Assertion failed: %s\n", __FILE__, __LINE__, #_x); \
      abort();                                                                                \
    }                                                                                         \
  } while (false)
#else
#define ASSERT(_x)
#endif

#define ARRAY_SIZE(_x) ((int)(sizeof(_x) / sizeof(_x[0])))

/** Mask for return values of #parse_escape, indicating that the escape value was a Unicode
 * character. */
#define ESCAPE_UNICODE (1 << 29)
/** Mask for return values of #parse_escape, indicating that the escape value was a replacement
 * marker. */
#define ESCAPE_REPLACEMENT (1 << 30)

// FIXME: do proper gettext stuff instead of this temporary wrapper
#define _(_x) _x

/** Parse a single escape sequence in a string.
    @param str The string to parse the escape from.
    @param error_message Pointer to storage for an error message.
    @param read_position The index in @p str to start reading, updated by parse_escape.
        @param replacements A boolean indicating whether replacement markers should be parsed as
   such.
*/
T3_WIDGET_LOCAL int parse_escape(const std::string &str, const char **error_message,
                                 size_t &read_position, bool replacements = false);
/** Convert escapes in a string to associated values.
    @param str The string to parse, updated by this function.
    @param error_message Pointer to storage for an error message.
        @param replacements A boolean indicating whether replacement markers should be parsed as
   such.
*/
T3_WIDGET_LOCAL bool parse_escapes(std::string &str, const char **error_message,
                                   bool replacements = false);

/* Key handling routines. */
class complex_error_t;
/** Initialize the key handling code. */
T3_WIDGET_LOCAL complex_error_t init_keys(const optional<std::string> &term, bool separate_keypad);
/** Clean-up any data allocated for the key handling code. */
T3_WIDGET_LOCAL void cleanup_keys();
/** Switch to the default keypad mode to allow other applications to function. */
T3_WIDGET_LOCAL void deinit_keys();
/** Switch back to best keypad mode after using #deinit_keys. */
T3_WIDGET_LOCAL void reinit_keys();
/** Insert a key to the queue, marked to ensure it is not interpreted by any widget except text
 * widgets. */
T3_WIDGET_LOCAL void insert_protected_key(t3_widget::key_t key);
/** Read chars into buffer for processing. */
T3_WIDGET_LOCAL bool read_keychar(int timeout);

/* char_buffer for key and mouse handling. Has to be shared between key.cc and
   mouse.cc because of XTerm in-band mouse reporting. */
extern char char_buffer[128];
extern int char_buffer_fill;

/** Initialize the mouse handling code. */
T3_WIDGET_LOCAL void init_mouse_reporting(bool xterm_mouse);
/** Switch off mouse reporting to allow other applications to function. */
T3_WIDGET_LOCAL void deinit_mouse_reporting();
/** Switch back to mouse reporting after using #deinit_mouse_reporting. */
T3_WIDGET_LOCAL void reinit_mouse_reporting();
/** Stop mouse reporting all together before program termination. */
T3_WIDGET_LOCAL void stop_mouse_reporting();
/** Decode an xterm mouse event. */
T3_WIDGET_LOCAL bool decode_xterm_mouse();
/** Decode an xterm mouse event using the SGR or URXVT protocols. */
T3_WIDGET_LOCAL bool decode_xterm_mouse_sgr_urxvt(const char *data, size_t len);
/** Report whether XTerm mouse reporting is active. */
T3_WIDGET_LOCAL bool use_xterm_mouse_reporting();
/** Set bit(s) for mouse event fd. */
T3_WIDGET_LOCAL void fd_set_mouse_fd(fd_set *readset, int *max_fd);
/** Check the mouse event fd for events, if appropriate bits in @p readset indicate available data.
 */
T3_WIDGET_LOCAL bool check_mouse_fd(fd_set *readset);

enum { CLASS_WHITESPACE, CLASS_ALNUM, CLASS_GRAPH, CLASS_OTHER };

/** Get the character class associated with the character at a specific position in a string. */
T3_WIDGET_LOCAL int get_class(const std::string &str, int pos);

template <typename C>
void remove_element(C &container, typename C::value_type value) {
  container.erase(std::remove(container.begin(), container.end(), value), container.end());
}

// FIXME: it would be good if this could work for the non-const case as well.
template <typename T>
class const_reverse_view_object {
 public:
  const_reverse_view_object(T &t) : t_(t) {}

  using const_iterator = typename T::const_reverse_iterator;

  const_iterator begin() const { return t_.rbegin(); }
  const_iterator end() const { return t_.rend(); }

 protected:
  T &t_;
};

template <typename T>
class mutable_reverse_view_object : public const_reverse_view_object<T> {
 public:
  using const_reverse_view_object<T>::const_reverse_view_object;

  using iterator = typename T::reverse_iterator;

  iterator begin() { return this->t_.rbegin(); }
  iterator end() { return this->t_.rend(); }
};

template <typename T>
typename std::enable_if<std::is_const<T>::value, const_reverse_view_object<T>>::type reverse_view(
    T &t) {
  return t;
}

template <typename T>
typename std::enable_if<!std::is_const<T>::value, mutable_reverse_view_object<T>>::type
reverse_view(T &t) {
  return t;
}

T3_WIDGET_LOCAL bool starts_with(const std::string &str, const std::string &with);

}  // namespace
#endif
