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
#include <cstring>

#include "main.h"
#include "dialogs/menupanel.h"
#include "widgets/menuitem.h"
#include "util.h"

using namespace std;
namespace t3_widget {

menu_panel_t::menu_panel_t(const char *name, menu_bar_t *_menu_bar) : dialog_t(3, 5, NULL), impl(new implementation_t(name)) {
	impl->width = 5;
	impl->label_width = 1;
	impl->hotkey_width = 0;
	if (_menu_bar != NULL)
		_menu_bar->add_menu(this);
}

bool menu_panel_t::process_key(key_t key) {
	switch (key) {
		case EKEY_LEFT:
			if (impl->menu_bar != NULL)
				impl->menu_bar->previous_menu();
			break;
		case EKEY_RIGHT:
			if (impl->menu_bar != NULL)
				impl->menu_bar->next_menu();
			break;
		case EKEY_UP:
			focus_previous();
			break;
		case EKEY_DOWN:
			focus_next();
			break;
		case EKEY_HOME:
			(*current_widget)->set_focus(window_component_t::FOCUS_OUT);
			current_widget = widgets.begin();
			(*current_widget)->set_focus(window_component_t::FOCUS_SET);
			break;
		case EKEY_END:
			(*current_widget)->set_focus(window_component_t::FOCUS_OUT);
			current_widget = widgets.end();
			current_widget--;
			(*current_widget)->set_focus(window_component_t::FOCUS_SET);
			break;
		case '\t':
		case EKEY_SHIFT | '\t':
			break;
		case EKEY_ESC:
			if (impl->menu_bar != NULL)
				impl->menu_bar->close();
			break;
		case EKEY_NL:
		case ' ':
			(*current_widget)->process_key(key);
			break;
		default:
			for (widgets_t::iterator iter = widgets.begin();
					iter != widgets.end(); iter++) {
				if ((*iter)->accepts_focus() && (*iter)->is_hotkey(key)) {
					(*current_widget)->set_focus(window_component_t::FOCUS_OUT);
					current_widget = iter;
					(*current_widget)->set_focus(window_component_t::FOCUS_SET);
					(*current_widget)->process_key(EKEY_HOTKEY);
					return true;
				}
			}
			return false;
	}
	return true;
}

void menu_panel_t::set_position(optint top, optint left) {
	(void) top;
	dialog_t::set_position(1, left);
}

bool menu_panel_t::set_size(optint height, optint _width) {
	widgets_t::iterator iter;
	bool result;
	int i;
	(void) _width;
	for (iter = widgets.begin(), i = 0; iter != widgets.end(); iter++, i++)
		(*iter)->set_size(None, impl->width - 2);

	result = dialog_t::set_size(height, impl->width);
	return result;
}

void menu_panel_t::close(void) {
	if (impl->menu_bar != NULL)
		impl->menu_bar->close();
}

menu_item_base_t *menu_panel_t::add_item(const char *_label, const char *hotkey, int id) {
	menu_item_t *item = new menu_item_t(this, _label, hotkey, id);
	return add_item(item);
}

menu_item_base_t *menu_panel_t::add_item(menu_item_t *item) {
	push_back(item);
	item->set_position(widgets.size(), None);

	impl->hotkey_width = max(impl->hotkey_width, item->get_hotkey_width());
	impl->label_width = max(impl->label_width, item->get_label_width());
	if (impl->hotkey_width + impl->label_width > impl->width - 2)
		impl->width = impl->hotkey_width + impl->label_width + 2;
	set_size(widgets.size() + 2, impl->width);
	return item;
}

menu_item_base_t *menu_panel_t::add_separator(void) {
	menu_separator_t *sep = new menu_separator_t(this);
	sep->set_position(widgets.size() + 1, None);
	push_back(sep);
	return sep;
}

void menu_panel_t::remove_item(menu_item_base_t *item) {
	widgets_t::iterator iter;
	menu_item_t *label_item;
	int i;

	for (iter = widgets.begin(); iter != widgets.end(); iter++) {
		if ((*iter) == item) {
			unset_widget_parent(item);
			widgets.erase(iter);
			goto resize_panel;
		}
	}
	return;

resize_panel:
	impl->width = 5;
	impl->label_width = 1;
	impl->hotkey_width = 0;
	for (iter = widgets.begin(), i = 1; iter != widgets.end(); iter++, i++) {
		(*iter)->set_position(i, None);
		label_item = dynamic_cast<menu_item_t *>(*iter);
		if (label_item != NULL) {
			impl->hotkey_width = max(impl->hotkey_width, label_item->get_hotkey_width());
			impl->label_width = max(impl->label_width, label_item->get_label_width());
		}
		if (impl->hotkey_width + impl->label_width > impl->width - 2)
			impl->width = impl->hotkey_width + impl->label_width + 2;
	}
	set_size(widgets.size() + 2, impl->width);
}

void menu_panel_t::signal(int id) {
	if (impl->menu_bar != NULL)
		impl->menu_bar->activate(id);
}

void menu_panel_t::set_menu_bar(menu_bar_t *_menu_bar) {
	if (impl->menu_bar == _menu_bar)
		return;

	if (_menu_bar == NULL) {
		impl->menu_bar = NULL;
		t3_win_set_anchor(window, NULL, 0);
	} else {
		if (impl->menu_bar != NULL)
			impl->menu_bar->remove_menu(this);
		impl->menu_bar = _menu_bar;
		t3_win_set_anchor(window, impl->menu_bar->get_base_window(), 0);
	}
}

void menu_panel_t::draw_label(t3_window_t *draw_window, t3_attr_t attr, bool selected) const {
	impl->label.draw(draw_window, attr, selected);
}

int menu_panel_t::get_label_width(void) const {
	return impl->label.get_width();
}

bool menu_panel_t::is_hotkey(key_t key) const {
	return impl->label.is_hotkey(key);
}

bool menu_panel_t::is_child(window_component_t *widget) {
	return dialog_t::is_child(widget);
}

void menu_panel_t::set_focus_from_xy(int x, int y) {
	if (x < 1 || x > t3_win_get_width(window) - 2 || y < 1 || y > t3_win_get_height(window) - 2)
		return;
	(*current_widget)->set_focus(FOCUS_OUT);
	current_widget = widgets.begin() + (y - 1);
	(*current_widget)->set_focus(FOCUS_SET);
}

}; // namespace
