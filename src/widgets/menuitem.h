/* Copyright (C) 2011 G.P. Halkes
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

#include <t3widget/widgets/widget.h>
#include <t3widget/dialogs/menupanel.h>

namespace t3_widget {

class T3_WIDGET_API menu_item_base_t : public widget_t {
	protected:
		menu_panel_t *parent;

	public:
		menu_item_base_t(menu_panel_t *_parent) : widget_t(1, 4), parent(_parent) {}
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
};

class T3_WIDGET_API menu_item_t : public menu_item_base_t {
	private:
		cleanup_ptr<smart_label_text_t> label;
		const char *hotkey;
		int id;
		bool has_focus;

	public:
		menu_item_t(menu_panel_t *_parent, const char *_label, const char *_hotkey, int _id);
		virtual bool process_key(key_t key);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool is_hotkey(key_t key);
		virtual bool process_mouse_event(mouse_event_t event);
		int get_label_width(void);
		int get_hotkey_width(void);
};

class T3_WIDGET_API menu_separator_t : public menu_item_base_t {
	public:
		menu_separator_t(menu_panel_t *_parent);
		virtual bool process_key(key_t key);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual bool accepts_focus(void);
};

}; // namespace
#endif
