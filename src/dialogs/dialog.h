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
#ifndef T3_WIDGET_DIALOGS_H
#define T3_WIDGET_DIALOGS_H

#include <list>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/dummywidget.h>

namespace t3_widget {

class dialog_t;
typedef std::list<dialog_t *> dialogs_t;
class complex_error_t;

class T3_WIDGET_API dialog_t : public virtual window_component_t, public container_t {
	private:
		friend void iterate(void);
		// main_window_base_t should be allowed to call dialog_t(), but no others should
		friend class main_window_base_t;

		static dialogs_t active_dialogs;
		static dialogs_t dialogs;
		static int dialog_depth;
		static dummy_widget_t *dummy;

		static void init(void);
		static bool init_connected;

		t3_window_t *shadow_window;

		void activate_dialog(void);
		void deactivate_dialog(void);

		bool active;

		dialog_t(void);

	protected:
		const char *title;
		widgets_t widgets;
		widgets_t::iterator current_widget;
		bool redraw;

		dialog_t(int height, int width, const char *_title);
		virtual ~dialog_t();
		virtual void draw_dialog(void);
		void focus_next(void);
		void focus_previous(void);
		void focus_set(widget_t *target);
		void push_back(widget_t *widget);
		virtual void close(void);

	public:
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual void force_redraw(void);
		virtual void center_over(window_component_t *center);

		static void force_redraw_all(void);

	T3_WIDGET_SIGNAL(closed, void);
};

}; // namespace
#endif
