/* Copyright (C) 2011 G.P. Halkes
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
#include <t3window/window.h>
#include "colorscheme.h"
#include "dialogs/dialog.h"
#include "main.h"

namespace t3_widget {

attributes_t attributes;

void init_attributes(void) {
	memset(&attributes, 0, sizeof(attributes));
	set_color_mode(true);
}

void set_color_mode(bool on) {
	/* Only actually switch to color mode if the terminal supports (sufficient) color. */
	if (on) {
		t3_term_caps_t terminal_caps;
		t3_term_get_caps(&terminal_caps);
		on = terminal_caps.colors >= 8;
	}

	if (on) {
		attributes.non_print = T3_ATTR_UNDERLINE;
		attributes.selection_cursor = T3_ATTR_FG_BLUE | T3_ATTR_BG_CYAN;
		attributes.selection_cursor2 = T3_ATTR_FG_BLUE | T3_ATTR_BG_GREEN;
		attributes.bad_draw = T3_ATTR_FG_RED;
		attributes.text_cursor = T3_ATTR_FG_BLUE | T3_ATTR_BG_CYAN;
		attributes.text = T3_ATTR_FG_WHITE | T3_ATTR_BG_BLUE;
		attributes.text_selected = T3_ATTR_FG_BLUE | T3_ATTR_BG_WHITE;
		attributes.highlight = T3_ATTR_FG_BLUE;
		attributes.highlight_selected = T3_ATTR_FG_YELLOW;
		attributes.dialog = T3_ATTR_FG_BLACK | T3_ATTR_BG_WHITE;
		attributes.dialog_selected = T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK;
		attributes.button = attributes.dialog;
		attributes.button_selected = T3_ATTR_FG_BLACK | T3_ATTR_BG_CYAN;
		attributes.scrollbar = T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK;
		attributes.menubar = T3_ATTR_FG_BLACK | T3_ATTR_BG_CYAN;
		attributes.menubar_selected = T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK;
		attributes.shadow = T3_ATTR_BG_BLACK;
	} else {
		attributes.non_print = T3_ATTR_UNDERLINE;
		attributes.selection_cursor = T3_ATTR_UNDERLINE | T3_ATTR_BLINK;
		attributes.selection_cursor2 = T3_ATTR_UNDERLINE | T3_ATTR_REVERSE | T3_ATTR_BLINK;
		attributes.bad_draw = T3_ATTR_BOLD;
		attributes.text_cursor = T3_ATTR_REVERSE;
		attributes.text = 0;
		attributes.text_selected = T3_ATTR_REVERSE;
		attributes.highlight = T3_ATTR_UNDERLINE;
		attributes.highlight_selected = 0;
		attributes.dialog = 0;
		attributes.dialog_selected = T3_ATTR_REVERSE;
		attributes.button = 0;
		attributes.button_selected = T3_ATTR_REVERSE;
		attributes.scrollbar = T3_ATTR_REVERSE;
		attributes.menubar = T3_ATTR_REVERSE;
		attributes.menubar_selected = 0;
		attributes.shadow = T3_ATTR_REVERSE;
	}
	attributes.background = attributes.dialog;
	t3_win_set_default_attrs(NULL, attributes.background);
	dialog_t::force_redraw_all();
}

void set_attribute(attribute_t attribute, t3_attr_t value) {
	switch (attribute) {
		case attribute_t::NON_PRINT:
			attributes.non_print = value;
			break;
		case attribute_t::SELECTION_CURSOR:
			attributes.selection_cursor = value;
			break;
		case attribute_t::SELECTION_CURSOR2:
			attributes.selection_cursor2 = value;
			break;
		case attribute_t::BAD_DRAW:
			attributes.bad_draw = value;
			break;
		case attribute_t::TEXT_CURSOR:
			attributes.text_cursor = value;
			break;
		case attribute_t::TEXT:
			attributes.text = value;
			break;
		case attribute_t::TEXT_SELECTED:
			attributes.text_selected = value;
			break;
		case attribute_t::HIGHLIGHT:
			attributes.highlight = value;
			break;
		case attribute_t::HIGHLIGHT_SELECTED:
			attributes.highlight_selected = value;
			break;
		case attribute_t::DIALOG:
			attributes.dialog = value;
			break;
		case attribute_t::DIALOG_SELECTED:
			attributes.dialog_selected = value;
			break;
		case attribute_t::BUTTON:
			attributes.button = value;
			break;
		case attribute_t::BUTTON_SELECTED:
			attributes.button_selected = value;
			break;
		case attribute_t::SCROLLBAR:
			attributes.scrollbar = value;
			break;
		case attribute_t::MENUBAR:
			attributes.menubar = value;
			break;
		case attribute_t::MENUBAR_SELECTED:
			attributes.menubar_selected = value;
			break;
		case attribute_t::BACKGROUND:
			attributes.background = value;
			t3_win_set_default_attrs(NULL, value);
			break;
		case attribute_t::SHADOW:
			attributes.shadow = value;
			break;
		default:
			return;
	}

	dialog_t::force_redraw_all();
}

t3_attr_t get_attribute(attribute_t attribute) {
	switch (attribute) {
		case attribute_t::NON_PRINT:
			return attributes.non_print;
		case attribute_t::SELECTION_CURSOR:
			return attributes.selection_cursor;
		case attribute_t::SELECTION_CURSOR2:
			return attributes.selection_cursor2;
		case attribute_t::BAD_DRAW:
			return attributes.bad_draw;
		case attribute_t::TEXT_CURSOR:
			return attributes.text_cursor;
		case attribute_t::TEXT:
			return attributes.text;
		case attribute_t::TEXT_SELECTED:
			return attributes.text_selected;
		case attribute_t::HIGHLIGHT:
			return attributes.highlight;
		case attribute_t::HIGHLIGHT_SELECTED:
			return attributes.highlight_selected;
		case attribute_t::DIALOG:
			return attributes.dialog;
		case attribute_t::DIALOG_SELECTED:
			return attributes.dialog_selected;
		case attribute_t::BUTTON:
			return attributes.button;
		case attribute_t::BUTTON_SELECTED:
			return attributes.button_selected;
		case attribute_t::SCROLLBAR:
			return attributes.scrollbar;
		case attribute_t::MENUBAR:
			return attributes.menubar;
		case attribute_t::MENUBAR_SELECTED:
			return attributes.menubar_selected;
		case attribute_t::BACKGROUND:
			return attributes.background;
		case attribute_t::SHADOW:
			return attributes.shadow;
		default:
			return 0;
	}
}

}; // namespace
