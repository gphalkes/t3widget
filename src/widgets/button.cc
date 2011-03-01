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
#include "button.h"

button_t::button_t(window_component_t *parent, window_component_t *anchor, int _width, int _top, int _left, int relation, const char *_text, bool _isDefault) :
	width(_width), top(_top), left(_left), text(_text), is_default(_isDefault)
{
	text_width = text.get_width();

	if (anchor == NULL)
		anchor = parent;

	if ((window = t3_win_new_relative(parent->get_draw_window(), 1, width > 0 ? width : text_width + 4, top, left, 0, anchor->get_draw_window(), relation)) == NULL)
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

bool button_t::resize(optint height, optint _width, optint _top, optint _left) {
	(void) height;

	if (_top.is_valid())
		top = _top;

	if (_left.is_valid())
		left = _left;
#warning FIXME: this can not be right because width is never changed!
	if ((!_width.is_valid() && width >= 0) || (_width.is_valid() && _width != width))
		t3_win_resize(window, 1, _width.is_valid() ? (int) _width : text_width + 4);

	t3_win_move(window, top, left);
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
	return width >= 0 ? width : text_width + 4;
}

bool button_t::is_hotkey(key_t key) {
	return text.is_hotkey(key);
}
