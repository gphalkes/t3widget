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

#include "textwindow.h"
#include "util.h"
#include "colorscheme.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

text_window_t::text_window_t(text_buffer_t *_text) : text_buffer_window_t(11, 11), scrollbar(true) {
	/* Note: we use a dirty trick here: the last position of the window is obscured by
	   the scrollbar. However, the last position will only contain the wrap character
	   anyway, so we don't care. */
	set_widget_parent(&scrollbar);
	scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	scrollbar.set_size(11, None);

	if (_text == NULL)
		text = new text_buffer_t();
	else
		text = _text;

	text->window = this;
	text->set_wrap(true);
	text->rewrap();
}

void text_window_t::set_text(text_buffer_t *_text) {
	if (text == _text)
		return;

	text->window = NULL;
	text = _text;
	text->window = this;
	text->set_wrap(true);
	text->rewrap();
	redraw = true;
}

bool text_window_t::set_size(optint height, optint width) {
	bool result = true;
	if (width != t3_win_get_width(window) || height > t3_win_get_height(window))
		redraw = true;

	result &= t3_win_resize(window, height, width);
	result &= scrollbar.set_size(height, None);

	text->rewrap();
	return result;
}

void text_window_t::inc_y(void) {
	if (text->topleft.line + t3_win_get_height(window) < text->get_used_lines()) {
		text->topleft.line++;
		redraw = true;
	}
}

void text_window_t::dec_y(void) {
	if (text->topleft.line > 0) {
		text->topleft.line--;
		redraw = true;
	}
}

void text_window_t::pgdn(void) {
	/* If the end of the text is already on the screen, don't change the top line. */
	if (text->topleft.line + 2 * t3_win_get_height(window) - 1 < text->get_used_lines()) {
		text->topleft.line += t3_win_get_height(window) - 1;
		redraw = true;
	} else if (text->topleft.line + t3_win_get_height(window) < text->get_used_lines()) {
		text->topleft.line = text->get_used_lines() - t3_win_get_height(window);
		if (text->topleft.line < 0)
			text->topleft.line = 0;
		redraw = true;
	}
}

void text_window_t::pgup(void) {
	if (text->topleft.line < t3_win_get_height(window) - 1) {
		if (text->topleft.line != 0) {
			redraw = true;
			text->topleft.line = 0;
		}
	} else {
		text->topleft.line -= t3_win_get_height(window) - 1;
		redraw = true;
	}
}

bool text_window_t::process_key(key_t key) {
	switch (key) {
		case EKEY_DOWN:
		case EKEY_NL:
			inc_y();
			break;
		case EKEY_UP:
			dec_y();
			break;
		case EKEY_PGUP:
			pgup();
			break;
		case EKEY_PGDN:
		case ' ':
			pgdn();
			break;
		case EKEY_HOME | EKEY_CTRL:
		case EKEY_HOME:
			if (text->topleft.line != 0) {
				text->topleft.line = 0;
				redraw = true;
			}
			break;
		case EKEY_END | EKEY_CTRL:
		case EKEY_END:
			if (text->topleft.line + t3_win_get_height(window) < text->get_used_lines()) {
				text->topleft.line = text->get_used_lines() - t3_win_get_height(window);
				if (text->topleft.line < 0)
					text->topleft.line = 0;
				redraw = true;
			}
			break;
		default:
			return false;
	}
	return true;
}

void text_window_t::update_contents(void) {
	text_coordinate_t current_start, current_end;
	text_line_t::paint_info_t info;
	int i;

	if (!redraw)
		return;

	redraw = false;
	t3_win_set_default_attrs(window, attributes.dialog);

	info.leftcol = text->topleft.pos;
	info.size = t3_win_get_width(window);
	info.normal_attr = 0;
	info.selected_attr = 0; /* There is no selected text anyway. */
	info.selection_start = -1;
	info.selection_end = -1;
	info.cursor = -1;
	for (i = 0; i < t3_win_get_height(window) && (i + text->topleft.line) < text->get_used_lines(); i++) {
		t3_win_set_paint(window, i, 0);
		t3_win_clrtoeol(window);
		text->paint_line(window, text->topleft.line + i, &info);
	}

	t3_win_clrtobot(window);

	scrollbar.set_parameters(max(text->get_used_lines(), text->topleft.line + t3_win_get_height(window)),
		text->topleft.line, t3_win_get_height(window));
	scrollbar.update_contents();
}

int text_window_t::get_text_width(void) {
	return t3_win_get_width(window);
}

text_buffer_t *text_window_t::get_text(void) {
	return text;
}

}; // namespace
