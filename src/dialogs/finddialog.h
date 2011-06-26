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

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/textfield.h>
#include <t3widget/widgets/checkbox.h>
#include <t3widget/widgets/button.h>
#include <t3widget/util.h>

namespace t3_widget {

class replace_buttons_dialog_t;
class finder_t;

class T3_WIDGET_API find_dialog_t : public dialog_t {
	protected:
		friend class replace_buttons_dialog_t;

		smart_label_t *replace_label;
		text_field_t *find_line, *replace_line;
		checkbox_t *whole_word_checkbox,
			*match_case_checkbox,
			*regex_checkbox,
			*wrap_checkbox,
			*transform_backslash_checkbox,
			*reverse_direction_checkbox;
		button_t *in_selection_button, *replace_all_button;
		sigc::connection find_button_up_connection;
		int state; // State of all the checkboxes converted to FIND_* flags

		void backward_toggled(void);
		void icase_toggled(void);
		void regex_toggled(void);
		void wrap_toggled(void);
		void transform_backslash_toggled(void);
		void whole_word_toggled(void);
		void find_activated(void);
		void find_activated(find_action_t);

	public:
		find_dialog_t(int _state = find_flags_t::ICASE | find_flags_t::WRAP);
		virtual bool set_size(optint height, optint width);
		virtual void set_text(const std::string *str);
		virtual void set_replace(bool _replace);
		virtual void set_state(int _state);

	T3_WIDGET_SIGNAL(activate, void, find_action_t, finder_t *);
};

class T3_WIDGET_API replace_buttons_dialog_t : public dialog_t {
	protected:
		button_t *find_button, *replace_button;

	public:
		replace_buttons_dialog_t(void);
		virtual void reshow(find_action_t button);

	T3_WIDGET_SIGNAL(activate, void, find_action_t);
};

}; // namespace
#endif
