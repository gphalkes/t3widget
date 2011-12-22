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

#include "textwindow.h"
#include "util.h"
#include "colorscheme.h"
#include "internal.h"
#include "wrapinfo.h"

using namespace std;
namespace t3_widget {

text_window_t::text_window_t(text_buffer_t *_text, bool with_scrollbar) : widget_t(11, 11), scrollbar(NULL), top(0, 0), focus(false) {
	cleanup_ptr<wrap_info_t> tmp_wrap_info;
	/* Note: we use a dirty trick here: the last position of the window is obscured by
	   the scrollbar-> However, the last position will only contain the wrap character
	   anyway, so we don't care. If the scrollbar is disabled, we set the wrap width
	   one higher, such that the wrap character falls off the edge. */
	if (with_scrollbar) {
		scrollbar = new scrollbar_t(true);
		set_widget_parent(scrollbar);
		scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
		scrollbar->set_size(11, None);
	}

	if (_text == NULL)
		text = new text_buffer_t();
	else
		text = _text;

	tmp_wrap_info = new wrap_info_t(scrollbar != NULL ? 11 : 12);
	tmp_wrap_info->set_text_buffer(text);
	wrap_info = tmp_wrap_info;
	tmp_wrap_info = NULL;
}

text_window_t::~text_window_t(void) {
	delete wrap_info;
}

void text_window_t::set_text(text_buffer_t *_text) {
	if (text == _text)
		return;

	text = _text;
	wrap_info->set_text_buffer(text);
	top.line = 0;
	top.pos = 0;
	redraw = true;
}

bool text_window_t::set_size(optint height, optint width) {
	bool result = true;

	if (!width.is_valid())
		width = t3_win_get_width(window);
	if (!height.is_valid())
		height = t3_win_get_height(window);

	if (width != t3_win_get_width(window) || height > t3_win_get_height(window))
		redraw = true;

	result &= t3_win_resize(window, height, width);
	if (scrollbar != NULL) {
		result &= scrollbar->set_size(height, None);
		wrap_info->set_wrap_width(width);
	} else {
		wrap_info->set_wrap_width(width + 1);
	}

	return result;
}

void text_window_t::set_scrollbar(bool with_scrollbar) {
	if (with_scrollbar == (scrollbar != NULL))
		return;
	if (with_scrollbar) {
		scrollbar = new scrollbar_t(true);
		set_widget_parent(scrollbar);
		scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
		scrollbar->set_size(t3_win_get_height(window), None);
		wrap_info->set_wrap_width(t3_win_get_width(window));
	} else {
		delete scrollbar;
		scrollbar = NULL;
		wrap_info->set_wrap_width(t3_win_get_width(window) + 1);
	}
	redraw = true;
}

void text_window_t::inc_y(void) {
	text_coordinate_t check_top = top;
	if (wrap_info->add_lines(check_top, t3_win_get_height(window)))
		return;
	wrap_info->add_lines(top, 1);
	redraw = true;
}

void text_window_t::dec_y(void) {
	if (!wrap_info->sub_lines(top, 1))
		redraw = true;
}

void text_window_t::pgdn(void) {
	text_coordinate_t new_top = top;

	if (wrap_info->add_lines(new_top, 2 * t3_win_get_height(window) - 1)) {
		wrap_info->sub_lines(new_top, t3_win_get_height(window) - 1);
		if (top != new_top) {
			top = new_top;
			redraw = true;
		}
	} else {
		wrap_info->add_lines(top, t3_win_get_height(window) - 1);
		redraw = true;
	}
}

void text_window_t::pgup(void) {
	if (top.line == 0 && top.pos == 0)
		return;

	wrap_info->sub_lines(top, t3_win_get_height(window) - 1);
	redraw = true;
}

bool text_window_t::process_key(key_t key) {
	switch (key) {
		case EKEY_NL:
			activate();
			break;
		case EKEY_DOWN:
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
			if (top.line != 0 || top.pos != 0) {
				top.line = 0;
				top.pos = 0;
				redraw = true;
			}
			break;
		case EKEY_END | EKEY_CTRL:
		case EKEY_END: {
			text_coordinate_t new_top = wrap_info->get_end();
			wrap_info->sub_lines(new_top, t3_win_get_height(window));
			if (new_top != top)
				redraw = true;
			break;
		}
		default:
			return false;
	}
	return true;
}

void text_window_t::update_contents(void) {
	text_coordinate_t current_start, current_end;
	text_line_t::paint_info_t info;
	int i, count = 0;

	if (!redraw)
		return;

	redraw = false;
	t3_win_set_default_attrs(window, attributes.dialog);

	info.size = t3_win_get_width(window);
	if (scrollbar == NULL)
		info.size++;
	info.normal_attr = 0;
	info.selected_attr = 0; /* There is no selected text anyway. */
	info.selection_start = -1;
	info.selection_end = -1;
	info.cursor = -1;
	info.leftcol = 0;

	text_coordinate_t end = wrap_info->get_end();
	text_coordinate_t draw_line = top;

	for (i = 0; i < t3_win_get_height(window); i++, wrap_info->add_lines(draw_line, 1)) {
		if (focus) {
			if (i == 0)
				info.cursor = wrap_info->calculate_line_pos(draw_line.line, 0, draw_line.pos);
			else
				info.cursor = -1;
		}
		t3_win_set_paint(window, i, 0);
		t3_win_clrtoeol(window);
		wrap_info->paint_line(window, draw_line, &info);

		if (draw_line.line == end.line && draw_line.pos == end.pos)
			break;
	}

	t3_win_clrtobot(window);


	for (i = 0; i < top.line; i++)
		count += wrap_info->get_line_count(i);
	count += top.pos;

	if (scrollbar != NULL) {
		scrollbar->set_parameters(max(wrap_info->get_size(), count + t3_win_get_height(window)),
			count, t3_win_get_height(window));
		scrollbar->update_contents();
	}
}

void text_window_t::set_focus(bool _focus) {
	if (focus != _focus)
		redraw = true;
	focus = _focus;
}

void text_window_t::focus_set(widget_t *target) {
	(void) target;
	set_focus(true);
}

bool text_window_t::is_child(widget_t *component) {
	return component == scrollbar;
}

text_buffer_t *text_window_t::get_text(void) {
	return text;
}

void text_window_t::set_tabsize(int size) {
	wrap_info->set_tabsize(size);
}

int text_window_t::get_text_height(void) {
	return wrap_info->get_size();
}

}; // namespace
