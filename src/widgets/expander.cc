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

namespace t3_widget {

expander_t::expander_t(const char *text) : impl(new implementation_t(text)) {
	init_unbacked_window(1, impl->label.get_width() + 2);
	if ((impl->symbol_window = t3_win_new(window, 1, 2 + impl->label.get_width(), 0, 0, 0)) == NULL)
		throw std::bad_alloc();
	t3_win_show(impl->symbol_window);
	register_mouse_target(impl->symbol_window);
}

void expander_t::focus_up_from_child(void) {
	if (impl->focus != FOCUS_CHILD || impl->child == NULL)
		return;
	impl->child->set_focus(window_component_t::FOCUS_OUT);
	impl->focus = FOCUS_SELF;
	redraw = true;
}

void expander_t::set_child(widget_t *_child) {
	focus_widget_t *focus_child;
	/* FIXME: connect to move_focus_XXX events. (requires dynamic_cast'ing to focus_widget_t) */
	if (impl->child != NULL) {
		unset_widget_parent(impl->child);
		impl->move_up_connection.disconnect();
		impl->move_down_connection.disconnect();
		impl->move_right_connection.disconnect();
		impl->move_left_connection.disconnect();
	}

	if (_child == NULL) {
		if (impl->is_expanded) {
			t3_win_resize(window, 1, t3_win_get_width(window));
			impl->is_expanded = false;
			redraw = true;
			expanded(false);
		}
		impl->child = _child;
		return;
	}
	impl->child = _child;
	set_widget_parent(impl->child);
	impl->child->set_anchor(this, 0);
	impl->child->set_position(1, 0);
	impl->full_height = t3_win_get_height(impl->child->get_base_window()) + 1;
	focus_child = dynamic_cast<focus_widget_t *>(impl->child());
	if (focus_child != NULL) {
		impl->move_up_connection = focus_child->connect_move_focus_up(signals::mem_fun(this, &expander_t::focus_up_from_child));
		impl->move_down_connection = focus_child->connect_move_focus_down(move_focus_down.make_slot());
		impl->move_right_connection = focus_child->connect_move_focus_right(move_focus_right.make_slot());
		impl->move_left_connection = focus_child->connect_move_focus_left(move_focus_left.make_slot());
	}
	set_size(None, t3_win_get_width(impl->child->get_base_window()));
}

void expander_t::set_expanded(bool expand) {
	if (!expand && impl->is_expanded) {
		if (impl->focus == FOCUS_CHILD) {
			impl->child->set_focus(window_component_t::FOCUS_OUT);
			impl->focus = FOCUS_SELF;
		}
		impl->child->hide();
		impl->is_expanded = false;
		t3_win_resize(window, 1, t3_win_get_width(window));
		redraw = true;
		expanded(false);
	} else if (expand && !impl->is_expanded && impl->child != NULL) {
		impl->child->show();
		impl->is_expanded = true;
		t3_win_resize(window, impl->full_height, t3_win_get_width(window));
		redraw = true;
		expanded(true);
	}
}

bool expander_t::process_key(key_t key) {
	lprintf("Handling key %08x impl->focus: %d\n", key, impl->focus);
	if (impl->focus == FOCUS_SELF) {
		if (impl->is_expanded && impl->child != NULL && (key == '\t' || key == EKEY_DOWN)) {
			if (impl->child->accepts_focus()) {
				impl->focus = FOCUS_CHILD;
				impl->child->set_focus(window_component_t::FOCUS_IN_FWD);
			}
			return true;
		} else if (key == EKEY_DOWN && !impl->is_expanded) {
			move_focus_down();
		} else if (key == EKEY_UP) {
			move_focus_up();
		} else if (key == EKEY_RIGHT) {
			move_focus_right();
		} else if (key == EKEY_LEFT) {
			move_focus_left();
		} else if (key == ' ' || key == EKEY_NL || key == EKEY_HOTKEY) {
			if (!impl->is_expanded && impl->child != NULL) {
				t3_win_resize(window, impl->full_height, t3_win_get_width(window));
				impl->is_expanded = true;
				redraw = true;
				impl->child->show();
				if (impl->child->accepts_focus()) {
					impl->focus = FOCUS_CHILD;
					impl->child->set_focus(window_component_t::FOCUS_SET);
				}
				expanded(true);
			} else if (impl->is_expanded) {
				if (impl->child != NULL)
					impl->child->hide();
				t3_win_resize(window, 1, t3_win_get_width(window));
				impl->is_expanded = false;
				redraw = true;
				expanded(false);
			}
			return true;
		}
	} else if (impl->focus == FOCUS_CHILD) {
		bool result = impl->child->process_key(key);
		if (!result && key == (EKEY_SHIFT | '\t')) {
			impl->focus = FOCUS_SELF;
			result = true;
			impl->child->set_focus(window_component_t::FOCUS_OUT);
		}
		return result;
	}
	return false;
}

void expander_t::update_contents(void) {
	if (impl->is_expanded && impl->child != NULL)
		impl->child->update_contents();
	if (!redraw)
		return;

	t3_win_set_paint(impl->symbol_window, 0, 0);
	t3_win_set_default_attrs(impl->symbol_window, (impl->focus == FOCUS_SELF ? attributes.dialog_selected : attributes.dialog));
	t3_win_addch(impl->symbol_window, impl->is_expanded ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS);
	t3_win_addch(impl->symbol_window, ' ', 0);
	impl->label.draw(impl->symbol_window, 0, impl->focus == FOCUS_SELF);
}

void expander_t::set_focus(focus_t _focus) {
	if (_focus == window_component_t::FOCUS_OUT) {
		if (impl->focus == FOCUS_CHILD && impl->child != NULL)
			impl->child->set_focus(window_component_t::FOCUS_OUT);
		impl->last_focus = impl->focus;
		impl->focus = FOCUS_NONE;
	} else if (_focus == window_component_t::FOCUS_SET || _focus == window_component_t::FOCUS_IN_FWD ||
			(_focus == window_component_t::FOCUS_REVERT && impl->last_focus == FOCUS_SELF) || impl->child == NULL ||
			!impl->is_expanded) {
		impl->focus = FOCUS_SELF;
	} else {
		impl->focus = FOCUS_CHILD;
		impl->child->set_focus(_focus);
	}
	redraw = true;
}

bool expander_t::set_size(optint height, optint width) {
	bool result = true;
	if (height.is_valid() && height > 1) {
		impl->full_height = height;
	}

	if (impl->child != NULL)
		result &= impl->child->set_size(impl->full_height - 1, width);

	if (!width.is_valid())
		width = t3_win_get_width(window);
	if (impl->is_expanded)
		result &= t3_win_resize(window, impl->full_height, width);
	else
		result &= t3_win_resize(window, 1, width);
	return result;
}

bool expander_t::is_hotkey(key_t key) {
	return impl->label.is_hotkey(key);
}

void expander_t::set_enabled(bool enable) {
	enabled = enable;
	if (impl->child != NULL)
		impl->child->set_enabled(enable);
}

void expander_t::force_redraw(void) {
	if (impl->child != NULL)
		impl->child->force_redraw();
}

void expander_t::set_child_focus(window_component_t *target) {
	if (target == impl->child) {
		impl->focus = FOCUS_CHILD;
		impl->child->set_focus(window_component_t::FOCUS_SET);
	} else {
		container_t * container = dynamic_cast<container_t *>((widget_t *) impl->child);
		if (container != NULL) {
			impl->focus = FOCUS_CHILD;
			container->set_child_focus(target);
		}
	}
}

bool expander_t::is_child(window_component_t *component) {
	container_t *container;
	if (component == impl->child)
		return true;
	container = dynamic_cast<container_t *>(impl->child());
	return container != NULL && container->is_child(component);
}

widget_t *expander_t::is_child_hotkey(key_t key) {
	widget_container_t *widget_container;

	if (!impl->is_expanded || impl->child == NULL || !impl->child->is_shown() || !impl->child->is_enabled())
		return NULL;

	if (impl->child->is_hotkey(key))
		return impl->child;
	widget_container = dynamic_cast<widget_container_t *>(impl->child());
	return widget_container == NULL ? NULL : widget_container->is_child_hotkey(key);
}

bool expander_t::process_mouse_event(mouse_event_t event) {
	if (event.button_state & EMOUSE_CLICKED_LEFT) {
		if (!impl->is_expanded && impl->child != NULL) {
			t3_win_resize(window, impl->full_height, t3_win_get_width(window));
			impl->is_expanded = true;
			redraw = true;
			impl->child->show();
			expanded(true);
			if (impl->focus == FOCUS_SELF) {
				impl->focus = FOCUS_CHILD;
				impl->child->set_focus(window_component_t::FOCUS_SET);
			}
		} else if (impl->is_expanded) {
			/* No need to handle impl->focus, because we got a set_focus(SET) event
			   before this call anyway. */
			if (impl->child != NULL)
				impl->child->hide();
			t3_win_resize(window, 1, t3_win_get_width(window));
			impl->is_expanded = false;
			redraw = true;
			expanded(false);
		}
	}
	return true;
}

}; // namespace
