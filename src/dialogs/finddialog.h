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
#ifndef T3_WIDGET_FINDDIALOG_H
#define T3_WIDGET_FINDDIALOG_H

#include "dialogs/dialogs.h"

namespace t3_widget {

class replace_buttons_dialog_t;

class find_dialog_t : public dialog_t {
	private:
		text_field_t *find_line, *replace_line;
		checkbox_t *whole_word_checkbox,
			*match_case_checkbox,
			*regex_checkbox,
			*wrap_checkbox,
			*transform_backslash_checkbox,
			*reverse_direction_checkbox;
		int state; // State of all the checkboxes converted to FIND_* flags
		bool replace;

		enum { REPLACE = DIALOG_ACTION_LAST, REPLACE_ALL, IN_SELECTION, ACTION_LAST };

		friend class replace_buttons_dialog_t;

	public:
		find_dialog_t(bool _replace = false);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void callback(int action, const void *data = NULL);
};

class replace_buttons_dialog_t : public dialog_t {
	public:
		replace_buttons_dialog_t(void);

		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void callback(int action, const void *data = NULL);
		virtual void set_show(bool show);
};

}; // namespace
#endif
