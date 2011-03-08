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
#ifndef T3_WIDGET_UTIL_H
#define T3_WIDGET_UTIL_H
#include <unistd.h>
#include <sigc++/sigc++.h>

namespace t3_widget {

/* Class defining integers with a separate validity check. */
class optint {
	private:
		int value;
		bool initialized;

	public:
		optint(void) : value(0), initialized(false) {}
		optint(int _value) : value(_value), initialized(true) {}
		bool is_valid(void) { return initialized; }
		operator int (void) const { if (!initialized) throw(0); return (int) value; }
		optint & operator=(const optint &other) { initialized = other.initialized; value = other.value; return *this; }
		optint & operator=(const int other) { initialized = true; value = other; return *this; }
};
extern const optint None;


#define ENUM(_name, ...) \
class _name { \
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

ENUM(selection_mode_t,
	NONE,
	SHIFT,
	MARK,
	ALL
);

ENUM(find_flags_t,
	BACKWARD = (1<<0),
	NEXT = (1<<1),
	ICASE = (1<<2),
	REGEX = (1<<3),
	WRAP = (1<<4),
	TRANSFROM_BACKSLASH = (1<<5),
	WHOLE_WORD = (1<<6),
	VALID = (1<<7)
);

#define SIGNAL(_name, ...) \
protected: \
	sigc::signal<__VA_ARGS__> _name; \
public: \
	sigc::connection connect_##_name(const sigc::slot<__VA_ARGS__> &_slot) { return _name.connect(_slot); }

ssize_t nosig_write(int fd, const char *buffer, size_t bytes);
ssize_t nosig_read(int fd, char *buffer, size_t bytes);

struct text_coordinate_t {
	text_coordinate_t(void) {}
	text_coordinate_t(int _line, int _pos) : line(_line), pos(_pos) {}
	int line;
	int pos;
};

}; // namespace
#endif