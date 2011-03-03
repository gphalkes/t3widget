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
#include "colorscheme.h"
#include "widgets/listpane.h"

#warning FIXME: do not draw on the parent window!
list_pane_t::list_pane_t(container_t *_parent, int _height, int _width, int _top, int _left, bool _indicator) : height(_height - 2),
	width(_width - 2), top(_top), left(_left), top_idx(0), parent(_parent), widgets(NULL), indicator(_indicator)
{
	clip_window = t3_win_new_relative(parent->get_draw_window(), height, width, top + 1, left + 1, 0, parent->get_draw_window(), T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	window = t3_win_new_unbacked(clip_window, 1, width, 0, 0, 0, clip_window, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	t3_win_set_default_attrs(window, colors.dialog_attrs);
	focus = false;
	current = 0;
	t3_win_show(window);
	t3_win_show(clip_window);

	clipwindow_component_t scrollbarAnchor(clip_window);
	scrollbar = new scrollbar_t(parent, &scrollbarAnchor, 0, 0, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), height);
}

list_pane_t::~list_pane_t(void) {
	t3_win_del(clip_window);
}

void list_pane_t::ensure_cursor_on_screen(void) {
	if (current >= top_idx + height)
		top_idx = current - height + 1;
	else if (current < top_idx)
		top_idx = current;
}

void list_pane_t::process_key(key_t key) {
	size_t old_current = current;
	switch (key) {
		case EKEY_DOWN:
			if (current + 1 >= widgets.size())
				return;
			current++;
			break;
		case EKEY_UP:
			if (current == 0)
				return;
			current--;
			break;
		case EKEY_END:
			current = widgets.size() - 1;
			break;
		case EKEY_HOME:
			current = 0;
			break;
		case EKEY_PGDN:
			if (current + height >= widgets.size()) {
				current = widgets.size() - 1;
			} else {
				current += height;
				if (top_idx + 2 * height < widgets.size())
					top_idx += height;
				else
					top_idx = widgets.size() - height;
			}
			break;
		case EKEY_PGUP:
			if (current < (size_t) height) {
				current = 0;
			} else {
				current -= height;
				top_idx -= height;
			}
			break;
		case EKEY_NL:
			if (widgets.size() > 0)
				activate();
			return;
		default:
			if (widgets.size() > 0)
				widgets[current]->process_key(key);
			return;
	}
	if (current != old_current) {
		widgets[old_current]->set_focus(false);
		widgets[current]->set_focus(focus);
		selection_changed();
	}
	ensure_cursor_on_screen();
}

void list_pane_t::set_position(optint _top, optint _left) {
	if (_top.is_valid())
		top = _top;
	if (_left.is_valid())
		left = _left;

	t3_win_move(clip_window, top + 1, left + 1);
}

bool list_pane_t::set_size(optint _height, optint _width) {
	int widget_width;

	if (_height.is_valid())
		height = _height - 2;
	if (_width.is_valid())
		width = _width - 2;

	t3_win_resize(clip_window, height, width);
	t3_win_resize(window, t3_win_get_height(window), width);

	widget_width = indicator ? width - 2 : width;

	for (widgets_t::iterator iter = widgets.begin();
			iter != widgets.end(); iter++)
		(*iter)->resize(None, widget_width, None, None);

	scrollbar->resize(height, None, None, None);

	ensure_cursor_on_screen();
	//FIXME: let return value depend on success!
	return true;
}


void list_pane_t::update_contents(void) {
	t3_win_set_paint(clip_window, 0, 0);
	t3_win_clrtobot(clip_window);
	if (indicator && widgets.size() > 0) {
		t3_win_set_paint(clip_window, current - top_idx, 0);
		t3_win_addch(clip_window, T3_ACS_RARROW, T3_ATTR_ACS | (focus ? T3_ATTR_REVERSE : 0));
		t3_win_set_paint(clip_window, current - top_idx, width - 1);
		t3_win_addch(clip_window, T3_ACS_LARROW, T3_ATTR_ACS | (focus ? T3_ATTR_REVERSE : 0));
	}
	t3_win_move(window, -top_idx, 0);
	t3_win_box(parent->get_draw_window(), top, left, height + 2, width + 2, 0);
	scrollbar->set_parameters(widgets.size(), top_idx, height);
	scrollbar->update_contents();
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void list_pane_t::set_focus(bool _focus) {
	focus = _focus;
	if (widgets.size() == 0)
		t3_term_hide_cursor();
	else if (current < widgets.size())
		widgets[current]->set_focus(focus);
}

bool list_pane_t::accepts_enter(void) { return true; }

void list_pane_t::reset(void) {
	top_idx = 0;
	current = 0;
}

void list_pane_t::update_positions(void) {
	widgets_t::iterator iter;
	size_t idx;
	int widget_width = width;
	if (indicator)
		widget_width -= 2;

	t3_win_resize(window, widgets.size(), t3_win_get_width(clip_window));
	for (iter = widgets.begin(), idx = 0;
			iter != widgets.end();
			iter++, idx++)
		(*iter)->resize(1, widget_width, idx, indicator ? 1 : 0);
}

void list_pane_t::push_back(widget_t *widget) {
	widgets.push_back(widget);
	update_positions();
}

void list_pane_t::push_front(widget_t *widget) {
	widgets.push_front(widget);
	if (current + 1 < widgets.size())
		current++;
	update_positions();
}

void list_pane_t::pop_back(void) {
	if (current + 1 == widgets.size()) {
		widgets[current]->set_focus(false);
		if (current > 0) {
			current--;
			widgets[current]->set_focus(focus);
		}
	}
	widgets.pop_back();
}

void list_pane_t::pop_front(void) {
	if (current == 0) {
		widgets[0]->set_focus(false);
		if (widgets.size() > 1)
			widgets[1]->set_focus(focus);
	} else {
		current--;
	}
	widgets.pop_front();
}

widget_t *list_pane_t::back(void) {
	return (widget_t *) widgets.back();
}

widget_t *list_pane_t::operator[](int idx) {
	return (widget_t *) widgets[idx];
}

size_t list_pane_t::size(void) {
	return widgets.size();
}

bool list_pane_t::empty(void) {
	return widgets.empty();
}

list_pane_t::iterator list_pane_t::erase(list_pane_t::iterator position) {
	if (current == position && current + 1 == widgets.size()) {
		if (current != 0)
			current--;
	}
	widgets.erase(widgets.begin() + position);
	update_positions();
	return position;
}

list_pane_t::iterator list_pane_t::begin(void) {
	return 0;
}

list_pane_t::iterator list_pane_t::end(void) {
	return widgets.size();
}

int list_pane_t::get_current(void) {
	return current;
}

void list_pane_t::set_current(int idx) {
	if (idx < 0 || idx >= (int) widgets.size())
		return;

	current = idx;
	ensure_cursor_on_screen();
}
