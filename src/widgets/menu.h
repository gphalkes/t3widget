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
#ifndef EDIT_MENU_H
#define EDIT_MENU_H

#include <vector>
#include "interfaces.h"
#include "widgets/smartlabel.h"
//#include "main.h" //For ActionID

class menu_panel_t;

class menu_bar_t : public widget_t {
	friend class menu_panel_t;

	private:
		int current_menu, old_menu;
		int start_col;
		bool hidden, redraw, has_focus;

		std::vector<menu_panel_t *> menus;

		void draw_menu_name(menu_panel_t *menu, int attr);

		void add_menu(menu_panel_t *menu);
		void close(void);
		void next_menu(void);
		void previous_menu(void);
	public:
		menu_bar_t(container_t *parent, bool _hidden = false);
		~menu_bar_t(void);

		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
		void draw(void);

		//FIXME: allow removal of menu_panel_t's to allow dynamic menus

	SIGNAL(activate, void, int);
};
#endif
