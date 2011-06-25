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
#ifndef T3_WIDGET_CHECKBOX_H
#define T3_WIDGET_CHECKBOX_H

#include "widgets/widget.h"
#include "widgets/smartlabel.h"

namespace t3_widget {

class T3_WIDGET_API checkbox_t : public widget_t, public focus_widget_t {
	private:
		bool state, has_focus;
		smart_label_t *label;

	public:
		checkbox_t(bool _state = false);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		bool get_state(void);
		void set_state(bool _state);
		void set_label(smart_label_t *_label);
		virtual bool is_hotkey(key_t key);
		virtual void set_enabled(bool enable);

	T3_WIDGET_SIGNAL(activate, void);
	T3_WIDGET_SIGNAL(toggled, void);
};

}; // namespace
#endif
