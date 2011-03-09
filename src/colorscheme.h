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
#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <window/window.h>

namespace t3_widget {

typedef struct {
	t3_attr_t non_print_attrs;
	t3_attr_t attr_selection_cursor;
	t3_attr_t attr_selection_cursor2;
	t3_attr_t attr_bad_draw;
	t3_attr_t attr_cursor;

	t3_attr_t dialog_attrs;
	t3_attr_t dialog_selected_attrs;
	t3_attr_t textfield_attrs;
	t3_attr_t textfield_selected_attrs;
	t3_attr_t button_attrs;
	t3_attr_t button_selected_attrs;
	t3_attr_t scrollbar_attrs;
	t3_attr_t scrollbar_selected_attrs;
	t3_attr_t text_attrs;
	t3_attr_t text_selected_attrs;
	t3_attr_t highlight_attrs;
	t3_attr_t menubar_attrs;
	t3_attr_t menubar_selected_attrs;
} color_scheme_t;

extern color_scheme_t colors;
void init_colors(void);

}; // namespace

#endif
