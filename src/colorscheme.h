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

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <window/window.h>
#include "util.h"
namespace t3_widget {

typedef struct {
	/* Text related attributes. */
	t3_attr_t non_print;
	t3_attr_t selection_cursor;
	t3_attr_t selection_cursor2;
	t3_attr_t bad_draw;
	t3_attr_t text_cursor;
	t3_attr_t text;
	t3_attr_t text_selected;
	/* High-light attributes for hot keys. */
	t3_attr_t highlight;
	t3_attr_t highlight_selected;

	t3_attr_t dialog;
	t3_attr_t dialog_selected;
	t3_attr_t button;
	t3_attr_t button_selected;
	t3_attr_t scrollbar;
	t3_attr_t scrollbar_selected;
	t3_attr_t menubar;
	t3_attr_t menubar_selected;

	t3_attr_t shadow;
} attributes_t;

extern attributes_t attributes;
void init_colors(void);

}; // namespace

#endif
