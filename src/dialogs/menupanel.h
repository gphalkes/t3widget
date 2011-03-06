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
#ifndef MENUPANEL_H
#define MENUPANEL_H

#include "dialogs/dialogs.h"

class menu_item_t;

class menu_panel_t : public dialog_t {
	friend class menu_bar_t;
	private:
		int width, label_width, hotkey_width;
		smart_label_text_t label;
		menu_bar_t *menu;

	public:
		menu_panel_t(menu_bar_t *_menu, const char *name);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		void close(void);
		void add_item(const char *label, const char *hotkey, int id);
		void add_item(menu_item_t *item);
		void add_separator(void);

		void signal(int id);

		//FIXME: allow removal of items to allow dynamic menus
};

#endif
