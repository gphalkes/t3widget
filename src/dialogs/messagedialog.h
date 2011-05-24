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
#ifndef T3_WIDGET_MESSAGEDIALOG_H
#define T3_WIDGET_MESSAGEDIALOG_H

#include <string>
#include "dialogs/dialog.h"
#include "widgets/button.h"

namespace t3_widget {

class text_line_t;

#define _T3_WIDGET_MESSAGEDIALOG_MAX_LINES 10

class message_dialog_t : public dialog_t {
	private:
		int break_positions[_T3_WIDGET_MESSAGEDIALOG_MAX_LINES + 1];
		int height;
		text_line_t message;
		int total_width;

		virtual void draw_dialog(void);

	public:
		message_dialog_t(int width, const char *_title, ...);
		virtual bool set_size(optint height, optint width);
		void set_message(const char *_message, size_t length);
		void set_message(const char *_message);
		void set_message(const std::string *_message);

		sigc::connection connect_activate(const sigc::slot<void> &_slot, size_t idx);
};

}; // namespace
#endif

