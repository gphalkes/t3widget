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

#include "colorscheme.h"
#include "widgets/button.h"

button_t::button_t(container_t *parent, const char *_text, bool _isDefault) : text(_text), is_default(_isDefault)
{
	text_width = text.get_width();
	width = text_width + 4;

	if ((window = t3_win_new(parent->get_draw_window(), 1, width, 0, 0, 0)) == NULL)
		throw(-1);
	t3_win_set_default_attrs(window, colors.button_attrs);

	has_focus = false;
	update_contents();
	t3_win_show(window);
}

void button_t::process_key(key_t key) {
	switch (key) {
		case EKEY_HOTKEY:
		case EKEY_NL:
		case ' ':
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
			break;
	}
}

bool button_t::set_size(optint height, optint _width) {
	(void) height;

	if (_width.is_valid()) {
		if (_width <= 0) {
			if (text_width + 4 == width)
				return true;
			width = text_width + 4;
		} else {
			width = _width;
		}
		return t3_win_resize(window, 1, width);
	}
	return true;
}

void button_t::update_contents(void) {
	int attr = has_focus ? colors.button_selected_attrs : 0;

	t3_win_set_paint(window, 0, 0);
	t3_win_addstr(window, is_default ? "[<" : "[ ", attr);
	if (width > text_width + 4)
		t3_win_addchrep(window, ' ', attr, (width - 4 - text_width) / 2);
	text.draw(window, attr);
	if (width > text_width + 4)
		t3_win_addchrep(window, ' ', attr, (width - 4 - text_width + 1) / 2);
	else if (width > 0)
		t3_win_set_paint(window, 0, width - 2);
	t3_win_addstr(window, is_default ? ">]" : " ]", attr);
}

void button_t::set_focus(bool focus) {
	has_focus = focus;
	if (focus)
		t3_term_hide_cursor();
}

int button_t::get_width(void) {
	return width;
}

bool button_t::is_hotkey(key_t key) {
	return text.is_hotkey(key);
}
