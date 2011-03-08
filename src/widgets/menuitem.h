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
#ifndef T3_WIDGET_MENUITEM_H
#define T3_WIDGET_MENUITEM_H

#include "widgets/widget.h"
#include "dialogs/menupanel.h"

namespace t3_widget {

class menu_item_base_t : public base_widget_t {
	protected:
		menu_panel_t *parent;
		int top;

	public:
		menu_item_base_t(menu_panel_t *_parent) : parent(_parent), top(0) {}
		virtual t3_window_t *get_draw_window(void) { return parent->get_draw_window(); }
};

class menu_item_t : public menu_item_base_t {
	private:
		smart_label_text_t label;
		const char *hotkey;
		int width, id;
		bool has_focus;

	public:
		menu_item_t(menu_panel_t *_parent, const char *_label, const char *_hotkey, int _id);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool is_hotkey(key_t key);
		int get_label_width(void);
		int get_hotkey_width(void);
};

class menu_separator_t : public menu_item_base_t {
	public:
		menu_separator_t(menu_panel_t *_parent);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool accepts_focus(void);
};

}; // namespace
#endif
