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
#include <t3widget/widgets/expandergroup.h>

namespace t3_widget {

class T3_WIDGET_API attribute_picker_dialog_t : public dialog_t {
	private:
		class T3_WIDGET_LOCAL test_line_t;
		class T3_WIDGET_LOCAL color_picker_t;

		struct T3_WIDGET_LOCAL implementation_t {
			checkbox_t *bold_box, *reverse_box, *blink_box, *underline_box, *dim_box;
			test_line_t *test_line;
			color_picker_t *fg_picker, *bg_picker;
			expander_group_t *expander_group;
			t3_attr_t base_attributes;
			implementation_t(void) : base_attributes(0) {}
		};
		pimpl_ptr<implementation_t>::t impl;

		void attribute_changed(void);
		void ok_activate(void);
		void group_expanded(bool state);
		t3_attr_t get_attribute(void);

	public:
		attribute_picker_dialog_t(const char *_title = "Attribute");
		void set_attribute(t3_attr_t attr);
		/** Set the base attributes for the attribute picker.
		    @param attr The base attributes to use

		    When selecting attributes, sometimes the result will be combined with
		    another set of attributes. To show the user what the effect of choosing
		    the a set of attributes is, you can set the base attributes with this function.
		*/
		void set_base_attributes(t3_attr_t attr);

	T3_WIDGET_SIGNAL(attribute_selected, void, t3_attr_t);

};

class T3_WIDGET_LOCAL attribute_picker_dialog_t::test_line_t : public widget_t {
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

class T3_WIDGET_LOCAL attribute_picker_dialog_t::color_picker_t : public widget_t {
	private:
		int max_color, current_color;
		bool fg, has_focus;
		t3_attr_t undefined_colors;

		int xy_to_color(int x, int y);
	public:
		color_picker_t(bool _fg);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual bool process_mouse_event(mouse_event_t event);
		virtual void set_focus(focus_t focus);

		/** Change the rendering of the default colors.
		    @param attr The colors to use for the default colors.

		    When selecting colors, sometimes the result will be combined with
		    another set of colors. The undefined color will then be overriden with
		    the color to combine with. To show the user what the effect of choosing
		    the undefined color is, you can set the colors to use for the undefined
		    colors with this function.
		*/
		void set_undefined_colors(t3_attr_t attr);
		t3_attr_t get_color(void);
		void set_color(t3_attr_t attr);

	T3_WIDGET_SIGNAL(activated, void);
	T3_WIDGET_SIGNAL(selection_changed, void);
};

}; // namespace
#endif
