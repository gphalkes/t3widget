/* Copyright (C) 2012 G.P. Halkes
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
#include "widgets/widgetgroup.h"
#include "log.h"

using namespace std;
namespace t3_widget {

widget_group_t::widget_group_t(void) : impl(new implementation_t()) {
	init_unbacked_window(1, 1);
}

void widget_group_t::focus_next(void) {
	if (impl->current_child + 1 < (int) impl->children.size()) {
		impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
		impl->current_child++;
		impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_IN_FWD);
	} else {
		move_focus_down();
	}
}

void widget_group_t::focus_previous(void) {
	if (impl->current_child > 0) {
		impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
		impl->current_child--;
		impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_IN_BCK);
	} else {
		move_focus_up();
	}
}

void widget_group_t::add_child(widget_t *child) {
	set_widget_parent(child);

	impl->children.push_back(child);
	if (impl->children.size() == 1)
		impl->current_child = 0;
}

widget_group_t::~widget_group_t(void) {
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++)
		delete *iter;
}

bool widget_group_t::process_key(key_t key) {
	if (impl->children.size() == 0)
		return false;

	if (impl->children[impl->current_child]->process_key(key))
		return true;

	if (key == '\t') {
		if (impl->current_child + 1 < (int) impl->children.size()) {
			if (impl->has_focus)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
			++impl->current_child;
			if (impl->has_focus)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_IN_FWD);
			return true;
		}
	} else if (key == (EKEY_SHIFT | '\t')) {
		if (impl->current_child > 0) {
			if (impl->has_focus)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
			--impl->current_child;
			if (impl->has_focus)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_IN_FWD);
			return true;
		}
	}
	return false;
}

void widget_group_t::update_contents(void) {
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++)
		(*iter)->update_contents();
}

void widget_group_t::set_focus(focus_t focus) {
	impl->has_focus = focus != window_component_t::FOCUS_OUT && impl->children.size() > 0;
	if (impl->children.size() == 0)
		return;
	switch (focus) {
		case window_component_t::FOCUS_OUT:
			impl->children[impl->current_child]->set_focus(focus);
			break;
		case window_component_t::FOCUS_SET:
		case window_component_t::FOCUS_IN_FWD:
			for (impl->current_child = 0; impl->current_child < (int) impl->children.size() &&
				!impl->children[impl->current_child]->accepts_focus(); impl->current_child++) {}
			impl->children[impl->current_child]->set_focus(focus);
			break;
		case window_component_t::FOCUS_IN_BCK: {
			for (impl->current_child = impl->children.size() - 1; impl->current_child >= 0 &&
					!impl->children[impl->current_child]->accepts_focus(); impl->current_child--) {}
			impl->children[impl->current_child]->set_focus(focus);
			break;
		}
		default:
			break;
	}
}

bool widget_group_t::set_size(optint height, optint width) {
	if (!width.is_valid())
		width = t3_win_get_width(window);
	if (!height.is_valid())
		height = t3_win_get_height(window);

	t3_win_resize(window, height, width);
	return true;
}

bool widget_group_t::accepts_focus(void) {
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++) {
		if ((*iter)->accepts_focus())
			return true;
	}
	return false;
}

void widget_group_t::force_redraw(void) {
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++)
		(*iter)->force_redraw();
}

void widget_group_t::set_child_focus(window_component_t *target) {
	bool had_focus = impl->has_focus;
	impl->has_focus = true;
	for (int i = 0; i < (int) impl->children.size(); i++) {
		container_t *container;

		if (impl->children[i] == target || ((container = dynamic_cast<container_t *>(impl->children[i])) != NULL &&
				container->is_child(target)))
		{
			if (had_focus && i != impl->current_child)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_OUT);
			impl->current_child = i;

			if (impl->children[impl->current_child] == target)
				impl->children[impl->current_child]->set_focus(window_component_t::FOCUS_SET);
			else
				container->set_child_focus(target);
			return;
		}
	}
}

bool widget_group_t::is_child(window_component_t *component) {
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++) {
		container_t *container;
		if (*iter == component || ((container = dynamic_cast<container_t *>(*iter)) != NULL && container->is_child(component)))
			return true;
	}
	return false;
}

widget_t *widget_group_t::is_child_hotkey(key_t key) {
	widget_container_t *widget_container;
	widget_t *hotkey_child;
	for (widgets_t::iterator iter = impl->children.begin(); iter != impl->children.end(); iter++) {
		if ((*iter)->is_hotkey(key))
			return (*iter);

		widget_container = dynamic_cast<widget_container_t *>(*iter);
		if ((hotkey_child = widget_container->is_child_hotkey(key)) != NULL)
			return hotkey_child;
	}
	return NULL;
}


}; // namespace
