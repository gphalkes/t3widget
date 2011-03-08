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
#ifndef DIALOGS_H
#define DIALOGS_H

#include <list>
#include "main.h"
#include "interfaces.h"
#include "widgets/widgets.h"
#include "widgets/dummywidget.h"

#define DIALOG_DEPTH 80

class dialog_t;
typedef std::list<dialog_t *> dialogs_t;
class main_window_t;

class dialog_t : public window_component_t, public container_t {
	private:
		friend void iterate(void);
		friend int init(main_window_t *main_window);
		// main_window_t should be allowed to call dialog_t(), but no others should
		friend class main_window_t;

		static dialogs_t dialogs;
		static int dialog_depth;
		static dialog_t *main_window;
		static dummy_widget_t *dummy;

		static void init(main_window_t *_main_window);

		void activate_dialog(void);
		void deactivate_dialog(void);

		bool active;

		dialog_t(void);

	protected:
		t3_window_t *window;
		const char *title;
		widgets_t widgets;
		widgets_t::iterator current_widget;
		bool redraw;

		dialog_t(int height, int width, int top, int left, int depth, const char *_title);
		virtual ~dialog_t();
		virtual void draw_dialog(void);
		void focus_next(void);
		void focus_previous(void);

	public:

		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual t3_window_t *get_draw_window(void) { return window; }
};

#endif
