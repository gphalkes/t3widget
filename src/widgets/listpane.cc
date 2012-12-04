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
#include "colorscheme.h"
#include "widgets/listpane.h"
#include "log.h"

using namespace std;
namespace t3_widget {

list_pane_t::list_pane_t(bool _indicator) : impl(new implementation_t(_indicator)) {
	init_unbacked_window(1, 3, true);
	impl->widgets_window = window.release();

	init_unbacked_window(1, 4);
	t3_win_set_parent(impl->widgets_window, window);
	t3_win_set_anchor(impl->widgets_window, window, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

	container_t::set_widget_parent(&impl->scrollbar);
	impl->scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	impl->scrollbar.set_size(1, None);
	impl->scrollbar.connect_clicked(sigc::mem_fun(this, &list_pane_t::scrollbar_clicked));

	if (impl->indicator) {
		impl->indicator_widget = new indicator_widget_t();
		set_widget_parent(impl->indicator_widget);
	}
}

list_pane_t::~list_pane_t(void) {
	for (widgets_t::iterator iter = impl->widgets.begin(); iter != impl->widgets.end(); iter++)
		delete *iter;
}

bool list_pane_t::set_widget_parent(window_component_t *widget) {
	return t3_win_set_parent(widget->get_base_window(), impl->widgets_window);
}

void list_pane_t::ensure_cursor_on_screen(void) {
	int height = t3_win_get_height(window);
	if (impl->current >= impl->top_idx + height)
		impl->top_idx = impl->current - height + 1;
	else if (impl->current < impl->top_idx)
		impl->top_idx = impl->current;
}

bool list_pane_t::process_key(key_t key) {
	size_t old_current = impl->current;
	window_component_t::focus_t focus_type;
	int height;

	switch (key) {
		case EKEY_DOWN:
			if (impl->current + 1 >= impl->widgets.size())
				return true;
			impl->current++;
			focus_type = window_component_t::FOCUS_IN_FWD;
			break;
		case EKEY_UP:
			if (impl->current == 0)
				return true;
			impl->current--;
			focus_type = window_component_t::FOCUS_IN_BCK;
			break;
		case EKEY_END:
			impl->current = impl->widgets.size() - 1;
			focus_type = window_component_t::FOCUS_SET;
			break;
		case EKEY_HOME:
			impl->current = 0;
			focus_type = window_component_t::FOCUS_SET;
			break;
		case EKEY_PGDN:
			height = t3_win_get_height(window);
			if (impl->current + height >= impl->widgets.size()) {
				impl->current = impl->widgets.size() - 1;
			} else {
				impl->current += height;
				if (impl->top_idx + 2 * height < impl->widgets.size())
					impl->top_idx += height;
				else
					impl->top_idx = impl->widgets.size() - height;
			}
			focus_type = window_component_t::FOCUS_SET;
			break;
		case EKEY_PGUP:
			height = t3_win_get_height(window);
			if (impl->current < (size_t) height) {
				impl->current = 0;
			} else {
				impl->current -= height;
				impl->top_idx -= height;
			}
			focus_type = window_component_t::FOCUS_SET;
			break;
		case EKEY_NL:
			if (impl->widgets.size() > 0)
				activate();
			return true;
		default:
			if (impl->widgets.size() > 0)
				return impl->widgets[impl->current]->process_key(key);
			return false;
	}
	if (impl->current != old_current) {
		impl->widgets[old_current]->set_focus(window_component_t::FOCUS_OUT);
		impl->widgets[impl->current]->set_focus(impl->has_focus ? focus_type : window_component_t::FOCUS_OUT);
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
	result &= t3_win_resize(impl->widgets_window, t3_win_get_height(impl->widgets_window), width - 1);
	if (impl->indicator)
		result &= impl->indicator_widget->set_size(None, width - 1);

	widget_width = impl->indicator ? (int) width - 3 : (int) width - 1;

	for (widgets_t::iterator iter = impl->widgets.begin();
			iter != impl->widgets.end(); iter++)
		result &= (*iter)->set_size(None, widget_width);

	result &= impl->scrollbar.set_size(height, None);

	ensure_cursor_on_screen();
	return result;
}


void list_pane_t::update_contents(void) {
	if (impl->indicator) {
		impl->indicator_widget->update_contents();
		impl->indicator_widget->set_position(impl->current, 0);
	}

	t3_win_move(impl->widgets_window, -impl->top_idx, 0);
	impl->scrollbar.set_parameters(impl->widgets.size(), impl->top_idx, t3_win_get_height(window));
	impl->scrollbar.update_contents();
	for (widgets_t::iterator iter = impl->widgets.begin(); iter != impl->widgets.end(); iter++)
		(*iter)->update_contents();
}

void list_pane_t::set_focus(focus_t focus) {
	impl->has_focus = focus;
	if (impl->current < impl->widgets.size())
		impl->widgets[impl->current]->set_focus(focus);
	if (impl->indicator)
		impl->indicator_widget->set_focus(focus);
}

bool list_pane_t::process_mouse_event(mouse_event_t event) {
	if (event.type == EMOUSE_BUTTON_RELEASE &&
			(event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) &&
			event.window != impl->widgets_window)
	{
		activate();
	} else if (event.type == EMOUSE_BUTTON_RELEASE &&
			(event.button_state & EMOUSE_CLICKED_LEFT) &&
			event.window != impl->widgets_window)
	{
		impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_OUT);
		impl->current = event.y;
		impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
		selection_changed();
	} else if (event.type == EMOUSE_BUTTON_PRESS && (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
		scroll((event.button_state & EMOUSE_SCROLL_UP) ? -3 : 3);
	}
	return true;
}

void list_pane_t::reset(void) {
	impl->top_idx = 0;
	impl->current = 0;
}

void list_pane_t::update_positions(void) {
	widgets_t::iterator iter;
	size_t idx;

	t3_win_resize(impl->widgets_window, impl->widgets.size(), t3_win_get_width(impl->widgets_window));
	for (iter = impl->widgets.begin(), idx = 0; iter != impl->widgets.end(); iter++, idx++)
		(*iter)->set_position(idx, impl->indicator ? 1 : 0);
}

void list_pane_t::set_anchor(window_component_t *anchor, int relation) {
	t3_win_set_anchor(window, anchor->get_base_window(), relation);
}

void list_pane_t::force_redraw(void) {
	for (widgets_t::iterator iter = impl->widgets.begin(); iter != impl->widgets.end(); iter++)
		(*iter)->force_redraw();
	if (impl->indicator)
		impl->indicator_widget->force_redraw();
}

void list_pane_t::set_child_focus(window_component_t *target) {
	widgets_t::iterator iter;
	size_t idx;
	size_t old_current = impl->current;

	if (target == &impl->scrollbar || target == impl->indicator_widget) {
		set_focus(window_component_t::FOCUS_SET);
		return;
	}

	for (iter = impl->widgets.begin(), idx = 0; iter != impl->widgets.end(); iter++, idx++) {
		if (*iter == target) {
			break;
		} else {
			container_t *container = dynamic_cast<container_t *>(*iter);
			if (container != NULL && container->is_child(target))
				break;
		}
	}
	if (idx < impl->widgets.size()) {
		impl->current = idx;
		if (impl->has_focus) {
			if (impl->current != old_current) {
				impl->widgets[old_current]->set_focus(window_component_t::FOCUS_OUT);
				impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
				selection_changed();
			}
		} else {
			set_focus(window_component_t::FOCUS_SET);
		}
	}
}

bool list_pane_t::is_child(window_component_t *widget) {
	if (widget == &impl->scrollbar || widget == impl->indicator_widget)
		return true;

	for (widgets_t::iterator iter = impl->widgets.begin(); iter != impl->widgets.end(); iter++) {
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
	widget->set_size(1, t3_win_get_width(impl->widgets_window) - (impl->indicator ? 2 : 0));
	widget->set_position(impl->widgets.size(), impl->indicator ? 1 : 0);
	set_widget_parent(widget);
	impl->widgets.push_back(widget);
	t3_win_resize(impl->widgets_window, impl->widgets.size(), t3_win_get_width(impl->widgets_window));
}

void list_pane_t::push_front(widget_t *widget) {
	widget->set_size(1, t3_win_get_width(impl->widgets_window) - (impl->indicator ? 2 : 0));
	set_widget_parent(widget);
	impl->widgets.push_front(widget);
	if (impl->current + 1 < impl->widgets.size())
		impl->current++;
	update_positions();
}

void list_pane_t::pop_back(void) {
	if (impl->current + 1 == impl->widgets.size()) {
		impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_OUT);
		if (impl->current > 0) {
			impl->current--;
			if (impl->has_focus)
				impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
		}
	}
	unset_widget_parent(impl->widgets.back());
	impl->widgets.pop_back();
	t3_win_resize(impl->widgets_window, impl->widgets.size(), t3_win_get_width(impl->widgets_window));
}

void list_pane_t::pop_front(void) {
	if (impl->current == 0) {
		impl->widgets[0]->set_focus(window_component_t::FOCUS_OUT);
		if (impl->widgets.size() > 1 && impl->has_focus)
			impl->widgets[1]->set_focus(window_component_t::FOCUS_SET);
	} else {
		impl->current--;
	}
	unset_widget_parent(impl->widgets.front());
	impl->widgets.pop_front();
	update_positions();
}

widget_t *list_pane_t::back(void) {
	return (widget_t *) impl->widgets.back();
}

widget_t *list_pane_t::operator[](int idx) {
	return (widget_t *) impl->widgets[idx];
}

size_t list_pane_t::size(void) {
	return impl->widgets.size();
}

bool list_pane_t::empty(void) {
	return impl->widgets.empty();
}

list_pane_t::iterator list_pane_t::erase(list_pane_t::iterator position) {
	if (impl->current == position && impl->current + 1 == impl->widgets.size()) {
		if (impl->current != 0)
			impl->current--;
	}
	unset_widget_parent(impl->widgets[position]);
	impl->widgets.erase(impl->widgets.begin() + position);
	update_positions();
	return position;
}

list_pane_t::iterator list_pane_t::begin(void) {
	return 0;
}

list_pane_t::iterator list_pane_t::end(void) {
	return impl->widgets.size();
}

size_t list_pane_t::get_current(void) const {
	return impl->current;
}

void list_pane_t::set_current(size_t idx) {
	if (idx >= impl->widgets.size())
		return;

	impl->current = idx;
	ensure_cursor_on_screen();
}

void list_pane_t::scroll(int change) {
	impl->top_idx = (change < 0 && impl->top_idx < (size_t) -change) ? 0 :
		(change > 0 && impl->top_idx + t3_win_get_height(window) + change >= impl->widgets.size()) ?
			impl->widgets.size() - t3_win_get_height(window) : impl->top_idx + change;
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

void list_pane_t::indicator_widget_t::set_focus(focus_t focus) {
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
