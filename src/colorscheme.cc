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
#include <cstring>
#include "colorscheme.h"
#include "window/window.h"

namespace t3_widget {

color_scheme_t colors;

void init_colors(void) {
	static bool initialized = false;

	if (initialized)
		return;
	initialized = true;

	memset(&colors, 0, sizeof(colors));
	//FIXME: read default parameters from config file
	colors.non_print_attrs = T3_ATTR_UNDERLINE;
	#warning default setting is only for debugging while we dont have config files
	colors.attr_selection_cursor = T3_ATTR_BG_GREEN;
	colors.attr_selection_cursor2 = T3_ATTR_FG_GREEN | T3_ATTR_BG_DEFAULT;
	colors.attr_bad_draw = T3_ATTR_BOLD;
	//~ colors.attr_cursor = T3_ATTR_BG_RED;
//~ #define DEBUG_COLORS
#ifdef DEBUG_COLORS
	colors.menubar_attrs = T3_ATTR_BG_YELLOW | T3_ATTR_FG_GREEN;
	colors.menubar_selected_attrs = T3_ATTR_BG_GREEN | T3_ATTR_FG_BLUE;
	colors.dialog_attrs = T3_ATTR_BG_WHITE | T3_ATTR_FG_BLACK;
	colors.dialog_selected_attrs = T3_ATTR_BG_GREEN | T3_ATTR_FG_RED;
	colors.textfield_attrs = T3_ATTR_BG_CYAN | T3_ATTR_FG_BLACK;
	colors.textfield_selected_attrs = T3_ATTR_BG_RED | T3_ATTR_FG_GREEN;
	colors.button_attrs = T3_ATTR_BG_BLUE | T3_ATTR_FG_GREEN;
	colors.button_selected_attrs = T3_ATTR_BG_YELLOW | T3_ATTR_FG_RED;
	colors.scrollbar_attrs = T3_ATTR_BG_BLACK | T3_ATTR_FG_WHITE;
	colors.scrollbar_selected_attrs = T3_ATTR_BG_WHITE | T3_ATTR_FG_BLACK;
	colors.text_attrs = T3_ATTR_BG_BLUE | T3_ATTR_FG_WHITE;
	colors.text_selected_attrs = T3_ATTR_BG_CYAN | T3_ATTR_FG_BLACK;
	colors.highlight_attrs = T3_ATTR_FG_WHITE | T3_ATTR_BOLD;
#else
	colors.menubar_attrs = T3_ATTR_REVERSE;
	colors.dialog_selected_attrs = T3_ATTR_REVERSE;
	colors.textfield_selected_attrs = T3_ATTR_REVERSE;
	colors.button_selected_attrs = T3_ATTR_REVERSE;
	colors.scrollbar_attrs = T3_ATTR_REVERSE;
	colors.scrollbar_selected_attrs = T3_ATTR_REVERSE;
	colors.text_selected_attrs = T3_ATTR_REVERSE;
	colors.highlight_attrs = T3_ATTR_UNDERLINE;
#endif
}

}; // namespace
