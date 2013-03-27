/* Copyright (C) 2013 G.P. Halkes
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
#ifndef T3_WIDGET_POPUP_H
#define T3_WIDGET_POPUP_H

#include <t3widget/dialogs/dialogbase.h>

namespace t3_widget {

/** Base class for dialogs. */
class T3_WIDGET_API popup_t : public dialog_base_t, public mouse_target_t {
	private:
		friend class dialog_t;

		bool draw;
		bool shown;
	protected:
		popup_t(int height, int width, bool shadow = true, bool _draw = true);

	public:
		virtual bool process_key(key_t key);
		virtual void update_contents(void);
		virtual void show(void);
		virtual void hide(void);
		virtual bool process_mouse_event(mouse_event_t event);

		bool is_shown(void);
};

} // namespace

#endif
