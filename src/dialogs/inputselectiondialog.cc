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
#include <t3unicode/unicode.h>
#include "dialogs/inputselectiondialog.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

input_selection_dialog_t::input_selection_dialog_t(int height, int width, text_buffer_t *_text) :
		dialog_t(height, width, _("Input Method"))
{
	text = _text == NULL ? get_default_text() : _text;

	text_window = new text_window_t(text);
	text_frame = new frame_t(frame_t::COVER_RIGHT);
	text_frame->set_child(text_window);
	text_frame->set_size(height - 7, width - 2);
	text_frame->set_position(1, 1);

	label_frame = new frame_t();
	label_frame->set_anchor(text_frame, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_TOPCENTER));
	label_frame->set_position(0, 0);
	label_frame->set_size(3, 12);

	key_label = new label_t("");
	key_label->set_accepts_focus(false);
	key_label->set_align(label_t::ALIGN_CENTER);
	label_frame->set_child(key_label);

	intuitive_button = new button_t(_("Most intuitive (F2)"));
	intuitive_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMCENTER));
	intuitive_button->set_position(-2, -intuitive_button->get_width() / 2 - 1);
	intuitive_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::close));
	intuitive_button->connect_activate(sigc::bind(sigc::ptr_fun(set_key_timeout), 100));
	intuitive_button->connect_activate(intuitive_activated.make_slot());
	compromise_button = new button_t(_("Compromise (F3)"));
	compromise_button->set_anchor(intuitive_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	compromise_button->set_position(0, 2);
	compromise_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::close));
	compromise_button->connect_activate(sigc::bind(sigc::ptr_fun(set_key_timeout), -1000));
	compromise_button->connect_activate(compromise_activated.make_slot());
	no_timeout_button = new button_t(_("No time-out (F4)"));
	no_timeout_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_BOTTOMCENTER));
	no_timeout_button->set_position(-1, 0);
	no_timeout_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::close));
	no_timeout_button->connect_activate(sigc::bind(sigc::ptr_fun(set_key_timeout), 0));
	no_timeout_button->connect_activate(no_timeout_activated.make_slot());

	push_back(text_frame);
	push_back(label_frame);
	push_back(intuitive_button);
	push_back(compromise_button);
	push_back(no_timeout_button);
}

input_selection_dialog_t::~input_selection_dialog_t(void) {
	delete text;
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
		case EKEY_ESC | EKEY_META:
			return true;
		case EKEY_F2:
		case EKEY_F2 | EKEY_META:
			close();
			set_key_timeout(100);
			intuitive_activated();
			return true;
		case EKEY_F3:
		case EKEY_F3 | EKEY_META:
			close();
			set_key_timeout(-1000);
			compromise_activated();
			return true;
		case EKEY_F4:
		case EKEY_F4 | EKEY_META:
			close();
			set_key_timeout(0);
			no_timeout_activated();
			return true;
		case '\t' | EKEY_META:
		case EKEY_RIGHT | EKEY_META:
		case EKEY_LEFT | EKEY_META:
			return dialog_t::process_key(key & ~EKEY_META);
		default:
			return dialog_t::process_key(key);
	}
}

text_buffer_t *input_selection_dialog_t::get_default_text(void) {
	text_buffer_t *default_text = new text_buffer_t();

	default_text->append_text(_("This program tries to provide an intuitive interface. For example, it allows "
		"you to use Meta-<letter> combinations to open menus and jump to items on your "
		"screen. However, not all terminals and terminal emulators handle the Meta key "
		"the same way. The result is that this program can not reliably handle the "
		"Meta-<letter> combinations on all terminals. While this dialog is open, the box "
		"below will show which keys you pressed, allowing you to test whether the Meta "
		"key is fully functional.\n\n"));
	default_text->append_text(_("If pressing Meta-<letter> combination does not result in the box below "
		"displaying the combination, you can simulate the Meta-<letter> combinations by "
		"pressing Escape followed by <letter>. Furthermore, for some terminals (e.g. "
		"gnome-terminal), pressing Meta-Shift-<letter> will result in the combination "
		"being displayed in the box below.\n\n"));
	default_text->append_text(_("After you have verified whether pressing Meta-<letter> works as expected, you "
		"can select the best interaction method given your terminal's capabilities. "
		"Selecting \"Most intuitive\" will allow you to use a single press of the Escape "
		"button to close dialogs, but requires that Meta-<letter> combinations work.\n\n"));
	default_text->append_text(_("Selecting \"Compromise\" or \"No time-out\" will require you to press the Escape "
		"key twice to close a dialog. When \"Compromise\" is selected you have one second "
		"to press the <letter> key to simulate the Meta-<letter> combination. After this "
		"second, the fact that you pressed the Escape key will be ignored. When \"No time-out\" "
		"is selected, the program will wait indefinitely for a <letter> (or "
		"other key) after you press Escape."));

	return default_text;
}

}; // namespace
