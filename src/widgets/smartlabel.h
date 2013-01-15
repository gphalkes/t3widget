/* Copyright (C) 2011-2012 G.P. Halkes
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

#include <t3widget/widgets/widget.h>
#include <t3widget/key.h>

namespace t3_widget {

class T3_WIDGET_API smart_label_text_t {
	protected:
		bool add_colon;
		cleanup_free_ptr<char>::t text;
		size_t underline_start, underline_length, text_length;
		bool underlined;
		key_t hotkey;

	public:
		smart_label_text_t(const char *spec, bool _addColon = false);
		virtual ~smart_label_text_t(void);
		void draw(t3_window_t *win, int attr, bool selected = false);
		int get_width(void);
		bool is_hotkey(key_t key);
};

class T3_WIDGET_API smart_label_t : public smart_label_text_t, public widget_t {
	public:
		smart_label_t(const char *spec, bool _addColon = false);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(focus_t focus);

		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
};

}; // namespace
#endif
