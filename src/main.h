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
#ifndef T3_WIDGET_MAIN_H
#define T3_WIDGET_MAIN_H

#include "util.h"
#include "dialogs/dialog.h"
#include "dialogs/insertchardialog.h"

namespace t3_widget {
class main_window_base_t;

extern insert_char_dialog_t insert_char_dialog;

sigc::connection connect_resize(const sigc::slot<void, int, int> &_slot);

int init(main_window_base_t *main_window);
void iterate(void);
void main_loop(void);

}; // namespace

#endif
