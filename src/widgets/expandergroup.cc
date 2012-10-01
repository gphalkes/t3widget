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
#include "widgets/expandergroup.h"
#include "log.h"

using namespace std;
namespace t3_widget {

expander_group_t::expander_group_t(void) : expanded_child(-1), has_focus(false) {
	init_unbacked_window(1, 1);
}

void expander_group_t::focus_next(void) {
	if (current_child + 1 != children.end()) {
		(*current_child)->set_focus(window_component_t::FOCUS_OUT);
		current_child++;
		(*current_child)->set_focus(window_component_t::FOCUS_IN_FWD);
	} else {
		move_focus_down();
	}
}

void expander_group_t::focus_previous(void) {
	if (current_child != children.begin()) {
		(*current_child)->set_focus(window_component_t::FOCUS_OUT);
		current_child--;
		(*current_child)->set_focus(window_component_t::FOCUS_IN_BCK);
	} else {
		move_focus_up();
	}
}

void expander_group_t::add_child(expander_t *child) {
	child->connect_expanded(sigc::bind(sigc::mem_fun(this, &expander_group_t::child_expanded), (int) children.size()));
	child->connect_move_focus_down(sigc::mem_fun(this, &expander_group_t::focus_next));
	child->connect_move_focus_up(sigc::mem_fun(this, &expander_group_t::focus_previous));
	set_widget_parent(child);

	if (!children.empty())
		child->set_anchor(children.back(), T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	child->set_position(0, 0);

	children.push_back(child);
	if (children.size() == 1 && has_focus)
		current_child = children.begin();
	child->set_size(None, t3_win_get_width(window));
	set_size(None, None);
}

void expander_group_t::child_expanded(bool is_expanded, int child_idx) {
	if (is_expanded) {
		if (expanded_child >= 0)
			children[expanded_child]->collapse();
		expanded_child = child_idx;
	} else {
		expanded_child = -1;
	}
	set_size(None, None);
	expanded(is_expanded);
}

expander_group_t::~expander_group_t(void) {
	for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++)
		delete *iter;
}

bool expander_group_t::process_key(key_t key) {
	if (!has_focus || children.size() == 0)
		return false;
	if ((*current_child)->process_key(key))
		return true;
	if (key == '\t') {
		if (current_child + 1 != children.end()) {
			if (has_focus)
				(*current_child)->set_focus(window_component_t::FOCUS_OUT);
			++current_child;
			if (has_focus)
				(*current_child)->set_focus(window_component_t::FOCUS_IN_FWD);
			return true;
		}
	} else if (key == (EKEY_SHIFT | '\t')) {
		if (current_child != children.begin()) {
			if (has_focus)
				(*current_child)->set_focus(window_component_t::FOCUS_OUT);
			--current_child;
			if (has_focus)
				(*current_child)->set_focus(window_component_t::FOCUS_IN_FWD);
			return true;
		}
	}
	return false;
}

void expander_group_t::update_contents(void) {
	for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++)
		(*iter)->update_contents();
}

void expander_group_t::set_focus(focus_t focus) {
	has_focus = focus != window_component_t::FOCUS_OUT && children.size() > 0;
	if (children.size() == 0)
		return;
	switch (focus) {
		case window_component_t::FOCUS_OUT:
			(*current_child)->set_focus(focus);
			break;
		case window_component_t::FOCUS_IN_FWD:
		case window_component_t::FOCUS_SET:
			for (current_child = children.begin(); !(*current_child)->accepts_focus() &&
				current_child != children.end(); ++current_child) {}
			(*current_child)->set_focus(focus);
			break;
		case window_component_t::FOCUS_IN_BCK: {
			expanders_t::reverse_iterator riter;
			for (riter = children.rbegin(); !(*riter)->accepts_focus() &&
					riter != children.rend(); ++riter) {}
			current_child = riter.base() - 1;
			(*current_child)->set_focus(focus);
			break;
		}
		default:
			break;
	}
}

bool expander_group_t::set_size(optint _height, optint width) {
	int height;
	(void) _height;

	if (!width.is_valid()) {
		width = t3_win_get_width(window);
	} else {
		for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++)
			(*iter)->set_size(None, width);
	}

	height = children.size();
	if (expanded_child >= 0) {
		height += t3_win_get_height(children[expanded_child]->get_base_window()) - 1;
		lprintf("Window height of child: %d\n", t3_win_get_height(children[expanded_child]->get_base_window()));
	}
	if (height == 0)
		height = 1;
	lprintf("Setting size to: %d, %d\n", height, (int) width);
	t3_win_resize(window, height, width);
	return true;
}

bool expander_group_t::accepts_focus(void) {
	for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++) {
		if ((*iter)->accepts_focus())
			return true;
	}
	return false;
}
//		virtual bool is_hotkey(key_t key);
//		virtual void set_enabled(bool enable);
//		virtual void force_redraw(void);
void expander_group_t::focus_set(window_component_t *target) {
	has_focus = true;
	for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++) {
		if ((*iter)->is_child(target)) {
			(*iter)->focus_set(target);
			return;
		}
	}
}

bool expander_group_t::is_child(window_component_t *component) {
	for (expanders_t::iterator iter = children.begin(); iter != children.end(); iter++) {
		if ((*iter)->is_child(component))
			return true;
	}
	return false;
}


}; // namespace
