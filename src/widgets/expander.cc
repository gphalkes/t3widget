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

using namespace std;
namespace t3_widget {

expander_t::expander_t(const char *_text) : is_expanded(false), label(_text), child(NULL), full_height(2) {
	init_unbacked_window(1, label.get_width() + 2);
	label.set_position(0, 2);
	set_widget_parent(&label);
	label.set_accepts_focus(true);
	if ((symbol_window = t3_win_new(window, 1, 2, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(symbol_window);
}

void expander_t::set_child(widget_t *_child) {
	/* FIXME: connect to move_focus_XXX events. */
	if (child == NULL && is_expanded) {
		t3_win_resize(window, 1, t3_win_get_width(window));
		is_expanded = false;
		redraw = true;
	}
	child = _child;
	set_widget_parent(child);
	child->set_anchor(this, 0);
	child->set_position(1, 0);
	child->set_size(full_height - 1, t3_win_get_width(window));
}

bool expander_t::process_key(key_t key) {
	if (focus == FOCUS_SELF) {
		if (is_expanded && child != NULL && (key == '\t' || key == EKEY_DOWN)) {
			if (child->accepts_focus()) {
				focus = FOCUS_CHILD;
				child->set_focus(window_component_t::FOCUS_IN_FWD);
				label.set_focus(window_component_t::FOCUS_OUT);
			}
			return true;
		} else if (key == EKEY_DOWN && !is_expanded) {
			move_focus_down();
		} else if (key == EKEY_UP) {
			move_focus_up();
		} else if (key == ' ' || key == EKEY_NL || key == EKEY_HOTKEY) {
			if (!is_expanded && child != NULL) {
				t3_win_resize(window, full_height, t3_win_get_width(window));
				is_expanded = true;
				redraw = true;
				child->show();
				if (child->accepts_focus()) {
					focus = FOCUS_CHILD;
					child->set_focus(window_component_t::FOCUS_SET);
					label.set_focus(window_component_t::FOCUS_OUT);
				}
			} else if (is_expanded) {
				if (child != NULL)
					child->hide();
				t3_win_resize(window, 1, t3_win_get_width(window));
				is_expanded = false;
				redraw = true;
			}
			return true;
		}
	} else if (focus == FOCUS_CHILD) {
		bool result = child->process_key(key);
		if (!result && key == (EKEY_SHIFT | '\t')) {
			focus = FOCUS_SELF;
			label.set_focus(window_component_t::FOCUS_SET);
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

	label.update_contents();
	t3_win_set_paint(symbol_window, 0, 0);
	t3_win_set_default_attrs(symbol_window, T3_ATTR_ACS | (focus == FOCUS_SELF ? attributes.dialog_selected : attributes.dialog));
	t3_win_addch(symbol_window, is_expanded ? T3_ACS_DARROW : T3_ACS_RARROW, 0);
}

void expander_t::set_focus(focus_t _focus) {
	/* FIXME: can we somehow determine where the focus came from, such that when
	   we shift-tab into this, we can set the focus to the child iso ourselves. */
	if (_focus == window_component_t::FOCUS_OUT) {
		if (focus == FOCUS_CHILD && child != NULL)
			child->set_focus(window_component_t::FOCUS_OUT);
		label.set_focus(window_component_t::FOCUS_OUT);
		focus = FOCUS_NONE;
	} else {
		if (_focus == window_component_t::FOCUS_SET || _focus == window_component_t::FOCUS_IN_FWD || child == NULL || !is_expanded) {
			focus = FOCUS_SELF;
			label.set_focus(window_component_t::FOCUS_SET);
		} else {
			focus = FOCUS_CHILD;
			child->set_focus(_focus);
		}
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
	return label.is_hotkey(key);
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
	container_t *container;
	if (target == child) {
		focus = FOCUS_CHILD;
		child->set_focus(window_component_t::FOCUS_SET);
	}
	container = dynamic_cast<container_t *>((widget_t *) child);
	if (container != NULL)
		container->focus_set(target);
}

bool expander_t::is_child(window_component_t *component) {
	container_t *container;
	if (component == child)
		return true;
	container = dynamic_cast<container_t *>((widget_t *) child);
	return container != NULL && container->is_child(component);
}

}; // namespace
