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
#ifndef T3_WIDGET_ALTMESSAGEDIALOG_H
#define T3_WIDGET_ALTMESSAGEDIALOG_H

#include "dialogs/dialogs.h"

namespace t3_widget {

class alt_message_dialog_t : public dialog_t {
	private:
		const char *message;
		virtual void draw_dialog(void);

	public:
		alt_message_dialog_t(const char *_message);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void process_key(key_t key);
};

}; // namespace
#endif
