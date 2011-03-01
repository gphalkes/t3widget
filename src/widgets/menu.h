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

class menu_bar_t : public window_component_t {
	private:
		t3_window_t *topline;
		int current_menu, old_menu;
		bool hide;

		vector<menu_panel_t *> menus;

		//void drawItem(menu_panel_t *menu, int item, int attr);
		void draw_menu_name(menu_panel_t *menu, int attr);
	public:
		menu_bar_t(bool _hide = false);
		~menu_bar_t(void);

		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void set_show(bool show);
		bool activate(key_t key);
};
#endif
