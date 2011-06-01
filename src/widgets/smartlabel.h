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
#ifndef T3_WIDGET_SMARTLABEL_H
#define T3_WIDGET_SMARTLABEL_H

#include "widgets/widget.h"
#include "key.h"

namespace t3_widget {

class smart_label_text_t {
	protected:
		bool add_colon;
		char *text;
		size_t underline_start, underline_length, text_length;
		bool underlined;
		key_t hotkey;

	public:
		smart_label_text_t(const char *spec, bool _addColon = false);
		smart_label_text_t(smart_label_text_t *other);
		~smart_label_text_t(void);
		void draw(t3_window_t *win, int attr, bool selected = false);
		int get_width(void);
		bool is_hotkey(key_t key);
};

class smart_label_t : public smart_label_text_t, public widget_t {
	public:
		smart_label_t(smart_label_text_t *spec);
		smart_label_t(const char *spec, bool _addColon = false);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);

		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
};

}; // namespace
#endif
