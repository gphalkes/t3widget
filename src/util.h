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
#ifndef T3_WIDGET_UTIL_H
#define T3_WIDGET_UTIL_H
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <sigc++/sigc++.h>
#include <t3window/window.h>

#include <t3widget/widget_api.h>
#include <t3widget/ptr.h>

namespace t3_widget {

/** Class defining values with a separate validity check. */
template <class T>
class T3_WIDGET_API optional {
	private:
		T value; /**< Value, if #initialized is @c true. */
		bool initialized; /**< Boolean indicating whether #value has been initialized. */

	public:
		optional(void) : initialized(false) {}
		optional(T _value) : value(_value), initialized(true) {}
		bool is_valid(void) const { return initialized; }
		void unset(void) { initialized = false; }
		operator T (void) const { if (!initialized) throw(0); return (T) value; }
		T operator()(void) const  { if (!initialized) throw(0); return (T) value; }
		optional & operator=(const optional &other) { initialized = other.initialized; value = other.value; return *this; }
		optional & operator=(const T other) { initialized = true; value = other; return *this; }
};

typedef optional<int> optint;
/** Standard uninitialized @ref optint value. */
T3_WIDGET_API extern const optint None;

struct T3_WIDGET_API text_coordinate_t {
	text_coordinate_t(void) {}
	text_coordinate_t(int _line, int _pos) : line(_line), pos(_pos) {}
	bool operator==(const text_coordinate_t &other) const { return line == other.line && pos == other.pos; }
	bool operator!=(const text_coordinate_t &other) const { return line != other.line || pos != other.pos; }
	bool operator>(const text_coordinate_t &other) const { return line > other.line || (line == other.line && pos > other.pos); }
	bool operator>=(const text_coordinate_t &other) const { return line > other.line || (line == other.line && pos >= other.pos); }
	bool operator<(const text_coordinate_t &other) const { return line < other.line || (line == other.line && pos < other.pos); }
	bool operator<=(const text_coordinate_t &other) const { return line < other.line || (line == other.line && pos <= other.pos); }
	int line;
	int pos;
};

#define T3_WIDGET_SIGNAL(_name, ...) \
protected: \
	sigc::signal<__VA_ARGS__> _name; \
public: \
	sigc::connection connect_##_name(const sigc::slot<__VA_ARGS__> &_slot) { return _name.connect(_slot); }

#define _T3_WIDGET_ENUM(_name, ...) \
class T3_WIDGET_API _name { \
	public: \
		enum _values { \
			__VA_ARGS__ \
		}; \
		_name(void) {} \
		_name(_values _value_arg) : _value(_value_arg) {} \
		_values operator =(_values _value_arg) { _value = _value_arg; return _value; } \
		operator int (void) const { return (int) _value; } \
		bool operator == (_values _value_arg) const { return _value == _value_arg; } \
	private: \
		_values _value; \
}

_T3_WIDGET_ENUM(selection_mode_t,
	NONE,
	SHIFT,
	MARK,
	ALL
);

_T3_WIDGET_ENUM(find_flags_t,
	BACKWARD = (1<<0),
	ICASE = (1<<1),
	REGEX = (1<<2),
	WRAP = (1<<3),
	TRANSFROM_BACKSLASH = (1<<4),
	WHOLE_WORD = (1<<5) | (1<<6),
	ANCHOR_WORD_LEFT = (1<<5),
	ANCHOR_WORD_RIGHT = (1<<6),
	VALID = (1<<7),
	REPLACEMENT_VALID = (1<<8),
);

_T3_WIDGET_ENUM(find_action_t,
	FIND,
	SKIP,
	REPLACE,
	REPLACE_ALL,
	REPLACE_IN_SELECTION
);

/** Constants for indicating which attribute to change/retrieve. */
_T3_WIDGET_ENUM(attribute_t,
	NON_PRINT,
	SELECTION_CURSOR,
	SELECTION_CURSOR2,
	BAD_DRAW,
	TEXT_CURSOR,
	TEXT,
	TEXT_SELECTED,
	HIGHLIGHT,
	HIGHLIGHT_SELECTED,
	DIALOG,
	DIALOG_SELECTED,
	BUTTON,
	BUTTON_SELECTED,
	SCROLLBAR,
	MENUBAR,
	MENUBAR_SELECTED,
	BACKGROUND,
	SHADOW,
	META_TEXT
);
/** @var attribute_t::NON_PRINT
    Attribute specifier for non-printable characters. */
/** @var attribute_t::SELECTION_CURSOR
    Attribute specifier for cursor when selecting text. */
/** @var attribute_t::SELECTION_CURSOR2
    Attribute specifier for cursor when selecting text in reverse direction. */
/** @var attribute_t::BAD_DRAW
    Attribute specifier for text which the terminal is not able to draw correctly. */
//FIXME: list other attributes

_T3_WIDGET_ENUM(rewrap_type_t,
	REWRAP_ALL,
	REWRAP_LINE,
	REWRAP_LINE_LOCAL,
	INSERT_LINES,
	DELETE_LINES
);

_T3_WIDGET_ENUM(wrap_type_t,
	NONE,
	WORD,
	CHARACTER
);

#undef _T3_WIDGET_ENUM


typedef cleanup_func_ptr<t3_window_t, t3_win_del>::t cleanup_t3_window_ptr;


T3_WIDGET_API ssize_t nosig_write(int fd, const char *buffer, size_t bytes);
T3_WIDGET_API ssize_t nosig_read(int fd, char *buffer, size_t bytes);

T3_WIDGET_API std::string get_working_directory(void);
T3_WIDGET_API std::string get_directory(const char *directory);
T3_WIDGET_API bool is_dir(const std::string *current_dir, const char *name);

T3_WIDGET_API void convert_lang_codeset(const char *str, size_t len, std::string *result, bool from);
T3_WIDGET_API void convert_lang_codeset(const char *str, std::string *result, bool from);
T3_WIDGET_API void convert_lang_codeset(const std::string *str, std::string *result, bool from);

}; // namespace
#endif
