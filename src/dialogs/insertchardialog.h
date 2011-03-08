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
#ifndef T3_WIDGET_INSERTCHARDIALOG_H
#define T3_WIDGET_INSERTCHARDIALOG_H

#include "dialogs/dialogs.h"

class insert_char_dialog_t : public dialog_t {
	private:
		text_field_t *description_line;
		key_t interpret_key(const string *descr);
	public:
		insert_char_dialog_t(void);
		virtual bool resize(optint height, optint width, optint top, optint left);
		virtual void set_show(bool show);
		virtual void callback(int action, const void *data = NULL);
};

#endif
