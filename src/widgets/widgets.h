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
#ifndef WIDGETS_H
#define WIDGETS_H

#include <deque>
#include <vector>
using namespace std;

#include "window/window.h"
#include "interfaces.h"
#include "colorscheme.h"

class base_widget_t : public window_component_t {
	protected:
		bool shown;

	public:
		base_widget_t(void) : shown(true) {}
		virtual ~base_widget_t(void);
		virtual bool is_hotkey(key_t key);
		virtual bool accepts_focus(void);
};


class widget_t : public base_widget_t {
	protected:
		t3_window_t *window;
	public:
		widget_t(void);
		virtual ~widget_t(void);
		virtual t3_window_t *get_draw_window(void) { return window; }
		virtual void set_show(bool show);
};

typedef deque<base_widget_t *> widgets_t;

#include "widgets/smartlabel.h"
#include "widgets/textfield.h"
#include "widgets/filepane.h"
#include "widgets/button.h"
#include "widgets/checkbox.h"
#include "widgets/dummywidget.h"
#include "widgets/menu.h"
#include "widgets/scrollbar.h"
#include "widgets/listpane.h"
#include "widgets/label.h"
#include "widgets/multiwidget.h"
#include "widgets/bullet.h"
#endif
