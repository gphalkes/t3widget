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
#include "widgets/frame.h"
#include "dialogs/attributepickerdialog.h"

#define ATTRIBUTE_PICKER_DIALOG_HEIGHT 10
#define ATTRIBUTE_PICKER_DIALOG_WIDTH 40

namespace t3_widget {

attribute_picker_dialog_t::attribute_picker_dialog_t(const char *_title) :
	dialog_t(ATTRIBUTE_PICKER_DIALOG_HEIGHT, ATTRIBUTE_PICKER_DIALOG_WIDTH, _title),
	impl(new implementation_t())
{
	smart_label_t *underline_label, *bold_label, *dim_label, *reverse_label, *blink_label;
	frame_t *test_line_frame;

	impl->underline_box = new checkbox_t(false);
	impl->underline_box->set_position(1, 2);
	underline_label = new smart_label_t("_Underline");
	underline_label->set_anchor(impl->underline_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	underline_label->set_position(0, 1);
	impl->underline_box->connect_move_focus_up(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_previous));
	impl->underline_box->connect_move_focus_down(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_next));
	impl->underline_box->set_label(underline_label);
	impl->underline_box->connect_toggled(sigc::mem_fun(this, &attribute_picker_dialog_t::attribute_changed));
	impl->underline_box->connect_activate(sigc::mem_fun0(this, &attribute_picker_dialog_t::ok_activate));


	impl->bold_box = new checkbox_t(false);
	impl->bold_box->set_anchor(impl->underline_box, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	impl->bold_box->set_position(1, 0);
	bold_label = new smart_label_t("_Bold");
	bold_label->set_anchor(impl->bold_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	bold_label->set_position(0, 1);
	impl->bold_box->connect_move_focus_up(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_previous));
	impl->bold_box->connect_move_focus_down(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_next));
	impl->bold_box->connect_toggled(sigc::mem_fun(this, &attribute_picker_dialog_t::attribute_changed));
	impl->bold_box->connect_activate(sigc::mem_fun0(this, &attribute_picker_dialog_t::ok_activate));


	impl->dim_box = new checkbox_t(false);
	impl->dim_box->set_anchor(impl->bold_box, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	impl->dim_box->set_position(1, 0);
	dim_label = new smart_label_t("_Dim");
	dim_label->set_anchor(impl->dim_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	dim_label->set_position(0, 1);
	impl->dim_box->connect_move_focus_up(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_previous));
	impl->dim_box->connect_move_focus_down(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_next));
	impl->dim_box->connect_toggled(sigc::mem_fun(this, &attribute_picker_dialog_t::attribute_changed));
	impl->dim_box->connect_activate(sigc::mem_fun0(this, &attribute_picker_dialog_t::ok_activate));


	impl->reverse_box = new checkbox_t(false);
	impl->reverse_box->set_anchor(impl->dim_box, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	impl->reverse_box->set_position(1, 0);
	reverse_label = new smart_label_t("_Reverse video");
	reverse_label->set_anchor(impl->reverse_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	reverse_label->set_position(0, 1);
	impl->reverse_box->connect_move_focus_up(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_previous));
	impl->reverse_box->connect_move_focus_down(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_next));
	impl->reverse_box->connect_toggled(sigc::mem_fun(this, &attribute_picker_dialog_t::attribute_changed));
	impl->reverse_box->connect_activate(sigc::mem_fun0(this, &attribute_picker_dialog_t::ok_activate));


	impl->blink_box = new checkbox_t(false);
	impl->blink_box->set_anchor(impl->reverse_box, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	impl->blink_box->set_position(1, 0);
	blink_label = new smart_label_t("Bl_ink");
	blink_label->set_anchor(impl->blink_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	blink_label->set_position(0, 1);
	impl->blink_box->connect_move_focus_up(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_previous));
	impl->blink_box->connect_move_focus_down(sigc::mem_fun(this, &attribute_picker_dialog_t::focus_next));
	impl->blink_box->connect_toggled(sigc::mem_fun(this, &attribute_picker_dialog_t::attribute_changed));
	impl->blink_box->connect_activate(sigc::mem_fun0(this, &attribute_picker_dialog_t::ok_activate));

	//FIXME: need to make the text and size configurable etc.
	test_line_frame = new frame_t();
	test_line_frame->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	test_line_frame->set_position(1, -2);
	impl->test_line = new test_line_t(4, "Text");
	test_line_frame->set_size(3, 6);
	test_line_frame->set_child(impl->test_line);

	push_back(impl->underline_box);
	push_back(underline_label);
	push_back(impl->bold_box);
	push_back(bold_label);
	push_back(impl->dim_box);
	push_back(dim_label);
	push_back(impl->reverse_box);
	push_back(reverse_label);
	push_back(impl->blink_box);
	push_back(blink_label);
	push_back(test_line_frame);
}

void attribute_picker_dialog_t::attribute_changed(void) {
	impl->test_line->set_attribute(get_attribute());
}

void attribute_picker_dialog_t::ok_activate(void) {
	attribute_selected(get_attribute());
}

t3_attr_t attribute_picker_dialog_t::get_attribute(void) {
	t3_attr_t result = 0;
	if (impl->underline_box->get_state())
		result |= T3_ATTR_UNDERLINE;
	if (impl->bold_box->get_state())
		result |= T3_ATTR_BOLD;
	if (impl->blink_box->get_state())
		result |= T3_ATTR_BLINK;
	if (impl->reverse_box->get_state())
		result |= T3_ATTR_REVERSE;
	return result;
}

void attribute_picker_dialog_t::set_attribute(t3_attr_t attr) {
	impl->underline_box->set_state(attr & T3_ATTR_UNDERLINE);
	impl->bold_box->set_state(attr & T3_ATTR_BOLD);
	impl->blink_box->set_state(attr & T3_ATTR_BLINK);
	impl->reverse_box->set_state(attr & T3_ATTR_REVERSE);
}


//================================================================================
attribute_picker_dialog_t::test_line_t::test_line_t(int width, const char *_text) : widget_t(1, width), text(_text) {}

bool attribute_picker_dialog_t::test_line_t::process_key(key_t key) {
	(void) key;
	return false;
}

bool attribute_picker_dialog_t::test_line_t::set_size(optint height, optint width) {
	(void) height;
	if (width.is_valid())
		return t3_win_resize(window, 1, width);
	return true;
}

void attribute_picker_dialog_t::test_line_t::update_contents(void) {
	if (!redraw)
		return;
	t3_win_set_default_attrs(window, attr);
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtoeol(window);
	t3_win_set_paint(window, 0, 0);
	t3_win_addstr(window, text.c_str(), 0);
}

bool attribute_picker_dialog_t::test_line_t::accepts_focus(void) { return false; }

void attribute_picker_dialog_t::test_line_t::set_attribute(t3_attr_t _attr) {
	redraw = true;
	attr = _attr;
}

}; // namespace
