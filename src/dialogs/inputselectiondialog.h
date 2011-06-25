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

class T3_WIDGET_API input_selection_dialog_t : public dialog_t {
	private:
		text_buffer_t *text;
		frame_t *text_frame, *label_frame;
		text_window_t *text_window;
		label_t *key_label;
		button_t *intuitive_button, *compromise_button, *no_timeout_button;
	public:
		input_selection_dialog_t(int height, int width, text_buffer_t *_text = NULL);
		~input_selection_dialog_t(void);
		virtual bool set_size(optint height, optint width);
		virtual bool process_key(key_t key);

		static text_buffer_t *get_default_text(void);

	T3_WIDGET_SIGNAL(intuitive_activated, void);
	T3_WIDGET_SIGNAL(compromise_activated, void);
	T3_WIDGET_SIGNAL(no_timeout_activated, void);
};

}; // namespace
#endif
