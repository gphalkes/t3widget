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

#include <string>

#include "dialogs/dialog.h"
#include "widgets/textfield.h"
#include "widgets/checkbox.h"
#include "findcontext.h"
#include "util.h"

namespace t3_widget {

class replace_buttons_dialog_t;

class find_dialog_t : public dialog_t {
	protected:
		friend class replace_buttons_dialog_t;

		text_field_t *find_line, *replace_line;
		checkbox_t *whole_word_checkbox,
			*match_case_checkbox,
			*regex_checkbox,
			*wrap_checkbox,
			*transform_backslash_checkbox,
			*reverse_direction_checkbox;
		int state; // State of all the checkboxes converted to FIND_* flags
		bool replace;

		void backward_toggled(void);
		void icase_toggled(void);
		void regex_toggled(void);
		void wrap_toggled(void);
		void transform_backslash_toggled(void);
		void whole_word_toggled(void);
		void find_activated(void);
		void find_activated(find_action_t);

	public:
		find_dialog_t(bool _replace = false);
		virtual bool set_size(optint height, optint width);
		virtual void set_text(const std::string *str);

	T3_WIDET_SIGNAL(activate, void, find_action_t, finder_t *);
};

class replace_buttons_dialog_t : public dialog_t {
	public:
		replace_buttons_dialog_t(void);

	T3_WIDET_SIGNAL(activate, void, find_action_t);
};

}; // namespace
#endif
