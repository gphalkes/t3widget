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
#include "dialogs/mainwindow.h"

window_components_t dialog_t::dialogs;
int dialog_t::dialog_depth;
window_component_t *dialog_t::main_window;

void dialog_t::init(void) {
	main_window = new main_window_t();
	dialogs.push_back(main_window);
}

dialog_t::dialog_t(int height, int width, int top, int left, int depth, const char *_title) : active(false), title(_title) {
	if ((window = t3_win_new(NULL, height + 1, width + 1, top, left, depth)) == NULL)
		throw bad_alloc();
	t3_win_set_default_attrs(window, colors.dialog_attrs);
}

dialog_t::~dialog_t() {
	t3_win_del(window);
}

void dialog_t::activate_dialog(void) {
	if (this == dialogs.back())
		return;

	dialogs.back()->set_focus(false);
	if (this->active) {
		for (window_components_t::iterator iter = dialogs.begin(); iter != dialogs.end(); iter++) {
			if (*iter == this) {
				dialogs.erase(iter);
				break;
			}
		}
	}

	this->active = true;
	this->set_focus(true);
	#warning FIXME: implement this in the window library
	//t3_window_set_depth(this->window, --dialog_depth);
	dialogs.push_back(this);
}

void dialog_t::deactivate_dialog(void) {
	this->active = false;
	if (this == dialogs.back()) {
		this->set_focus(false);
		dialogs.pop_back();
		dialogs.back()->set_focus(true);
		dialog_depth++;
		return;
	}

	for (window_components_t::iterator iter = dialogs.begin(); iter != dialogs.end(); iter++) {
		if (*iter == this) {
			dialogs.erase(iter);
			break;
		}
	}
}

void dialog_t::draw_dialog(void) {
	int i, x;

	t3_win_box(window, 0, 0, t3_win_get_height(window) - 1, t3_win_get_width(window) - 1, 0);
	if (title != NULL) {
		t3_win_set_paint(window, 0, 2);
		t3_win_addstr(window, "[ ", 0);
		t3_win_addstr(window, title, 0);
		t3_win_addstr(window, " ]", 0);
	}

	x = t3_win_get_width(window) - 1;
	for (i = t3_win_get_height(window) - 1; i > 0; i--) {
		t3_win_set_paint(window, i, x);
		t3_win_addch(window, ' ', T3_ATTR_REVERSE);
	}
	t3_win_set_paint(window, t3_win_get_height(window) - 1, 1);
	t3_win_addchrep(window, ' ', T3_ATTR_REVERSE, t3_win_get_width(window) - 1);
}

void dialog_t::process_key(key_t key) {
	if (key & EKEY_META) {
		for (widgets_t::iterator iter = widgets.begin();
				iter != widgets.end(); iter++) {
			if ((*iter)->accepts_focus() && (*iter)->is_hotkey(key & ~EKEY_META)) {
				(*current_widget)->set_focus(false);
				current_widget = iter;
				(*current_widget)->set_focus(true);
				(*current_widget)->process_key(EKEY_HOTKEY);
				break;
			}
		}
		return;
	}

	switch (key) {
		case EKEY_ESC:
			set_show(false);
			break;
		case '\t':
			focus_next();
			break;
		case EKEY_SHIFT | '\t':
			focus_previous();
			break;
		default:
			(*current_widget)->process_key(key);
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

	result &= (t3_win_resize(window, height + 1, width + 1) == 0);
	t3_win_move(window, top, left);
	return result;
}

void dialog_t::update_contents(void) {
	for (widgets_t::iterator iter = widgets.begin();
			iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void dialog_t::set_focus(bool focus) {
	(*current_widget)->set_focus(focus);
}

void dialog_t::set_show(bool show) {
	if (show) {
		activate_dialog();
		for (current_widget = widgets.begin();
			!(*current_widget)->accepts_focus() && current_widget != widgets.end();
			current_widget++)
		{}

		t3_win_show(window);
	} else {
		deactivate_dialog();
		t3_win_hide(window);
	}
}

void dialog_t::focus_next(void) {
	(*current_widget)->set_focus(false);
	do {
		current_widget++;
		if (current_widget == widgets.end())
			current_widget = widgets.begin();
	} while (!(*current_widget)->accepts_focus());

	(*current_widget)->set_focus(true);
}

void dialog_t::focus_previous(void) {
	(*current_widget)->set_focus(false);

	do {
		if (current_widget == widgets.begin())
			current_widget = widgets.end();

		current_widget--;
	} while (!(*current_widget)->accepts_focus());

	(*current_widget)->set_focus(true);
}
