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
#ifndef T3_WIDGET_INTERFACES_H
#define T3_WIDGET_INTERFACES_H

#include <list>
#include <t3window/window.h>

#include <t3widget/key.h>
#include <t3widget/util.h>

namespace t3_widget {

class T3_WIDGET_API window_component_t {
	protected:
		t3_window_t *window;
	public:
		window_component_t(void);
		/* Virtual destructor is required for proper functioning of the delete
		   operator in multiple-inheritance situations. */
		virtual ~window_component_t(void);
		virtual t3_window_t *get_base_window(void);
		virtual bool process_key(key_t key) = 0;
		virtual void set_position(optint top, optint left) = 0;
		virtual bool set_size(optint height, optint width) = 0;
		virtual void update_contents(void) = 0;
		virtual void set_focus(bool focus) = 0;
		virtual void show(void) = 0;
		virtual void hide(void) = 0;
};

class widget_t;
class T3_WIDGET_API container_t : private virtual window_component_t {
	protected:
		virtual bool set_widget_parent(widget_t *widget);
		virtual void unset_widget_parent(widget_t *widget);
};

class T3_WIDGET_API center_component_t : private virtual window_component_t {
	protected:
		window_component_t *center_window;
	public:
		center_component_t(void);
		virtual void set_center_window(window_component_t *_center_window);
};

class T3_WIDGET_API bad_draw_recheck_t {
	private:
		static std::list<bad_draw_recheck_t *> to_signal;
		static sigc::connection initialized;
		static void bad_draw_recheck_all(void);

	public:
		bad_draw_recheck_t(void);
		virtual ~bad_draw_recheck_t(void);

		virtual void bad_draw_recheck(void) = 0;
};

}; // namespace
#endif
