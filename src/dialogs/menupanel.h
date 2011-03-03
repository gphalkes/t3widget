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

class menu_panel_t : public dialog_t {
	friend class menu_bar_t;
	private:
		int width, label_width, hotkey_width;
		smart_label_text_t *label;
		int colnr;

	protected:
		virtual void draw_dialog(void);

	public:
		menu_panel_t(int left);
		virtual void process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		void add_item(const char *item, const char *hotkey, int id);
};

#endif
