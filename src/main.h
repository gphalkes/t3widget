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

#include "dialogs/dialogs.h"

namespace t3_widget {
class main_window_t;

class text_line_t;
//FIXME: remove anything not related to the external interface
extern text_line_t *copy_buffer;

sigc::connection connect_resize(const sigc::slot<void, int, int> &_slot);

int init(main_window_t *main_window);
void iterate(void);
void main_loop(void);

#ifdef _T3_WIDGET_DEBUG
#define ASSERT(_x) do { if (!(_x)) { fprintf(stderr, "%s:%d: libt3widget: Assertion failed: %s\n", __FILE__, __LINE__, #_x); abort(); }} while (0)
#else
#define ASSERT(_x)
#endif

}; // namespace

#endif
