/* Copyright (C) 2012 G.P. Halkes
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
#ifndef T3_WIDGET_ATTRIBUTEPICKERDIALOG_H
#define T3_WIDGET_ATTRIBUTEPICKERDIALOG_H

#include <t3widget/interfaces.h>
#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/checkbox.h>

namespace t3_widget {

class T3_WIDGET_API attribute_picker_dialog_t : public dialog_t {
	private:
		class T3_WIDGET_LOCAL test_line_t;

		struct T3_WIDGET_LOCAL implementation_t {
			checkbox_t *bold_box, *reverse_box, *blink_box, *underline_box, *dim_box;
			test_line_t *test_line;
			/* implementation_t(void) : view(&names), option_widget_set(false) {}*/
		};
		pimpl_ptr<implementation_t>::t impl;

		void attribute_changed(void);
		void ok_activate(void);
		t3_attr_t get_attribute(void);

	public:
		attribute_picker_dialog_t(const char *_title = "Attribute");
		void set_attribute(t3_attr_t attr);

	T3_WIDGET_SIGNAL(attribute_selected, void, t3_attr_t);

};

class attribute_picker_dialog_t::test_line_t : public widget_t {
	private:
		std::string text;
		t3_attr_t attr;
	public:
		test_line_t(int width, const char *_text);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual bool accepts_focus(void);

		void set_attribute(t3_attr_t _attr);
};

}; // namespace
#endif
