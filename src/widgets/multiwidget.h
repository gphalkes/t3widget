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
#ifndef T3_WIDGET_MULTIWIDGET_H
#define T3_WIDGET_MULTIWIDGET_H

#include "widgets/widget.h"

namespace t3_widget {

class multi_widget_t : public widget_t, public focus_widget_t, public container_t {
	private:
		struct item_t {
			widget_t *widget;
			int width;
			int calculated_width;
			bool takes_focus;
		};
		std::list<item_t> widgets;
		int width, fixed_sum, proportion_sum;
		widget_t *send_key_widget;

	public:
		multi_widget_t(container_t *parent);
		virtual ~multi_widget_t(void);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual bool accepts_focus(void);
		virtual void force_redraw(void);
		virtual void set_enabled(bool enable);

		/* Width is negative for fixed width widgets, positive for proportion */
		void push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys);
		void resize_widgets(void);
};

}; // namespace
#endif
