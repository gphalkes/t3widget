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
#include <stdlib.h>
#include <string.h>

#include "widgets/widgets.h"
#include "dialogs/dialogs.h"
#include "main.h"
#include "keys.h"

#warning FIXME: should this be part of the main_window somehow?
menu_bar_t::menu_bar_t(bool _hide) : current_menu(0), hide(_hide) {
	int width;
	t3_term_get_size(NULL, &width);

	if ((topline = t3_win_new(NULL, 1, width, 0, 0, 50)) == NULL)
		throw -1;

	resize(None, width, None, None);

	if (!hide)
		t3_win_show(topline);
}

menu_bar_t::~menu_bar_t(void) {
	t3_win_del(topline);
}

void menu_bar_t::draw_menu_name(menu_panel_t *menu, int attr) {
	t3_win_set_paint(topline, 0, menu->colnr);
	t3_win_addch(topline, ' ', attr);
	menu->label->draw(topline, attr);
	t3_win_addch(topline, ' ', attr);
}

bool menu_bar_t::activate(key_t key) {
	size_t i;

	if (key == EKEY_F10) {
		old_menu = current_menu = 0;
		return true;
	}

	key = (key & ~EKEY_META) | ('a'-'A');
	for (i = 0; i < menus.size(); i++) {
		if (menus[i]->label->is_hotkey(key)) {
			old_menu = current_menu = i;
			return true;
		}
	}
	return false;
}

void menu_bar_t::process_key(key_t key) {
	if (menus.size() == 0)
		return;

	switch (key) {
		case EKEY_RIGHT:
			/* go to next menu */
			current_menu++;
			current_menu %= menus.size();
			break;
		case EKEY_LEFT:
			/* go to previous Menu */
			current_menu += menus.size() - 1;
			current_menu %= menus.size();
			break;
		default:
			menus[current_menu]->process_key(key);
			break;
	}
}

bool menu_bar_t::resize(optint height, optint width, optint top, optint left) {
	bool result;
	(void) height;
	(void) top;
	(void) left;

	result = t3_win_resize(topline, 1, width) == 0;
	t3_win_set_paint(topline, 0, 0);
	t3_win_addchrep(topline, ' ', colors.menubar_attrs, width);
	for (vector<menu_panel_t *>::iterator iter = menus.begin(); iter != menus.end(); iter++)
		draw_menu_name(*iter, colors.menubar_attrs);

	return result;
}

void menu_bar_t::update_contents(void) {
	if (old_menu == current_menu) {
		menus[current_menu]->update_contents();
		return;
	}
	if (old_menu != current_menu) {
		menus[old_menu]->set_show(false);
		menus[old_menu]->set_focus(false);
		menus[current_menu]->set_show(true);
		menus[current_menu]->set_focus(true);
		draw_menu_name(menus[old_menu], colors.menubar_attrs);
		draw_menu_name(menus[current_menu], colors.menubar_selected_attrs);
	}
	old_menu = current_menu;
	menus[current_menu]->update_contents();
}

void menu_bar_t::set_focus(bool focus) {
	if (focus)
		t3_term_hide_cursor();
	menus[current_menu]->set_focus(focus);
}

void menu_bar_t::set_show(bool show) {
	if (show) {
		if (hide)
			t3_win_show(topline);
		draw_menu_name(menus[current_menu], colors.menubar_selected_attrs);
	} else {
		if (hide)
			t3_win_hide(topline);
		draw_menu_name(menus[current_menu], colors.menubar_attrs);
	}
	menus[current_menu]->set_show(show);
}
