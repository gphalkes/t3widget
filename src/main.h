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
#ifndef T3_WIDGET_MAIN_H
#define T3_WIDGET_MAIN_H

#include "util.h"
#include "dialogs/dialog.h"
#include "dialogs/insertchardialog.h"
#include "dialogs/messagedialog.h"

namespace t3_widget {

/** The version of libt3widget encoded as a single integer.

    The least significant 8 bits represent the patch level.
    The second 8 bits represent the minor version.
    The third 8 bits represent the major version.

	At runtime, the value of T3_WIDGET_VERSION can be retrieved by calling
	t3_widget::get_version.

    @internal
    The value 0 is an invalid value which should be replaced by the script
    that builds the release package.
*/
#define T3_WIDGET_VERSION 0

class main_window_base_t;

class complex_error_t {
	public:
		enum source_t{
			SRC_NONE,
			SRC_ERRNO,
			SRC_TRANSCRIPT,
			SRC_T3_KEY,
			SRC_T3_WINDOW
		};
	private:
		bool success;
		source_t source;
		int error;

	public:
		complex_error_t(void);
		complex_error_t(source_t _source, int _error);
		void set_error(source_t _source, int _error);
		bool get_success(void);
		source_t get_source(void);
		int get_error(void);
		const char *get_string(void);
};

//FIXME: shouldn't these be internal?
extern insert_char_dialog_t *insert_char_dialog;
extern message_dialog_t *message_dialog;

sigc::connection connect_resize(const sigc::slot<void, int, int> &slot);
sigc::connection connect_update_notification(const sigc::slot<void> &slot);
sigc::connection connect_on_init(const sigc::slot<void> &slot);
sigc::connection connect_terminal_settings_changed(const sigc::slot<void> &slot);

complex_error_t init(const char *term = NULL, bool separate_keypad = false);
void restore(void);
void iterate(void);
void main_loop(void);
void suspend(void);
void redraw(void);

void set_color_mode(bool on);
void set_attribute(attribute_t attribute, t3_attr_t value);
long get_version(void);
}; // namespace

#endif
