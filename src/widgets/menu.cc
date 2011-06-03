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

#include "colorscheme.h"
#include "widgets/menu.h"
#include "dialogs/menupanel.h"
#include "main.h"
#include "key.h"

using namespace std;
namespace t3_widget {

menu_bar_t::menu_bar_t(bool _hidden) : widget_t(1, 80), current_menu(0), old_menu(0),
		start_col(0), hidden(_hidden), has_focus(false)
{
	// Menu bar should be above normal widgets
	t3_win_set_depth(window, -1);
	if (hidden)
		t3_win_hide(window);
}

menu_bar_t::~menu_bar_t(void) {
	t3_win_del(window);
}

void menu_bar_t::draw_menu_name(menu_panel_t *menu, bool selected) {
	int attr = selected ? attributes.menubar_selected : attributes.menubar;
	t3_win_set_paint(window, 0, t3_win_get_x(menu->get_draw_window()) + 1);
	t3_win_addch(window, ' ', attr);
	menu->label.draw(window, attr, selected);
	t3_win_addch(window, ' ', attr);
}

void menu_bar_t::add_menu(menu_panel_t *menu) {
	menus.push_back(menu);
	menu->set_position(None, start_col);
	start_col += menu->label.get_width() + 2;
	redraw = true;
}

void menu_bar_t::close(void) {
	has_focus = false;
	if (hidden)
		t3_win_hide(window);
	draw_menu_name(menus[current_menu], false);
	menus[current_menu]->hide();
}

void menu_bar_t::next_menu(void) {
	current_menu++;
	current_menu %= menus.size();
}

void menu_bar_t::previous_menu(void) {
	current_menu += menus.size() - 1;
	current_menu %= menus.size();
}

bool menu_bar_t::process_key(key_t key) {
	if (menus.size() == 0)
		return false;

	switch (key) {
		case EKEY_HOTKEY:
			has_focus = true;
			if (hidden)
				t3_win_show(window);
			draw_menu_name(menus[current_menu], true);
			menus[current_menu]->show();
			return true;
		default:
			return false;
	}
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
			draw_menu_name(menus[current_menu], true);
	}

	if (!has_focus)
		return;

	if (old_menu == current_menu) {
		menus[current_menu]->update_contents();
		return;
	}
	if (old_menu != current_menu) {
		menus[old_menu]->hide();
		menus[current_menu]->show();
		draw_menu_name(menus[old_menu], false);
		draw_menu_name(menus[current_menu], true);
	}
	old_menu = current_menu;
	menus[current_menu]->update_contents();
}

void menu_bar_t::set_focus(bool focus) {
	(void) focus;
}

void menu_bar_t::show(void) {
	shown = true;
	if (!hidden || has_focus)
		t3_win_show(window);
}

bool menu_bar_t::is_hotkey(key_t key) {
	if (key == EKEY_F10 || key == (EKEY_META | '0')) {
		old_menu = current_menu = 0;
		return true;
	}

	for (int i = 0; i < (int) menus.size(); i++) {
		if (menus[i]->label.is_hotkey(key)) {
			old_menu = current_menu = i;
			return true;
		}
	}
	return false;
}

bool menu_bar_t::accepts_focus(void) { return false; }

void menu_bar_t::draw(void) {
	redraw = false;
	t3_win_set_paint(window, 0, 0);
	t3_win_addchrep(window, ' ', attributes.menubar, t3_win_get_width(window));
	for (vector<menu_panel_t *>::iterator iter = menus.begin(); iter != menus.end(); iter++)
		draw_menu_name(*iter, false);
}

}; // namespace
