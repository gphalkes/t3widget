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
#include "colorscheme.h"
#include "widgets/checkbox.h"

namespace t3_widget {

checkbox_t::checkbox_t(container_t *parent, bool _state) : widget_t(parent, 1, 3), state(_state), has_focus(false), label(NULL) {}

bool checkbox_t::process_key(key_t key) {
	switch (key) {
		case ' ':
		case EKEY_HOTKEY:
			state ^= true;
			redraw = true;
			toggled();
			update_contents();
			break;
		case EKEY_NL:
			activate();
			break;
		case EKEY_LEFT:
			move_focus_left();
			break;
		case EKEY_RIGHT:
			move_focus_right();
			break;
		case EKEY_UP:
			move_focus_up();
			break;
		case EKEY_DOWN:
			move_focus_down();
			break;
		default:
			return false;
	}
	return true;
}

bool checkbox_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void checkbox_t::update_contents(void) {
	if (!redraw)
		return;
	redraw = false;
	t3_win_set_default_attrs(window, colors.dialog_attrs);
	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, '[', 0);
	t3_win_addch(window, state ? 'X' : ' ', has_focus ? T3_ATTR_REVERSE : 0);
	t3_win_addch(window, ']', 0);
}

void checkbox_t::set_focus(bool focus) {
	if (has_focus != focus)
		redraw = true;

	has_focus = focus;
	if (focus)
		t3_term_hide_cursor();
}

bool checkbox_t::get_state(void) {
	return state;
}

void checkbox_t::set_state(bool _state) {
	state = _state;
}

void checkbox_t::set_label(smart_label_t *_label) {
	label = _label;
}

bool checkbox_t::is_hotkey(key_t key) {
	return label == NULL ? false : label->is_hotkey(key);
}

}; // namespace
