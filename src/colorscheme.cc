/* Copyright (C) 2011-2012 G.P. Halkes
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

static t3_attr_t ensure_color(t3_attr_t value) {
	if ((value & T3_ATTR_FG_MASK) == 0)
		value |= T3_ATTR_FG_DEFAULT;
	if ((value & T3_ATTR_BG_MASK) == 0)
		value |= T3_ATTR_BG_DEFAULT;
	return value;
}

bool set_color_mode(bool on) {
	bool result = true;
	/* Only actually switch to color mode if the terminal supports (sufficient) color. */
	if (on) {
		t3_term_caps_t terminal_caps;
		t3_term_get_caps(&terminal_caps);
		on = terminal_caps.colors >= 8;
		result = on;
	}

	attributes.non_print = get_default_attribute(attribute_t::NON_PRINT, on);
	attributes.text_selection_cursor = get_default_attribute(attribute_t::TEXT_SELECTION_CURSOR, on);
	attributes.text_selection_cursor2 = get_default_attribute(attribute_t::TEXT_SELECTION_CURSOR2, on);
	attributes.bad_draw = get_default_attribute(attribute_t::BAD_DRAW, on);
	attributes.text_cursor = get_default_attribute(attribute_t::TEXT_CURSOR, on);
	attributes.text = get_default_attribute(attribute_t::TEXT, on);
	attributes.text_selected = get_default_attribute(attribute_t::TEXT_SELECTED, on);
	attributes.hotkey_highlight = get_default_attribute(attribute_t::HOTKEY_HIGHLIGHT, on);
	attributes.dialog = get_default_attribute(attribute_t::DIALOG, on);
	attributes.dialog_selected = get_default_attribute(attribute_t::DIALOG_SELECTED, on);
	attributes.scrollbar = get_default_attribute(attribute_t::SCROLLBAR, on);
	attributes.menubar = get_default_attribute(attribute_t::MENUBAR, on);
	attributes.menubar_selected = get_default_attribute(attribute_t::MENUBAR_SELECTED, on);
	attributes.shadow = get_default_attribute(attribute_t::SHADOW, on);
	attributes.meta_text = get_default_attribute(attribute_t::META_TEXT, on);
	attributes.background = get_default_attribute(attribute_t::BACKGROUND, on);

	t3_win_set_default_attrs(NULL, attributes.background);
	dialog_t::force_redraw_all();
	return result;
}

void set_attribute(attribute_t attribute, t3_attr_t value) {
	switch (attribute) {
		case attribute_t::NON_PRINT:
			attributes.non_print = value;
			break;
		case attribute_t::TEXT_SELECTION_CURSOR:
			attributes.text_selection_cursor = value;
			break;
		case attribute_t::TEXT_SELECTION_CURSOR2:
			attributes.text_selection_cursor2 = value;
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
		case attribute_t::HOTKEY_HIGHLIGHT:
			attributes.hotkey_highlight = value;
			break;
		case attribute_t::DIALOG:
			attributes.dialog = value;
			break;
		case attribute_t::DIALOG_SELECTED:
			attributes.dialog_selected = value;
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
		case attribute_t::META_TEXT:
			attributes.meta_text = value;
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
		case attribute_t::TEXT_SELECTION_CURSOR:
			return attributes.text_selection_cursor;
		case attribute_t::TEXT_SELECTION_CURSOR2:
			return attributes.text_selection_cursor2;
		case attribute_t::BAD_DRAW:
			return attributes.bad_draw;
		case attribute_t::TEXT_CURSOR:
			return attributes.text_cursor;
		case attribute_t::TEXT:
			return attributes.text;
		case attribute_t::TEXT_SELECTED:
			return attributes.text_selected;
		case attribute_t::HOTKEY_HIGHLIGHT:
			return attributes.hotkey_highlight;
		case attribute_t::DIALOG:
			return attributes.dialog;
		case attribute_t::DIALOG_SELECTED:
			return attributes.dialog_selected;
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
		case attribute_t::META_TEXT:
			return attributes.meta_text;
		default:
			return 0;
	}
}

t3_attr_t get_default_attribute(attribute_t attribute, bool color_mode) {
	/* The attributes fall into two categories:
	   - full attributes
	   - highlight attributes
	   Highlight attributes are partial attributes which will be added to
	   other attributes, while the full attributes define the complete rendering.
	   In full attributes, the color should not be left unspecified, at least
	   not in the default setting.
	*/
	switch (attribute) {
		case attribute_t::NON_PRINT:
			return T3_ATTR_UNDERLINE;
		case attribute_t::TEXT_SELECTION_CURSOR:
			return color_mode ? T3_ATTR_FG_BLUE | T3_ATTR_BG_CYAN : T3_ATTR_UNDERLINE | T3_ATTR_BLINK;
		case attribute_t::TEXT_SELECTION_CURSOR2:
			return color_mode ? T3_ATTR_FG_BLUE | T3_ATTR_BG_GREEN : T3_ATTR_UNDERLINE | T3_ATTR_REVERSE | T3_ATTR_BLINK;
		case attribute_t::BAD_DRAW:
			return color_mode ? T3_ATTR_FG_RED : T3_ATTR_BOLD;
		case attribute_t::TEXT_CURSOR:
			return color_mode ? T3_ATTR_FG_BLUE | T3_ATTR_BG_CYAN :  T3_ATTR_REVERSE;
		case attribute_t::TEXT:
			return ensure_color(color_mode ? T3_ATTR_FG_WHITE | T3_ATTR_BG_BLUE : 0);
		case attribute_t::TEXT_SELECTED:
			return ensure_color(color_mode ? T3_ATTR_FG_BLUE | T3_ATTR_BG_WHITE : T3_ATTR_REVERSE);
		case attribute_t::HOTKEY_HIGHLIGHT:
			return color_mode ? T3_ATTR_FG_BLUE : T3_ATTR_UNDERLINE;
		case attribute_t::DIALOG:
			return ensure_color(color_mode ? T3_ATTR_FG_BLACK | T3_ATTR_BG_WHITE : 0);
		case attribute_t::DIALOG_SELECTED:
			return ensure_color(color_mode ? T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK : T3_ATTR_REVERSE);
		case attribute_t::SCROLLBAR:
			return ensure_color(color_mode ? T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK : T3_ATTR_REVERSE);
		case attribute_t::MENUBAR:
			return ensure_color(color_mode ? T3_ATTR_FG_BLACK | T3_ATTR_BG_CYAN : T3_ATTR_REVERSE);
		case attribute_t::MENUBAR_SELECTED:
			return ensure_color(color_mode ? T3_ATTR_FG_WHITE | T3_ATTR_BG_BLACK : 0);
		case attribute_t::BACKGROUND:
			return 0;
		case attribute_t::SHADOW:
			return ensure_color(color_mode ? T3_ATTR_BG_BLACK : T3_ATTR_REVERSE);
		case attribute_t::META_TEXT:
			return color_mode ? T3_ATTR_FG_CYAN : T3_ATTR_UNDERLINE;
		default:
			return 0;
	}
}

}; // namespace
