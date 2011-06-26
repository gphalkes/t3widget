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
#ifndef T3_WIDGET_GOTODIALOG_H
#define T3_WIDGET_GOTODIALOG_H

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/textfield.h>

namespace t3_widget {

class T3_WIDGET_API goto_dialog_t : public dialog_t {
	private:
		text_field_t *number_line;
	public:
		goto_dialog_t(void);
		virtual bool set_size(optint height, optint width);
		void reset(void);

		void ok_activate(void);

	T3_WIDGET_SIGNAL(activate, void, int);
};

}; // namespace
#endif
