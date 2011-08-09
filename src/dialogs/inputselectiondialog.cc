/* Copyright (C) 2011 G.P. Halkes
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
#include "widgets/button.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

input_selection_dialog_t::input_selection_dialog_t(int height, int width, text_buffer_t *_text) :
		dialog_t(height, width, _("Input Handling"))
{
	button_t *ok_button, *cancel_button;
	smart_label_t *enable_simulate_label, *disable_timeout_label, *close_remark_label;

	text = _text == NULL ? get_default_text() : _text;

	text_window = new text_window_t(text);
	text_frame = new frame_t(frame_t::COVER_RIGHT);
	text_frame->set_child(text_window);
	text_frame->set_size(height - 9, width - 2);
	text_frame->set_position(1, 1);

	label_frame = new frame_t();
	label_frame->set_anchor(text_frame, T3_PARENT(T3_ANCHOR_BOTTOMCENTER) | T3_CHILD(T3_ANCHOR_TOPCENTER));
	label_frame->set_position(0, 0);
	label_frame->set_size(3, 18);

	key_label = new label_t("");
	key_label->set_accepts_focus(false);
	key_label->set_align(label_t::ALIGN_CENTER);
	label_frame->set_child(key_label);

	enable_simulate_box = new checkbox_t();
	enable_simulate_box->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_PARENT(T3_ANCHOR_BOTTOMLEFT));
	enable_simulate_box->set_position(-5, 2);
	enable_simulate_box->connect_toggled(sigc::mem_fun(this, &input_selection_dialog_t::check_state));
	enable_simulate_box->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::ok_activated));
	enable_simulate_box->connect_move_focus_up(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));
	enable_simulate_box->connect_move_focus_down(sigc::mem_fun(this, &input_selection_dialog_t::focus_next));

	enable_simulate_label = new smart_label_t("'Escape <letter>' simulates Meta+<letter>");
	enable_simulate_label->set_anchor(enable_simulate_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	enable_simulate_label->set_position(0, 2);

	close_remark_label = new smart_label_t("(Requires 'Escape Escape' to close menu or dialog)");
	close_remark_label->set_anchor(enable_simulate_label, 0);
	close_remark_label->set_position(1, 0);

	disable_timeout_box = new checkbox_t();
	disable_timeout_box->set_anchor(enable_simulate_box, 0);
	disable_timeout_box->set_position(2, 0);
	disable_timeout_box->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::ok_activated));
	disable_timeout_box->connect_move_focus_up(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));
	disable_timeout_box->connect_move_focus_down(sigc::mem_fun(this, &input_selection_dialog_t::focus_next));

	disable_timeout_label = new smart_label_t("Disable timeout on Escape");
	disable_timeout_label->set_anchor(disable_timeout_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	disable_timeout_label->set_position(0, 2);

	cancel_button = new button_t(_("Cancel"));
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::cancel));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));

	ok_button = new button_t(_("Ok"), true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);
	ok_button->connect_activate(sigc::mem_fun(this, &input_selection_dialog_t::ok_activated));
	ok_button->connect_move_focus_right(sigc::mem_fun(this, &input_selection_dialog_t::focus_next));
	ok_button->connect_move_focus_up(sigc::mem_fun(this, &input_selection_dialog_t::focus_previous));

	push_back(text_frame);
	push_back(label_frame);
	push_back(enable_simulate_box);
	push_back(enable_simulate_label);
	push_back(close_remark_label);
	push_back(disable_timeout_box);
	push_back(disable_timeout_label);
	push_back(ok_button);
	push_back(cancel_button);
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
	switch (key) {
		case EKEY_ESC:
		case EKEY_ESC | EKEY_META:
			set_key_timeout(old_timeout);
			close();
			return true;
		case '\t' | EKEY_META:
		case EKEY_RIGHT | EKEY_META:
		case EKEY_LEFT | EKEY_META:
			return dialog_t::process_key(key & ~EKEY_META);
		default:
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
			if (!dialog_t::process_key(key))
				key_label->set_text("<other>");
			return true;
	}
}

void input_selection_dialog_t::show(void) {
	old_timeout = get_key_timeout();
	set_key_timeout(-1000);
	if (old_timeout <= 0) {
		enable_simulate_box->set_state(true);
		disable_timeout_box->set_state(old_timeout == 0);
		disable_timeout_box->set_enabled(true);
	} else {
		enable_simulate_box->set_state(false);
		disable_timeout_box->set_enabled(false);
	}
	dialog_t::show();
}

text_buffer_t *input_selection_dialog_t::get_default_text(void) {
	text_buffer_t *default_text = new text_buffer_t();

	default_text->append_text(_("This program tries to provide an intuitive interface for people accustomed "
		"to GUI applications. For example, it allows you to use Meta+<letter> combinations to open "
		"menus and jump to items on your screen. However, not all terminals and terminal emulators "
		"handle the Meta key the same way. The result is that this program can not reliably handle the "
		"Meta+<letter> combinations on all terminals. While this dialog is open, the box "
		"below will show which keys you pressed, allowing you to test whether the Meta "
		"key is fully functional.\n\n"));
	default_text->append_text(_("As an alternative to Meta+<letter>, this program can allow you to simulate "
		"Meta+<letter> by pressing Esacpe followed by <letter>. However, this does mean that you have to "
		"press Escape twice to close a menu or dialog. While this dialog is open, this work-around "
		"is enabled. If you do not require this work-around because Meta+<letter> is fully functional, "
		"you can disable it below for the rest of the program, allowing you to close menus and dialogs "
		"(except this one) with a single press of the Escape key.\n\n"));
	default_text->append_text(_("When the 'Escape <letter>' work-around is enabled, the fact that you "
		"pressed the Escape key is discarded after one second. This may be inconvenient in some cases, "
		"therefore the timeout on the Escape key can be disabled.\n\n"));
	default_text->append_text(_("Other methods\n=============\n\nSome terminal emulators have configuration "
		"options to either use Meta+<letter> for their own purposes, or pass the key combination through to the "
		"program running in the terminal. An example of this is gnome-terminal. Furthermore, most terminal "
		"emulators only intercept Meta+<letter> but not Meta+Shift+<letter>. This combination is therefore "
		"also accepted as if it were Meta+<letter>."));

	return default_text;
}

void input_selection_dialog_t::cancel(void) {
	set_key_timeout(old_timeout);
	close();
}

void input_selection_dialog_t::ok_activated(void) {
	hide();
	set_key_timeout(enable_simulate_box->get_state() ? (disable_timeout_box->get_state() ? 0 : -1000) : 100);
	activate();
}

void input_selection_dialog_t::check_state(void) {
	disable_timeout_box->set_enabled(enable_simulate_box->get_state());
}

}; // namespace
