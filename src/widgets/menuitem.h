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
#ifndef MENUITEM_H
#define MENUITEM_H

#include "widgets/widgets.h"

class menu_item_base_t : public base_widget_t {
	protected:
		t3_window_t *parent;
		int top, width;

	public:
		menu_item_base_t(t3_window_t *_parent, int _top) : parent(_parent), top(_top) {}
		virtual t3_window_t *get_draw_window(void) { return parent; }
};

class menu_item_t : public menu_item_base_t {
	private:
		smart_label_text_t label;
		const char *hotkey;
		int id;
		bool has_focus;

	public:
		menu_item_t(t3_window_t *_parent, const char *_label, const char *_hotkey, int _top, int _id);
		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint _width, optint _top, optint left);
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
		menu_separator_t(t3_window_t *_parent, int _top, int _width);
		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint _width, optint _top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool accepts_focus(void);
};

#endif
