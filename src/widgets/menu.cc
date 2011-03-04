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

using namespace std;

menu_bar_t::menu_bar_t(container_t *parent, bool _hidden) : widget_t(parent, 1, t3_win_get_width(parent->get_draw_window())),
		current_menu(0), hidden(_hidden), redraw(true), has_focus(false)
{
	// Menu bar should be above normal widgets
	t3_win_set_depth(window, -1);
	if (hidden)
		t3_win_hide(window);
}

menu_bar_t::~menu_bar_t(void) {
	t3_win_del(window);
}

void menu_bar_t::draw_menu_name(menu_panel_t *menu, int attr) {
	t3_win_set_paint(window, 0, menu->colnr);
	t3_win_addch(window, ' ', attr);
	menu->label->draw(window, attr);
	t3_win_addch(window, ' ', attr);
}

bool menu_bar_t::process_key(key_t key) {
	if (menus.size() == 0)
		return false;

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
		case EKEY_HOTKEY:
			break;
		default:
			return menus[current_menu]->process_key(key);
	}
	return true;
}

bool menu_bar_t::set_size(optint height, optint width) {
	(void) height;
	if (!width.is_valid())
		return true;
	redraw = true;
	return t3_win_resize(window, 1, width) == 0;
}

void menu_bar_t::update_contents(void) {
	if (redraw) {
		draw();
		if (has_focus)
			draw_menu_name(menus[current_menu], colors.menubar_selected_attrs);
	}

	if (old_menu == current_menu) {
		menus[current_menu]->update_contents();
		return;
	}
	if (old_menu != current_menu) {
		menus[old_menu]->hide();
		menus[old_menu]->set_focus(false);
		menus[current_menu]->show();
		menus[current_menu]->set_focus(true);
		draw_menu_name(menus[old_menu], colors.menubar_attrs);
		draw_menu_name(menus[current_menu], colors.menubar_selected_attrs);
	}
	old_menu = current_menu;
	menus[current_menu]->update_contents();
}

void menu_bar_t::set_focus(bool focus) {
	has_focus = focus;
	if (focus) {
		t3_term_hide_cursor();
		if (hidden)
			t3_win_show(window);
		draw_menu_name(menus[current_menu], colors.menubar_selected_attrs);
		menus[current_menu]->show();
	} else {
		if (hidden)
			t3_win_hide(window);
		draw_menu_name(menus[current_menu], colors.menubar_attrs);
		menus[current_menu]->hide();
	}
	menus[current_menu]->set_focus(focus);
}

void menu_bar_t::show(void) {
	if (!hidden || has_focus)
		t3_win_show(window);
}

void menu_bar_t::hide(void) {
	t3_win_hide(window);
}

bool menu_bar_t::is_hotkey(key_t key) {
	if (key == EKEY_MENU) {
		old_menu = current_menu = 0;
		return true;
	}

	for (int i = 0; i < (int) menus.size(); i++) {
		if (menus[i]->label->is_hotkey(key)) {
			old_menu = current_menu = i;
			return true;
		}
	}
	return false;
}

bool menu_bar_t::accepts_focus(void) { return false; }

void menu_bar_t::draw(void) {
	t3_win_set_paint(window, 0, 0);
	t3_win_addchrep(window, ' ', colors.menubar_attrs, t3_win_get_width(window));
	for (vector<menu_panel_t *>::iterator iter = menus.begin(); iter != menus.end(); iter++)
		draw_menu_name(*iter, colors.menubar_attrs);
}
