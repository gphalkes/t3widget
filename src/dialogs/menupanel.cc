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
#include "main.h"
#include "dialogs/menupanel.h"
#include "widgets/menuitem.h"
#include "util.h"

using namespace std;

menu_panel_t::menu_panel_t(int left) : dialog_t(3, 5, 1, left, 40, NULL) {
	width = 5;
	label_width = 1;
	hotkey_width = 0;
	draw_dialog();
}

void menu_panel_t::draw_dialog(void) {
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);
	dialog_t::draw_dialog();
	update_contents();
}

void menu_panel_t::process_key(key_t key) {
	switch (key) {
		case EKEY_UP:
			focus_previous();
			break;
		case EKEY_DOWN:
			focus_next();
			break;
		case EKEY_HOME:
			(*current_widget)->set_focus(false);
			current_widget = widgets.begin();
			(*current_widget)->set_focus(true);
			break;
		case EKEY_END:
			(*current_widget)->set_focus(false);
			current_widget = widgets.end();
			current_widget--;
			(*current_widget)->set_focus(true);
			break;
		case '\t':
		case EKEY_SHIFT | '\t':
			break;
		case EKEY_ESC:
			hide();
			break;
		case EKEY_NL:
		case ' ':
			(*current_widget)->process_key(key);
			break;
		default:
			for (widgets_t::iterator iter = widgets.begin();
					iter != widgets.end(); iter++) {
				if ((*iter)->accepts_focus() && (*iter)->is_hotkey(key)) {
					(*current_widget)->set_focus(false);
					current_widget = iter;
					(*current_widget)->set_focus(true);
					(*current_widget)->process_key(EKEY_HOTKEY);
					break;
				}
			}
			break;
	}
}

bool menu_panel_t::resize(optint height, optint _width, optint top, optint left) {
	widgets_t::iterator iter;
	bool result;
	int i;
	(void) _width;
	(void) top;
	for (iter = widgets.begin(), i = 0; iter != widgets.end(); iter++, i++)
		(*iter)->resize(None, width - 2, i + 1, None);

	result = dialog_t::resize(height, width, 1, left);
	draw_dialog();
	return result;
}

void menu_panel_t::add_item(const char *item, const char *hotkey, int action) {
	menu_item_t *menu_item;
	if (strcmp(item, "-") == 0) {
		widgets.push_back(new menu_separator_t(window, widgets.size() + 1, width - 2));
		return;
	}

	menu_item = new menu_item_t(window, item, hotkey, widgets.size() + 1, action);
	widgets.push_back(menu_item);

	hotkey_width = max(hotkey_width, menu_item->get_hotkey_width());
	label_width = max(label_width, menu_item->get_label_width());
	if (hotkey_width + label_width > width - 2)
		width = hotkey_width + label_width + 2;
	resize(widgets.size() + 2, width, 1, t3_win_get_x(window));
}
