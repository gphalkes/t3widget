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
#ifndef SMARTLABEL_H
#define SMARTLABEL_H

#include "widgets/widgets.h"
#include "window/window.h"
#include "keys.h"


class smart_label_text_t {
	protected:
		bool add_colon;
		char *text;
		size_t underline_start, underline_length, text_length;
		bool underlined;
		key_t *hotkeys;

	public:
		smart_label_text_t(const char *spec, bool _addColon = false);
		smart_label_text_t(smart_label_text_t *other);
		void draw(t3_window_t *win, int attr);
		int get_width(void);
		bool is_hotkey(key_t key);
};

class smart_label_t : public smart_label_text_t, public widget_t {
	private:
		void init(window_component_t *parent, window_component_t *anchor, int top, int left, int relation);
	public:
		smart_label_t(window_component_t *parent, int top, int left, smart_label_text_t *spec);
		smart_label_t(window_component_t *parent, int top, int left, const char *spec, bool _addColon = false);
		smart_label_t(window_component_t *parent, window_component_t *anchor,
			int top, int left, int relation, smart_label_text_t *spec);
		smart_label_t(window_component_t *parent, window_component_t *anchor,
			int top, int left, int relation, const char *spec, bool _addColon = false);
		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);

		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
};

#endif
