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

text_window_t::text_window_t(text_buffer_t *_text, bool with_scrollbar) : widget_t(11, 11), impl(new implementation_t()) {
	/* Note: we use a dirty trick here: the last position of the window is obscured by
	   the impl->scrollbar-> However, the last position will only contain the wrap character
	   anyway, so we don't care. If the impl->scrollbar is disabled, we set the wrap width
	   one higher, such that the wrap character falls off the edge. */
	if (with_scrollbar) {
		impl->scrollbar = new scrollbar_t(true);
		set_widget_parent(impl->scrollbar);
		impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
		impl->scrollbar->set_size(11, None);
		impl->scrollbar->connect_clicked(sigc::mem_fun(this, &text_window_t::scrollbar_clicked));
	}

	if (_text == NULL)
		impl->text = new text_buffer_t();
	else
		impl->text = _text;

	impl->wrap_info = new wrap_info_t(impl->scrollbar != NULL ? 11 : 12);
	impl->wrap_info->set_text_buffer(impl->text);
}

void text_window_t::set_text(text_buffer_t *_text) {
	if (impl->text == _text)
		return;

	impl->text = _text;
	impl->wrap_info->set_text_buffer(impl->text);
	impl->top.line = 0;
	impl->top.pos = 0;
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
	if (impl->scrollbar != NULL) {
		result &= impl->scrollbar->set_size(height, None);
		impl->wrap_info->set_wrap_width(width);
	} else {
		impl->wrap_info->set_wrap_width(width + 1);
	}

	return result;
}

void text_window_t::set_scrollbar(bool with_scrollbar) {
	if (with_scrollbar == (impl->scrollbar != NULL))
		return;
	if (with_scrollbar) {
		impl->scrollbar = new scrollbar_t(true);
		set_widget_parent(impl->scrollbar);
		impl->scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
		impl->scrollbar->set_size(t3_win_get_height(window), None);
		impl->wrap_info->set_wrap_width(t3_win_get_width(window));
	} else {
		impl->scrollbar = NULL;
		impl->wrap_info->set_wrap_width(t3_win_get_width(window) + 1);
	}
	redraw = true;
}

void text_window_t::scroll_down(int lines) {
	text_coordinate_t new_top = impl->top;

	if (impl->wrap_info->add_lines(new_top, t3_win_get_height(window) + lines)) {
		impl->wrap_info->sub_lines(new_top, t3_win_get_height(window) - 1);
		if (impl->top != new_top) {
			impl->top = new_top;
			redraw = true;
		}
	} else {
		impl->wrap_info->add_lines(impl->top, lines);
		redraw = true;
	}
}

void text_window_t::scroll_up(int lines) {
	if (impl->top.line == 0 && impl->top.pos == 0)
		return;

	impl->wrap_info->sub_lines(impl->top, lines);
	redraw = true;
}

bool text_window_t::process_key(key_t key) {
	switch (key) {
		case EKEY_NL:
			activate();
			break;
		case EKEY_DOWN:
			scroll_down(1);
			break;
		case EKEY_UP:
			scroll_up(1);
			break;
		case EKEY_PGUP:
			scroll_up(t3_win_get_height(window) - 1);
			break;
		case EKEY_PGDN:
		case ' ':
			scroll_down(t3_win_get_height(window) - 1);
			break;
		case EKEY_HOME | EKEY_CTRL:
		case EKEY_HOME:
			if (impl->top.line != 0 || impl->top.pos != 0) {
				impl->top.line = 0;
				impl->top.pos = 0;
				redraw = true;
			}
			break;
		case EKEY_END | EKEY_CTRL:
		case EKEY_END: {
			text_coordinate_t new_top = impl->wrap_info->get_end();
			impl->wrap_info->sub_lines(new_top, t3_win_get_height(window));
			if (new_top != impl->top)
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
	if (impl->scrollbar == NULL)
		info.size++;
	info.normal_attr = 0;
	info.selected_attr = 0; /* There is no selected impl->text anyway. */
	info.selection_start = -1;
	info.selection_end = -1;
	info.cursor = -1;
	info.leftcol = 0;
	info.flags = 0;

	text_coordinate_t end = impl->wrap_info->get_end();
	text_coordinate_t draw_line = impl->top;

	for (i = 0; i < t3_win_get_height(window); i++, impl->wrap_info->add_lines(draw_line, 1)) {
		if (impl->focus) {
			if (i == 0)
				info.cursor = impl->wrap_info->calculate_line_pos(draw_line.line, 0, draw_line.pos);
			else
				info.cursor = -1;
		}
		t3_win_set_paint(window, i, 0);
		t3_win_clrtoeol(window);
		impl->wrap_info->paint_line(window, draw_line, &info);

		if (draw_line.line == end.line && draw_line.pos == end.pos)
			break;
	}

	t3_win_clrtobot(window);


	for (i = 0; i < impl->top.line; i++)
		count += impl->wrap_info->get_line_count(i);
	count += impl->top.pos;

	if (impl->scrollbar != NULL) {
		impl->scrollbar->set_parameters(max(impl->wrap_info->get_size(), count + t3_win_get_height(window)),
			count, t3_win_get_height(window));
		impl->scrollbar->update_contents();
	}
}

void text_window_t::set_focus(bool _focus) {
	if (impl->focus != _focus)
		redraw = true;
	impl->focus = _focus;
}

void text_window_t::focus_set(widget_t *target) {
	(void) target;
	set_focus(true);
}

bool text_window_t::is_child(widget_t *component) {
	return component == impl->scrollbar;
}

text_buffer_t *text_window_t::get_text(void) {
	return impl->text;
}

void text_window_t::set_tabsize(int size) {
	impl->wrap_info->set_tabsize(size);
}

int text_window_t::get_text_height(void) {
	return impl->wrap_info->get_size();
}

bool text_window_t::process_mouse_event(mouse_event_t event) {
	if (event.window != window || event.type != EMOUSE_BUTTON_PRESS)
		return true;
	if (event.button_state & EMOUSE_SCROLL_UP)
		scroll_up(3);
	else if (event.button_state & EMOUSE_SCROLL_DOWN)
		scroll_down(3);
	return true;
}

void text_window_t::scrollbar_clicked(scrollbar_t::step_t step) {
	int scroll = step == scrollbar_t::BACK_SMALL ? -3 :
		step == scrollbar_t::FWD_SMALL ? 3 :
		step == scrollbar_t::BACK_MEDIUM ? -t3_win_get_height(window) / 2 :
		step == scrollbar_t::FWD_MEDIUM ? t3_win_get_height(window) / 2 :
		step == scrollbar_t::BACK_PAGE ? -(t3_win_get_height(window) - 1) :
		step == scrollbar_t::FWD_PAGE ? (t3_win_get_height(window) - 1) : 0;

	if (scroll < 0)
		scroll_up(-scroll);
	else
		scroll_down(scroll);
}

}; // namespace
