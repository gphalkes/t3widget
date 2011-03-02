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

#warning FIXME: top should be derived automatically from adding to the menu_panel_t
#warning FIXME: passing a t3_window_t * as parent is wrong. Should probably refer to menu_panel_t
menu_item_t::menu_item_t(t3_window_t *_parent, const char *_label, const char *_hotkey, int _top, int _id) :
		menu_item_base_t(_parent, _top), label(_label), hotkey(_hotkey), id(_id)
{
	has_focus = false;
}

void menu_item_t::process_key(key_t key) {
	switch (key) {
		case EKEY_NL:
		case ' ':
		case EKEY_HOTKEY:
#warning FIXME: deactivate window here
			//deactivate_window();
#warning FIXME: signaling should be done from menu_bar_t class
			//executeAction(action);
			break;
		default:
			break;
	}
}

bool menu_item_t::resize(optint height, optint _width, optint _top, optint left) {
	(void) height;
	(void) left;
	if (_width.is_valid())
		width = _width;
	if (_top.is_valid())
		top = _top;
	return true;
}

void menu_item_t::update_contents(void) {
	int spaces;

	t3_win_set_paint(parent, top, 1);
	t3_win_addch(parent, ' ', has_focus ? colors.dialog_selected_attrs: colors.dialog_attrs);
	label.draw(parent, has_focus ? colors.dialog_selected_attrs : colors.dialog_attrs);

	spaces = width - 2 - label.get_width();
	if (hotkey != NULL) {
		spaces -= t3_term_strwidth(hotkey);
		t3_win_addchrep(parent, ' ', has_focus ? colors.dialog_selected_attrs : colors.dialog_attrs, spaces);
		t3_win_addstr(parent, hotkey, has_focus ? colors.dialog_selected_attrs : colors.dialog_attrs);
		t3_win_addch(parent, ' ', has_focus ? colors.dialog_selected_attrs : colors.dialog_attrs);
	} else {
		spaces++;
		t3_win_addchrep(parent, ' ', has_focus ? colors.dialog_selected_attrs : colors.dialog_attrs, spaces);
	}
}

void menu_item_t::set_focus(bool focus) {
	has_focus = focus;
}

void menu_item_t::set_show(bool show) {
	(void) show;
}

bool menu_item_t::is_hotkey(key_t key) {
	return label.is_hotkey(key);
}

int menu_item_t::get_label_width(void) {
	return label.get_width() + 2;
}

int menu_item_t::get_hotkey_width(void) {
	return hotkey == NULL ? 0 : (t3_term_strwidth(hotkey) + 2);
}

menu_separator_t::menu_separator_t(t3_window_t *_parent, int _top, int _width) : menu_item_base_t(_parent, _top) { width = _width; }

void menu_separator_t::process_key(key_t key) {
	(void) key;
}

bool menu_separator_t::resize(optint height, optint _width, optint _top, optint left) {
	(void) height;
	(void) left;
	if (_top.is_valid())
		top = _top;
	if (_width.is_valid())
		width = _width;
	return true;
}

void menu_separator_t::update_contents(void) {
	t3_win_set_paint(parent, top, 1);
	t3_win_addchrep(parent, T3_ACS_HLINE, T3_ATTR_ACS | colors.dialog_attrs, width);
}

void menu_separator_t::set_focus(bool focus) {
	(void) focus;
}

void menu_separator_t::set_show(bool show) {
	(void) show;
}

bool menu_separator_t::accepts_focus(void) {
	return false;
}

