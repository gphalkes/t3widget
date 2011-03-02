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

#include "main.h"
#include "interfaces.h"
#include "widgets/widgets.h"
#include "dialogs/mainwindow.h"

#define DIALOG_DEPTH 80

class dialog_t : public window_component_t {
	private:
		friend void ::iterate(void);
		static window_components_t dialogs;
		static int dialog_depth;

		void activate_dialog(void);
		void deactivate_dialog(void);

		bool active;

	protected:
		t3_window_t *window;
		const char *title;
		widgets_t widgets;
		widgets_t::iterator current_widget;

		dialog_t(int height, int width, int top, int left, int depth, const char *_title);
		virtual ~dialog_t();
		virtual void draw_dialog(void);
		void focus_next(void);
		void focus_previous(void);

	public:
		static window_component_t *main_window;
		static void init(void);

		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void set_show(bool show);
		virtual t3_window_t *get_draw_window(void) { return window; }
};

//FIXME: enable the proper includes again
//~ #include "dialogs/filedialog.h"
#include "dialogs/messagedialog.h"
//~ #include "dialogs/finddialog.h"
//~ #include "dialogs/insertchardialog.h"
//~ #include "dialogs/closeconfirmdialog.h"
//~ #include "dialogs/altmessagedialog.h"
#include "dialogs/menupanel.h"
//~ #include "dialogs/gotodialog.h"
//~ #include "dialogs/overwriteconfirmdialog.h"
//~ #include "dialogs/openrecentdialog.h"
//~ #include "dialogs/selectbufferdialog.h"
//#include "dialogs/encodingdialog.h"
#endif
