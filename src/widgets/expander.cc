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
#include "widgets/expander.h"
#include "colorscheme.h"
#include "log.h"

using namespace std;
namespace t3_widget {

expander_t::expander_t(const char *_text) : is_expanded(false), child_hotkey(false), label(_text), child(NULL), full_height(2) {
	init_unbacked_window(1, label.get_width() + 2);
	if ((symbol_window = t3_win_new(window, 1, 2 + label.get_width(), 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(symbol_window);
	register_mouse_target(symbol_window);
}

void expander_t::focus_up_from_child(void) {
	if (focus != FOCUS_CHILD || child == NULL)
		return;
	child->set_focus(window_component_t::FOCUS_OUT);
	focus = FOCUS_SELF;
	redraw = true;
}

void expander_t::set_child(widget_t *_child) {
	focus_widget_t *focus_child;
	/* FIXME: connect to move_focus_XXX events. (requires dynamic_cast'ing to focus_widget_t) */
	if (child != NULL) {
		unset_widget_parent(child);
		move_up_connection.disconnect();
		move_down_connection.disconnect();
		move_right_connection.disconnect();
		move_left_connection.disconnect();
	}

	if (_child == NULL) {
		if (is_expanded) {
			t3_win_resize(window, 1, t3_win_get_width(window));
			is_expanded = false;
			redraw = true;
			expanded(false);
		}
		child = _child;
		return;
	}
	child = _child;
	set_widget_parent(child);
	child->set_anchor(this, 0);
	child->set_position(1, 0);
	child->set_size(full_height - 1, t3_win_get_width(window));
	focus_child = dynamic_cast<focus_widget_t *>(child());
	if (focus_child != NULL) {
		move_up_connection = focus_child->connect_move_focus_up(sigc::mem_fun(this, &expander_t::focus_up_from_child));
		move_down_connection = focus_child->connect_move_focus_down(move_focus_down.make_slot());
		move_right_connection = focus_child->connect_move_focus_right(move_focus_right.make_slot());
		move_left_connection = focus_child->connect_move_focus_left(move_focus_left.make_slot());
	}
}

void expander_t::collapse(void) {
	if (is_expanded) {
		if (focus == FOCUS_CHILD) {
			child->set_focus(window_component_t::FOCUS_OUT);
			focus = FOCUS_SELF;
		}
		child->hide();
		is_expanded = false;
		t3_win_resize(window, 1, t3_win_get_width(window));
		redraw = true;
	}
}

bool expander_t::process_key(key_t key) {
	if (focus == FOCUS_SELF) {
		if (is_expanded && child != NULL && (key == '\t' || key == EKEY_DOWN)) {
			if (child->accepts_focus()) {
				focus = FOCUS_CHILD;
				child->set_focus(window_component_t::FOCUS_IN_FWD);
			}
			return true;
		} else if (key == EKEY_DOWN && !is_expanded) {
			move_focus_down();
		} else if (key == EKEY_UP) {
			move_focus_up();
		} else if (key == EKEY_RIGHT) {
			move_focus_right();
		} else if (key == EKEY_LEFT) {
			move_focus_left();
		} else if (key == ' ' || key == EKEY_NL || key == EKEY_HOTKEY) {
			if (!is_expanded && child != NULL) {
				t3_win_resize(window, full_height, t3_win_get_width(window));
				is_expanded = true;
				redraw = true;
				child->show();
				if (child->accepts_focus()) {
					focus = FOCUS_CHILD;
					child->set_focus(window_component_t::FOCUS_SET);
				}
				expanded(true);
			} else if (is_expanded) {
				if (child != NULL)
					child->hide();
				t3_win_resize(window, 1, t3_win_get_width(window));
				is_expanded = false;
				redraw = true;
				expanded(false);
			}
			return true;
		}
	} else if (focus == FOCUS_CHILD) {
		bool result = child->process_key(key);
		if (!result && key == (EKEY_SHIFT | '\t')) {
			focus = FOCUS_SELF;
			result = true;
			child->set_focus(window_component_t::FOCUS_OUT);
		}
		return result;
	}
	return false;
}

void expander_t::update_contents(void) {
	if (is_expanded && child != NULL)
		child->update_contents();
	if (!redraw)
		return;

	t3_win_set_paint(symbol_window, 0, 0);
	t3_win_set_default_attrs(symbol_window, (focus == FOCUS_SELF ? attributes.dialog_selected : attributes.dialog));
	t3_win_addch(symbol_window, is_expanded ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS);
	t3_win_addch(symbol_window, ' ', 0);
	label.draw(symbol_window, 0, true);
}

void expander_t::set_focus(focus_t _focus) {
	if (_focus == window_component_t::FOCUS_OUT) {
		if (focus == FOCUS_CHILD && child != NULL)
			child->set_focus(window_component_t::FOCUS_OUT);
		focus = FOCUS_NONE;
	} else if ((_focus == window_component_t::FOCUS_SET && !child_hotkey) || _focus == window_component_t::FOCUS_IN_FWD || child == NULL || !is_expanded) {
		focus = FOCUS_SELF;
	} else {
		child_hotkey = false;
		focus = FOCUS_CHILD;
		child->set_focus(_focus);
	}
	redraw = true;
}

bool expander_t::set_size(optint height, optint width) {
	bool result = true;
	if (height.is_valid() && height > 1) {
		full_height = height;
	}

	if (child != NULL)
		result &= child->set_size(full_height - 1, width);

	if (!width.is_valid())
		width = t3_win_get_width(window);
	if (is_expanded)
		result &= t3_win_resize(window, full_height, width);
	else
		result &= t3_win_resize(window, 1, width);
	return result;
}

bool expander_t::is_hotkey(key_t key) {
	if (label.is_hotkey(key))
		return true;
	if (is_expanded && child != NULL && child->is_hotkey(key)) {
		child_hotkey = true;
		return true;
	}
	return false;
}

void expander_t::set_enabled(bool enable) {
	enabled = enable;
	if (child != NULL)
		child->set_enabled(enable);
}

void expander_t::force_redraw(void) {
	if (child != NULL)
		child->force_redraw();
}

void expander_t::focus_set(window_component_t *target) {
	if (target == this) {
		focus = FOCUS_SELF;
	} else if (target == child) {
		focus = FOCUS_CHILD;
		child->set_focus(window_component_t::FOCUS_SET);
	} else {
		container_t * container = dynamic_cast<container_t *>((widget_t *) child);
		if (container != NULL) {
			focus = FOCUS_CHILD;
			container->focus_set(target);
		}
	}
}

bool expander_t::is_child(window_component_t *component) {
	container_t *container;
	if (component == child)
		return true;
	container = dynamic_cast<container_t *>((widget_t *) child);
	return container != NULL && container->is_child(component);
}

bool expander_t::process_mouse_event(mouse_event_t event) {
	if (event.button_state & EMOUSE_CLICKED_LEFT) {
		if (!is_expanded && child != NULL) {
			t3_win_resize(window, full_height, t3_win_get_width(window));
			is_expanded = true;
			redraw = true;
			child->show();
			expanded(true);
			if (focus == FOCUS_SELF) {
				focus = FOCUS_CHILD;
				child->set_focus(window_component_t::FOCUS_SET);
			}
		} else if (is_expanded) {
			/* No need to handle focus, because we got a set_focus(SET) event
			   before this call anyway. */
			if (child != NULL)
				child->hide();
			t3_win_resize(window, 1, t3_win_get_width(window));
			is_expanded = false;
			redraw = true;
			expanded(false);
		}
	}
	return true;
}

}; // namespace
