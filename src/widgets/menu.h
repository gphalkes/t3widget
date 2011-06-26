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
#ifndef T3_WIDGET_MENU_H
#define T3_WIDGET_MENU_H

#include <vector>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/smartlabel.h>

namespace t3_widget {

class menu_panel_t;

class T3_WIDGET_API menu_bar_t : public widget_t {
	friend class menu_panel_t;

	private:
		int current_menu, old_menu;
		int start_col;
		bool hidden, has_focus;

		std::vector<menu_panel_t *> menus;

		void draw_menu_name(menu_panel_t *menu, bool selected);

		void add_menu(menu_panel_t *menu);
		void close(void);
		void next_menu(void);
		void previous_menu(void);
	public:
		menu_bar_t(bool _hidden = false);
		~menu_bar_t(void);

		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
		void draw(void);

		//FIXME: allow removal of menu_panel_t's to allow dynamic menus

	T3_WIDGET_SIGNAL(activate, void, int);
};

}; // namespace
#endif
