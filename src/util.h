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
#ifndef UTIL_H
#define UTIL_H
#include <sigc++/sigc++.h>

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

ENUM(SelectionMode,
	NONE,
	SHIFT,
	MARK,
	ALL
);

#define SIGNAL(_name, ...) \
protected: \
	sigc::signal<__VA_ARGS__> _name; \
public: \
	sigc::connection connect_##_name(const sigc::slot<__VA_ARGS__> &_slot) { return _name.connect(_slot); }


#endif