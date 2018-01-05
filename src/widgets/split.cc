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
#include <algorithm>
#include <cstring>
#include "widgets/split.h"
#include "internal.h"

using namespace std;

namespace t3_widget {

signals::connection split_t::init_connected = connect_on_init(signals::ptr_fun(split_t::init));
std::map<key_t, split_t::Action> split_t::key_bindings;

static const char *action_names[] = {
#define _T3_ACTION(action, name) name,
#include <t3widget/widgets/split.actions.h>
#undef _T3_ACTION
};

void split_t::init(bool _init) {
	if (_init) {
		key_bindings[EKEY_F8] = ACTION_NEXT_SPLIT;
		key_bindings[EKEY_META | '8'] = ACTION_NEXT_SPLIT;
		key_bindings[EKEY_F8 | EKEY_SHIFT] = ACTION_PREVIOUS_SPLIT;
	}
}

split_t::split_t(widget_t *widget) : horizontal(true) {
	init_unbacked_window(3, 3);
	set_widget_parent(widget);
	widget->set_anchor(this, 0);
	widget->show();
	widgets.push_back(widget);
	current = widgets.begin();
}

split_t::~split_t(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		delete (*iter);
}

bool split_t::process_key(key_t key) {
	if (widgets.empty())
		return false;

	std::map<key_t, Action>::iterator iter = key_bindings.find(key);
	if (iter == key_bindings.end()) {
		return (*current)->process_key(key);
	}

	switch (iter->second) {
		case ACTION_NEXT_SPLIT:
			next();
			break;
		case ACTION_PREVIOUS_SPLIT:
			previous();
			break;
		default:
			break;
	}
	return true;
}

bool split_t::set_size(optint height, optint width) {
	bool result;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result = t3_win_resize(window, height, width);

	if (horizontal) {
		int idx;
		int step = height / widgets.size();
		int left_over = height % widgets.size();
		widgets_t::iterator iter;

		for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
			result &= (*iter)->set_size(step + (idx < left_over), width);
			(*iter)->set_position(idx * step + min(idx, left_over), 0);
		}
	} else {
		int idx;
		int step = width / widgets.size();
		int left_over = width % widgets.size();
		widgets_t::iterator iter;

		for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
			result &= (*iter)->set_size(height, step + (idx < left_over));
			(*iter)->set_position(0, idx * step + min(idx, left_over));
		}
	}
	return result;
}

void split_t::update_contents(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void split_t::set_focus(focus_t _focus) {
	focus = _focus;
	(*current)->set_focus(_focus);
}

void split_t::force_redraw(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->force_redraw();
}

void split_t::set_child_focus(window_component_t *target) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
		if (*iter == target) {
			if (*current != *iter) {
				(*current)->set_focus(window_component_t::FOCUS_OUT);
				current = iter;
				(*current)->set_focus(window_component_t::FOCUS_SET);
			}
			return;
		} else {
			container_t *container = dynamic_cast<container_t *>(*iter);
			if (container != NULL && container->is_child(target)) {
				if (*current != *iter) {
					(*current)->set_focus(window_component_t::FOCUS_OUT);
					current = iter;
				}
				container->set_child_focus(target);
				return;
			}
		}
	}
}

bool split_t::is_child(window_component_t *widget) {
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

void split_t::split(widget_t *widget, bool _horizontal) {
	split_t *current_window = dynamic_cast<split_t *>(*current);

	if (current_window != NULL) {
		current_window->split(widget, _horizontal);
	} else if (widgets.size() == 1 || _horizontal == horizontal) {
		horizontal = _horizontal;
		set_widget_parent(widget);
		widget->set_anchor(this, 0);
		widget->show();
		if (focus)
			(*current)->set_focus(window_component_t::FOCUS_OUT);
		current++;
		current = widgets.insert(current, widget);
		set_size(None, None);
		if (focus)
			(*current)->set_focus(window_component_t::FOCUS_SET);
	} else {
		/* Create a new split_t with the current widget as its contents. Then
		   add split that split_t to splice in the requested widget. */
		(*current)->set_focus(window_component_t::FOCUS_OUT);
		current_window = new split_t(*current);
		set_widget_parent(current_window);
		current_window->set_focus(window_component_t::FOCUS_SET);
		current_window->split(widget, _horizontal);
		*current = current_window;
		set_size(None, None);
	}
}

bool split_t::unsplit(widget_t **widget) {
	split_t *current_window = dynamic_cast<split_t *>(*current);

	if (current_window == NULL) {
		/* This should not happen for previously split windows. However, for
		   the first split_t instance this may be the case, so we have to
		   handle it. */
		if (widgets.size() == 1)
			return true;
		*widget = *current;
		current = widgets.erase(current);
		if (current == widgets.end()) {
			current--;
			if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
				current_window->set_to_end();
		} else {
			if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
				current_window->set_to_begin();
		}
		if (focus)
			(*current)->set_focus(window_component_t::FOCUS_SET);
		set_size(None, None);
		if (widgets.size() == 1)
			return true;
	} else {
		if (current_window->unsplit(widget)) {
			*current = current_window->widgets.front();
			set_widget_parent(*current);
			(*current)->set_anchor(this, 0);
			current_window->widgets.clear();
			delete current_window;
			if (focus)
				(*current)->set_focus(window_component_t::FOCUS_SET);
			set_size(None, None);
		}
	}
	return false;
}

widget_t *split_t::unsplit(void) {
	widget_t *result = NULL;
	unsplit(&result);
	return result;
}

bool split_t::next_recurse(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->next_recurse()) {
		(*current)->set_focus(window_component_t::FOCUS_OUT);
		current++;
		if (current != widgets.end()) {
			if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
				current_window->set_to_begin();
			if (focus)
				(*current)->set_focus(window_component_t::FOCUS_IN_FWD);
			return true;
		} else {
			current--;
			return false;
		}
	}
	return true;
}

bool split_t::previous_recurse(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->previous_recurse()) {
		(*current)->set_focus(window_component_t::FOCUS_OUT);
		if (current == widgets.begin())
			return false;
		current--;

		if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
			current_window->set_to_end();
		(*current)->set_focus(window_component_t::FOCUS_IN_BCK);
	}
	return true;
}

void split_t::next(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->next_recurse()) {
		(*current)->set_focus(window_component_t::FOCUS_OUT);
		current++;
		if (current == widgets.end())
			current = widgets.begin();

		if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
			current_window->set_to_begin();
		if (focus)
			(*current)->set_focus(window_component_t::FOCUS_IN_FWD);
	}
}

void split_t::previous(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->previous_recurse()) {
		(*current)->set_focus(window_component_t::FOCUS_OUT);
		if (current == widgets.begin())
			current = widgets.end();
		current--;

		if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
			current_window->set_to_end();
		if (focus)
			(*current)->set_focus(window_component_t::FOCUS_IN_BCK);
	}
}

widget_t *split_t::get_current(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL)
		return *current;
	else
		return current_window->get_current();
}

void split_t::set_to_begin(void) {
	split_t *current_window;

	current = widgets.begin();
	if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
		current_window->set_to_begin();
}

void split_t::set_to_end(void) {
	split_t *current_window;

	current = widgets.end();
	current--;
	if ((current_window = dynamic_cast<split_t *>(*current)) != NULL)
		current_window->set_to_end();
}

split_t::Action split_t::map_action_name(const char *name) {
	for (size_t i = 0; i < ARRAY_SIZE(action_names); ++i) {
		if (strcmp(name, action_names[i]) == 0) {
			return static_cast<Action>(i);
		}
	}
	return ACTION_NONE;
}

void split_t::bind_key(key_t key, Action action) {
	if (action == ACTION_NONE) {
		key_bindings.erase(key);
	} else {
		key_bindings[key] = action;
	}
}

std::vector<std::string> split_t::get_action_names() {
	std::vector<std::string> result;
	for (size_t i = 0; i < ARRAY_SIZE(action_names); ++i) {
		result.push_back(action_names[i]);
	}
	return result;
}

}; // namespace
