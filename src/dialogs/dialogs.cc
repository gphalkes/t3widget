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
#include "main.h"
#include "dialogs/dialogs.h"

dialog_t::dialog_t(int height, int width, int top, int left, int depth, const char *_title) : title(_title) {
	if ((window = t3_win_new(NULL, height, width, top, left, depth)) == NULL)
		throw bad_alloc();
	if ((shadow_window = t3_win_new(NULL, height, width, top + 1, left + 1, depth + 1)) == NULL) {
		t3_win_del(window);
		throw bad_alloc();
	}
	t3_win_set_default_attrs(window, colors.dialog_attrs);
}

dialog_t::~dialog_t() {
	t3_win_del(window);
	t3_win_del(shadow_window);
}

void dialog_t::draw_dialog(void) {
	t3_win_box(window, 0, 0, t3_win_get_height(window), t3_win_get_width(window), 0);
	if (title != NULL) {
		t3_win_set_paint(window, 0, 2);
		t3_win_addstr(window, "[ ", 0);
		t3_win_addstr(window, title, 0);
		t3_win_addstr(window, " ]", 0);
	}

	for (int i = 0; i < t3_win_get_height(shadow_window); i++) {
		t3_win_set_paint(shadow_window, i, t3_win_get_width(shadow_window) - 1);
		t3_win_addch(shadow_window, ' ', T3_ATTR_REVERSE);
	}
	t3_win_set_paint(shadow_window, t3_win_get_height(shadow_window) - 1, 0);
	t3_win_addchrep(shadow_window, ' ', T3_ATTR_REVERSE, t3_win_get_width(shadow_window));
}

void dialog_t::process_key(key_t key) {
	if (key & EKEY_META) {
		for (widgets_t::iterator iter = components.begin();
				iter != components.end(); iter++) {
			if ((*iter)->accepts_focus() && (*iter)->is_hotkey(key & ~EKEY_META)) {
				(*current_component)->set_focus(false);
				current_component = iter;
				(*current_component)->set_focus(true);
				(*current_component)->process_key(EKEY_HOTKEY);
				break;
			}
		}
		return;
	}

	switch (key) {
		case EKEY_ESC:
			deactivate_window();
			break;
		case '\t':
			focus_next();
			break;
		case EKEY_SHIFT | '\t':
			focus_previous();
			break;
		default:
			(*current_component)->process_key(key);
			break;
	}
}

bool dialog_t::resize(optint height, optint width, optint top, optint left) {
	bool result = true;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);
	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);

	result &= (t3_win_resize(window, height, width) == 0);
	result &= (t3_win_resize(shadow_window, height, width) == 0);
	t3_win_move(window, top, left);
	t3_win_move(shadow_window, top + 1, left + 1);
	return result;
}

void dialog_t::update_contents(void) {
	for (widgets_t::iterator iter = components.begin();
			iter != components.end(); iter++)
		(*iter)->update_contents();
}

void dialog_t::set_focus(bool focus) {
	(*current_component)->set_focus(focus);
}

void dialog_t::set_show(bool show) {
	if (show) {
		for (current_component = components.begin();
			!(*current_component)->accepts_focus() && current_component != components.end();
			current_component++)
		{}

		t3_win_show(window);
		t3_win_show(shadow_window);
	} else {
		t3_win_hide(window);
		t3_win_hide(shadow_window);
	}
}

void dialog_t::focus_next(void) {
	(*current_component)->set_focus(false);
	do {
		current_component++;
		if (current_component == components.end())
			current_component = components.begin();
	} while (!(*current_component)->accepts_focus());

	(*current_component)->set_focus(true);
}

void dialog_t::focus_previous(void) {
	(*current_component)->set_focus(false);

	do {
		if (current_component == components.begin())
			current_component = components.end();

		current_component--;
	} while (!(*current_component)->accepts_focus());

	(*current_component)->set_focus(true);
}
