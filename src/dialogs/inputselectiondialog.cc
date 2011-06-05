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
#include <cstring>
#include <unicode.h>
#include "dialogs/inputselectiondialog.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

input_selection_dialog_t::input_selection_dialog_t(int height, int width) :
		dialog_t(height, width, _("Input Method"))
{
	text_buffer_t *text = new text_buffer_t();

	text->append_line(
		_("This program has two ways of capturing key-presses. The first method is a "
		"conservative method which allows users to simulate the Meta-<letter> "
		"combinations by first pressing the Escape key, followed by the letter. However, "
		"this method also requires users to press the Escape key twice to close dialogs "
		"and can not reliably handle new or unknown keys.\n\n"));
	text->append_line(
		_("The second (time-out) method can handle new or unknown keys, but requires that "
		"pressing Meta-<letter> generates two characters: i.e. Escape and the letter. "
		"Furthermore, some terminals use the Meta-<letter> combination for their own "
		"purposes (e.g. gnome-terminal). If your terminal provides the required functionality, "
		"this method provides the most intuitive experience.\n\n"));
	text->append_line(
		_("The box below will show the Meta-<letter> combinations pressed while this "
		"dialog is active, to allow you to test the capabilities of your terminal. Note "
		"that for some specific terminals there are other work-arounds as well, which "
		"are listed below this point.\n\n"));
	text->append_line(
		_("== gnome-terminal ==\n\n"));
	text->append_line(
		_("Pressing Meta-Shift-<letter> will not be intercepted by gnome-terminal, and "
		"will be accepted as Meta-<letter> by this program. As an alternative, "
		"gnome-terminal can be instructed not to intercept Meta-<letter> combinations by "
		"unchecking \"Enable menu access keys\" in \"Edit->Keyboard Shortcuts\"."));

	text_window = new text_window_t(text);
	text_frame = new frame_t(frame_t::COVER_RIGHT);
	text_frame->set_child(text_window);
	text_frame->set_size(height - 6, width - 2);
	text_frame->set_position(1, 1);

	label_frame = new frame_t();
	label_frame->set_anchor(text_frame, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_TOPCENTER));
	label_frame->set_position(0, 0);
	label_frame->set_size(3, 12);

	key_label = new label_t("");
	key_label->set_accepts_focus(false);
	label_frame->set_child(key_label);

	conservative_button = new button_t(_("Conservative (F2)"), true);
	conservative_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMCENTER));
	conservative_button->set_position(-1, -conservative_button->get_width() / 2 - 1);
	conservative_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::close));
	conservative_button->connect_activate(conservative_activated.make_slot());
	timeout_button = new button_t(_("Time-out based (F3)"));
	timeout_button->set_anchor(conservative_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	timeout_button->set_position(0, 2);
	timeout_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::close));
	timeout_button->connect_activate(timeout_activated.make_slot());

	push_back(text_frame);
	push_back(label_frame);
	push_back(conservative_button);
	push_back(timeout_button);
}

bool input_selection_dialog_t::set_size(optint height, optint width) {
	bool result;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result = dialog_t::set_size(height, width);
	result &= text_frame->set_size(height - 6, width - 2);
	return result;
}

bool input_selection_dialog_t::process_key(key_t key) {
	if ((key & ~EKEY_META) < 0x110000 && (key & ~EKEY_META) > 0x20) {
		char buffer[100];
		size_t buffer_contents_length = 0;
		buffer[0] = 0;
		if (key & EKEY_META)
			strcat(buffer, _("Meta-"));
		buffer_contents_length = strlen(buffer);
		buffer_contents_length += t3_unicode_put(key & ~EKEY_META, buffer + buffer_contents_length);
		buffer[buffer_contents_length] = 0;
		key_label->set_text(buffer);
		return true;
	}
	switch (key) {
		case EKEY_ESC:
			return true;
		case EKEY_F2:
			close();
			conservative_activated();
			return true;
		case EKEY_F3:
			close();
			timeout_activated();
			return true;
		default:
			return dialog_t::process_key(key);
	}
}


}; // namespace
