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
#include "colorscheme.h"
#include "widgets/listpane.h"
#include "log.h"

using namespace std;
namespace t3_widget {

list_pane_t::list_pane_t(bool _indicator) : top_idx(0), current(0),
		has_focus(false), indicator(_indicator)
{
	init_unbacked_window(1, 3, true);
	widgets_window = (t3_window_t *) window;
	window = NULL;

	init_unbacked_window(1, 4);
	t3_win_set_parent(widgets_window, window);
	t3_win_set_anchor(widgets_window, window, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	scrollbar = new scrollbar_t(true);
	container_t::set_widget_parent(scrollbar);
	scrollbar->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	scrollbar->set_size(1, None);
	scrollbar->connect_clicked(sigc::mem_fun(this, &list_pane_t::scrollbar_clicked));

	if (indicator) {
		indicator_widget = new indicator_widget_t();
		set_widget_parent(indicator_widget);
	}
}

list_pane_t::~list_pane_t(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		delete *iter;
}

bool list_pane_t::set_widget_parent(widget_t *widget) {
	return t3_win_set_parent(widget->get_base_window(), widgets_window);
}

void list_pane_t::ensure_cursor_on_screen(void) {
	int height = t3_win_get_height(window);
	if (current >= top_idx + height)
		top_idx = current - height + 1;
	else if (current < top_idx)
		top_idx = current;
}

bool list_pane_t::process_key(key_t key) {
	size_t old_current = current;
	int height;

	switch (key) {
		case EKEY_DOWN:
			if (current + 1 >= widgets.size())
				return true;
			current++;
			break;
		case EKEY_UP:
			if (current == 0)
				return true;
			current--;
			break;
		case EKEY_END:
			current = widgets.size() - 1;
			break;
		case EKEY_HOME:
			current = 0;
			break;
		case EKEY_PGDN:
			height = t3_win_get_height(window);
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
			height = t3_win_get_height(window);
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
			return true;
		default:
			if (widgets.size() > 0)
				return widgets[current]->process_key(key);
			return false;
	}
	if (current != old_current) {
		widgets[old_current]->set_focus(false);
		widgets[current]->set_focus(has_focus);
		selection_changed();
	}
	ensure_cursor_on_screen();
	return true;
}

void list_pane_t::set_position(optint top, optint left) {
	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);
	t3_win_move(window, top, left);
}

bool list_pane_t::set_size(optint height, optint width) {
	int widget_width;
	bool result;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result = t3_win_resize(window, height, width);
	result &= t3_win_resize(widgets_window, t3_win_get_height(widgets_window), width - 1);
	if (indicator)
		result &= indicator_widget->set_size(None, width - 1);

	widget_width = indicator ? (int) width - 3 : (int) width - 1;

	for (widgets_t::iterator iter = widgets.begin();
			iter != widgets.end(); iter++)
		result &= (*iter)->set_size(None, widget_width);

	result &= scrollbar->set_size(height, None);

	ensure_cursor_on_screen();
	return result;
}


void list_pane_t::update_contents(void) {
	if (indicator) {
		indicator_widget->update_contents();
		indicator_widget->set_position(current, 0);
	}

	t3_win_move(widgets_window, -top_idx, 0);
	scrollbar->set_parameters(widgets.size(), top_idx, t3_win_get_height(window));
	scrollbar->update_contents();
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void list_pane_t::set_focus(bool focus) {
	has_focus = focus;
	if (current < widgets.size())
		widgets[current]->set_focus(focus);
	if (indicator)
		indicator_widget->set_focus(focus);
}

bool list_pane_t::process_mouse_event(mouse_event_t event) {
	if (event.type == EMOUSE_BUTTON_RELEASE &&
			(event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) &&
			event.window != widgets_window)
	{
		activate();
	} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
		scroll((event.button_state & EMOUSE_SCROLL_UP) ? -3 : 3);
	}
	return true;
}

void list_pane_t::reset(void) {
	top_idx = 0;
	current = 0;
}

void list_pane_t::update_positions(void) {
	widgets_t::iterator iter;
	size_t idx;

	t3_win_resize(widgets_window, widgets.size(), t3_win_get_width(widgets_window));
	for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++)
		(*iter)->set_position(idx, indicator ? 1 : 0);
}

void list_pane_t::set_anchor(window_component_t *anchor, int relation) {
	t3_win_set_anchor(window, anchor->get_base_window(), relation);
}

void list_pane_t::force_redraw(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->force_redraw();
	if (indicator)
		indicator_widget->force_redraw();
}

void list_pane_t::focus_set(widget_t *target) {
	widgets_t::iterator iter;
	size_t idx;
	size_t old_current = current;

	if (target == scrollbar || target == indicator_widget) {
		set_focus(true);
		return;
	}

	for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
		if (*iter == target) {
			break;
		} else {
			container_t *container = dynamic_cast<container_t *>(*iter);
			if (container != NULL && container->is_child(target))
				break;
		}
	}
	if (idx < widgets.size()) {
		current = idx;
		if (has_focus) {
			if (current != old_current) {
				widgets[old_current]->set_focus(false);
				widgets[current]->set_focus(has_focus);
				selection_changed();
			}
		} else {
			set_focus(true);
		}
	}
}

bool list_pane_t::is_child(widget_t *widget) {
	if (widget == scrollbar || widget == indicator_widget)
		return true;

	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
		if (*iter == widget) {
			return true;
		} else {
			container_t *container = dynamic_cast<container_t *>(*iter);
			if (container != NULL && container->is_child(widget))
				return true;
		}
	}
	return false;
}

void list_pane_t::push_back(widget_t *widget) {
	widget->set_size(1, t3_win_get_width(widgets_window) - (indicator ? 2 : 0));
	widget->set_position(widgets.size(), indicator ? 1 : 0);
	set_widget_parent(widget);
	widgets.push_back(widget);
	t3_win_resize(widgets_window, widgets.size(), t3_win_get_width(widgets_window));
}

void list_pane_t::push_front(widget_t *widget) {
	widget->set_size(1, t3_win_get_width(widgets_window) - (indicator ? 2 : 0));
	set_widget_parent(widget);
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
			widgets[current]->set_focus(has_focus);
		}
	}
	unset_widget_parent(widgets.back());
	widgets.pop_back();
	t3_win_resize(widgets_window, widgets.size(), t3_win_get_width(widgets_window));
}

void list_pane_t::pop_front(void) {
	if (current == 0) {
		widgets[0]->set_focus(false);
		if (widgets.size() > 1)
			widgets[1]->set_focus(has_focus);
	} else {
		current--;
	}
	unset_widget_parent(widgets.front());
	widgets.pop_front();
	update_positions();
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
	unset_widget_parent(widgets[position]);
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

size_t list_pane_t::get_current(void) const {
	return current;
}

void list_pane_t::set_current(size_t idx) {
	if (idx >= widgets.size())
		return;

	current = idx;
	ensure_cursor_on_screen();
}

void list_pane_t::scroll(int change) {
	top_idx = (change < 0 && top_idx < (size_t) -change) ? 0 :
		(change > 0 && top_idx + t3_win_get_height(window) + change >= widgets.size()) ?
			widgets.size() - t3_win_get_height(window) : top_idx + change;
}
void list_pane_t::scrollbar_clicked(scrollbar_t::step_t step) {
	scroll(step == scrollbar_t::BACK_SMALL ? -3 :
		step == scrollbar_t::BACK_MEDIUM ? -t3_win_get_height(window) / 2 :
		step == scrollbar_t::BACK_PAGE ? -t3_win_get_height(window) :
		step == scrollbar_t::FWD_SMALL ? 3 :
		step == scrollbar_t::FWD_MEDIUM ? t3_win_get_height(window) / 2 :
		step == scrollbar_t::FWD_PAGE ? t3_win_get_height(window) : 0);
}

//=========== Indicator widget ================

list_pane_t::indicator_widget_t::indicator_widget_t(void) : widget_t(1, 3), has_focus(false) {
	t3_win_set_depth(window, INT_MAX);
}

bool list_pane_t::indicator_widget_t::process_key(key_t key) { (void) key; return false; }

void list_pane_t::indicator_widget_t::update_contents(void) {
	if (!redraw)
		return;
	redraw = false;
	t3_win_set_default_attrs(window, attributes.dialog);
	t3_win_set_paint(window, 0, 0);
	t3_win_addch(window, T3_ACS_RARROW, T3_ATTR_ACS | (has_focus ? attributes.dialog_selected : attributes.dialog));
	t3_win_set_paint(window, 0, t3_win_get_width(window) - 1);
	t3_win_addch(window, T3_ACS_LARROW, T3_ATTR_ACS | (has_focus ? attributes.dialog_selected : attributes.dialog));
}

void list_pane_t::indicator_widget_t::set_focus(bool focus) {
	has_focus = focus;
	redraw = true;
}

bool list_pane_t::indicator_widget_t::set_size(optint _height, optint width) {
	(void) _height;

	if (!width.is_valid())
		return true;
	redraw = true;
	return t3_win_resize(window, 1, width);
}

bool list_pane_t::indicator_widget_t::accepts_focus(void) { return false; }

}; // namespace
