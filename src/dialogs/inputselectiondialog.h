/* Copyright (C) 2011 G.P. Halkes
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
#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/frame.h>
#include <t3widget/widgets/label.h>
#include <t3widget/widgets/checkbox.h>
#include <t3widget/widgets/textwindow.h>

namespace t3_widget {

class T3_WIDGET_API input_selection_dialog_t : public dialog_t {
	private:
		struct implementation_t {
			cleanup_ptr<text_buffer_t>::t text;
			frame_t *text_frame, *label_frame;
			text_window_t *text_window;
			label_t *key_label;
			checkbox_t *enable_simulate_box, *disable_timeout_box;
			int old_timeout;
		};
		pimpl_ptr<implementation_t>::t impl;

	public:
		input_selection_dialog_t(int height, int width, text_buffer_t *_text = NULL);
		virtual bool set_size(optint height, optint width);
		virtual bool process_key(key_t key);
		virtual void show(void);

		void cancel(void);
		void ok_activated(void);
		void check_state(void);

		static text_buffer_t *get_default_text(void);

	T3_WIDGET_SIGNAL(activate, void);
};

}; // namespace
#endif
