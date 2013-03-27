/* Copyright (C) 2011-2012 G.P. Halkes
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
#include "log.h"

using namespace std;
namespace t3_widget {

menu_bar_t::menu_bar_t(bool _hidden) : widget_t(1, 80), impl(new implementation_t(_hidden)) {
	// Menu bar should be above normal widgets
	t3_win_set_depth(window, -1);
	if (impl->hidden)
		t3_win_hide(window);
}

menu_bar_t::~menu_bar_t(void) {
	for (vector<menu_panel_t *>::iterator iter = impl->menus.begin(); iter != impl->menus.end(); iter++)
		delete *iter;
}

void menu_bar_t::draw_menu_name(menu_panel_t *menu, bool selected) {
	int attr = selected ? attributes.menubar_selected : attributes.menubar;
	t3_win_set_paint(window, 0, t3_win_get_x(menu->get_base_window()) + 1);
	t3_win_addch(window, ' ', attr);
	menu->draw_label(window, attr, selected);
	t3_win_addch(window, ' ', attr);
}

void menu_bar_t::add_menu(menu_panel_t *menu) {
	impl->menus.push_back(menu);
	menu->set_menu_bar(this);
	menu->set_position(None, impl->start_col);
	impl->start_col += menu->get_label_width() + 2;
	redraw = true;
}

void menu_bar_t::remove_menu(menu_panel_t *menu) {
	int idx = 0;
	for (std::vector<menu_panel_t *>::iterator iter = impl->menus.begin(); iter != impl->menus.end(); iter++, idx++) {
		if (*iter == menu) {
			menu->set_menu_bar(NULL);

			if (impl->current_menu == idx) {
				if (impl->has_focus) {
					(*iter)->hide();
					next_menu();
					impl->menus[impl->current_menu]->show();
				}
			} else if (impl->current_menu > idx) {
				impl->current_menu--;
			}
			impl->old_menu = 0; // Make sure impl->old_menu isn't out of bounds

			impl->start_col = t3_win_get_x((*iter)->get_base_window());
			iter = impl->menus.erase(iter);
			/* Move all the remaining impl->menus to their new position. */
			for (; iter != impl->menus.end(); iter++) {
				(*iter)->set_position(None, impl->start_col);
				impl->start_col += (*iter)->get_label_width() + 2;
			}
			redraw = true;
			return;
		}
	}
}

void menu_bar_t::close(void) {
	impl->has_focus = false;
	if (impl->hidden)
		t3_win_hide(window);
	draw_menu_name(impl->menus[impl->current_menu], false);
	impl->menus[impl->current_menu]->hide();
	release_mouse_grab();
}

void menu_bar_t::next_menu(void) {
	impl->current_menu++;
	impl->current_menu %= impl->menus.size();
}

void menu_bar_t::previous_menu(void) {
	impl->current_menu += impl->menus.size() - 1;
	impl->current_menu %= impl->menus.size();
}

bool menu_bar_t::process_key(key_t key) {
	if (impl->menus.size() == 0)
		return false;

	switch (key) {
		case EKEY_HOTKEY:
			show();
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
		if (impl->has_focus)
			draw_menu_name(impl->menus[impl->current_menu], true);
		impl->old_menu = impl->current_menu;
	}

	if (!impl->has_focus)
		return;

	if (impl->old_menu == impl->current_menu) {
		impl->menus[impl->current_menu]->update_contents();
		return;
	}
	impl->menus[impl->old_menu]->hide();
	impl->menus[impl->current_menu]->show();
	draw_menu_name(impl->menus[impl->old_menu], false);
	draw_menu_name(impl->menus[impl->current_menu], true);
	impl->old_menu = impl->current_menu;
	impl->menus[impl->current_menu]->update_contents();
}

void menu_bar_t::set_focus(focus_t focus) {
	(void) focus;
}

void menu_bar_t::show(void) {
	if (!impl->has_focus) {
		impl->has_focus = true;
		redraw = true;
		t3_win_show(window);
		draw_menu_name(impl->menus[impl->current_menu], true);
		impl->menus[impl->current_menu]->show();
		grab_mouse();
	}
}

void menu_bar_t::hide() {
	release_mouse_grab();
	widget_t::hide();
}

bool menu_bar_t::is_hotkey(key_t key) {
	if (key == EKEY_F10 || key == '0') {
		impl->old_menu = impl->current_menu = 0;
		return true;
	}

	for (int i = 0; i < (int) impl->menus.size(); i++) {
		if (impl->menus[i]->is_hotkey(key)) {
			impl->old_menu = impl->current_menu = i;
			return true;
		}
	}
	return false;
}

bool menu_bar_t::accepts_focus(void) { return false; }

bool menu_bar_t::process_mouse_event(mouse_event_t event) {
	bool outside_area, on_bar;
	int current_menu_x;

	event.type &= ~EMOUSE_OUTSIDE_GRAB;

	if (event.y == 0) {
		outside_area = event.x < 0 || event.x >= t3_win_get_width(window);
		on_bar = !outside_area;
	} else {
		int current_menu_width = t3_win_get_width(impl->menus[impl->current_menu]->get_base_window());
		int current_menu_height = t3_win_get_height(impl->menus[impl->current_menu]->get_base_window());
		current_menu_x = t3_win_get_x(impl->menus[impl->current_menu]->get_base_window());

		outside_area = event.x < current_menu_x || event.x >= current_menu_x + current_menu_width ||
				event.y > current_menu_height || event.y < 0;
		on_bar = false;
	}

	if (on_bar) {
		if ((event.type == EMOUSE_BUTTON_PRESS || event.type == EMOUSE_MOTION) && (event.button_state & EMOUSE_BUTTON_LEFT)) {
			int clicked_idx = coord_to_menu_idx(event.x);
			if (event.y == 0) {
				if (clicked_idx != -1) {
					impl->current_menu = clicked_idx;
					show();
				}
			}
		}
	} else if (outside_area) {
		if (event.type != EMOUSE_MOTION) {
			close();
			return false;
		}
	} else {
		if (event.type == EMOUSE_MOTION) {
			impl->menus[impl->current_menu]->set_focus_from_xy(event.x - current_menu_x, event.y - 1);
			return true;
		}
		return false;
	}
	return true;
}

int menu_bar_t::coord_to_menu_idx(int x) {
	std::vector<menu_panel_t *>::iterator iter;
	int idx;
	int menu_start;

	for (iter = impl->menus.begin(), idx = 0; iter != impl->menus.end(); iter++, idx++) {
		menu_start = t3_win_get_x((*iter)->get_base_window()) + 2;
		if (x < menu_start)
			return -1;
		if (x < menu_start + (*iter)->get_label_width())
			return idx;
	}
	return -1;
}

void menu_bar_t::draw(void) {
	redraw = false;
	t3_win_set_paint(window, 0, 0);
	t3_win_addchrep(window, ' ', attributes.menubar, t3_win_get_width(window));
	for (vector<menu_panel_t *>::iterator iter = impl->menus.begin(); iter != impl->menus.end(); iter++)
		draw_menu_name(*iter, false);
}

void menu_bar_t::set_hidden(bool _hidden) {
	impl->hidden = _hidden;
	if (impl->hidden)
		t3_win_hide(window);
	else
		t3_win_show(window);
}

}; // namespace
