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
#include "widgets/menuitem.h"

namespace t3_widget {

menu_item_t::menu_item_t(menu_panel_t *_parent, const char *_label, const char *_hotkey, int _id) :
		menu_item_base_t(_parent), label(_label), hotkey(_hotkey), id(_id)
{
	has_focus = false;
}

bool menu_item_t::process_key(key_t key) {
	switch (key) {
		case EKEY_NL:
		case ' ':
		case EKEY_HOTKEY:
			parent->close();
			parent->signal(id);
			break;
		default:
			return false;
	}
	return true;
}

void menu_item_t::set_position(optint _top, optint left) {
	(void) left;
	if (_top.is_valid())
		top = _top;
}

bool menu_item_t::set_size(optint height, optint _width) {
	(void) height;
	if (_width.is_valid())
		width = _width;
	return true;
}

void menu_item_t::update_contents(void) {
	t3_window_t *parent_window = parent->get_draw_window();
	t3_attr_t attrs = has_focus ? attributes.dialog_selected: attributes.dialog;
	int spaces;

	t3_win_set_paint(parent_window, top, 1);
	t3_win_addch(parent_window, ' ', attrs);
	label.draw(parent_window, attrs, has_focus);

	spaces = width - 2 - label.get_width();
	if (hotkey != NULL) {
		spaces -= t3_term_strwidth(hotkey);
		t3_win_addchrep(parent_window, ' ', attrs, spaces);
		t3_win_addstr(parent_window, hotkey, attrs);
		t3_win_addch(parent_window, ' ', attrs);
	} else {
		spaces++;
		t3_win_addchrep(parent_window, ' ', attrs, spaces);
	}
}

void menu_item_t::set_focus(bool focus) {
	menu_item_base_t::set_focus(focus);
	has_focus = focus;
}

void menu_item_t::show(void) {}
void menu_item_t::hide(void) {}

bool menu_item_t::is_hotkey(key_t key) {
	return label.is_hotkey(key);
}

int menu_item_t::get_label_width(void) {
	return label.get_width() + 2;
}

int menu_item_t::get_hotkey_width(void) {
	return hotkey == NULL ? 0 : (t3_term_strwidth(hotkey) + 2);
}

menu_separator_t::menu_separator_t(menu_panel_t *_parent) : menu_item_base_t(_parent) {}

bool menu_separator_t::process_key(key_t key) {
	(void) key;
	return false;
}

void menu_separator_t::set_position(optint _top, optint left) {
	(void) left;
	if (_top.is_valid())
		top = _top;
}

bool menu_separator_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}


void menu_separator_t::update_contents(void) {
	t3_window_t *parent_window = parent->get_draw_window();
	t3_win_set_paint(parent_window, top, 1);
	t3_win_addchrep(parent_window, T3_ACS_HLINE, T3_ATTR_ACS | attributes.dialog, t3_win_get_width(parent_window) - 2);
}

void menu_separator_t::set_focus(bool focus) {
	(void) focus;
}

void menu_separator_t::show(void) {}
void menu_separator_t::hide(void) {}

bool menu_separator_t::accepts_focus(void) {
	return false;
}

}; // namespace
