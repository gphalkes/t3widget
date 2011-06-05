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
#ifndef T3_WIDGET_INPUTSELECTIONDIALOG_H
#define T3_WIDGET_INPUTSELECTIONDIALOG_H

#include <string>
#include "dialogs/dialog.h"
#include "widgets/frame.h"
#include "widgets/label.h"
#include "widgets/button.h"
#include "widgets/textwindow.h"

namespace t3_widget {

class input_selection_dialog_t : public dialog_t {
	private:
		frame_t *text_frame, *label_frame;
		text_window_t *text_window;
		label_t *key_label;
		button_t *conservative_button, *timeout_button;
	public:
		input_selection_dialog_t(int height, int width);
		virtual bool set_size(optint height, optint width);
		virtual bool process_key(key_t key);

	T3_WIDGET_SIGNAL(conservative_activated, void);
	T3_WIDGET_SIGNAL(timeout_activated, void);
};

}; // namespace
#endif
