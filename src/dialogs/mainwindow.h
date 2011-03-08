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
#ifndef T3_WIDGET_MAINWINDOW_H
#define T3_WIDGET_MAINWINDOW_H

#include "interfaces.h"
#include "dialogs/dialog.h"

namespace t3_widget {

class main_window_t : public dialog_t {
	private:
		void set_size_real(int height, int width);
	protected:
		main_window_t(void);

	public:
		virtual bool set_size(optint height, optint width);
		virtual void set_position(optint top, optint left);
		virtual void hide(void) {}
};

}; // namespace
#endif
