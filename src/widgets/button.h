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
#ifndef T3_WIDGET_BUTTON_H
#define T3_WIDGET_BUTTON_H

#include "widgets/widget.h"
#include "widgets/smartlabel.h"

namespace t3_widget {

class button_t : public widget_t {
	private:
		int width;
		smart_label_text_t text;

		int text_width;
		bool is_default, has_focus;

	public:
		button_t(container_t *_parent, const char *_text, bool _isDefault = false);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		int get_width(void);
		virtual bool is_hotkey(key_t key);

	SIGNAL(activate, void);
	SIGNAL(move_focus_left, void);
	SIGNAL(move_focus_right, void);
	SIGNAL(move_focus_up, void);
	SIGNAL(move_focus_down, void);
};

}; // namespace
#endif
