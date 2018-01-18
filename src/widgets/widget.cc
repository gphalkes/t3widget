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
#include "widgets/widget.h"
#include "main.h"
#include "log.h"

namespace t3_widget {

/* The default_parent must exist before any widgets are created. Thus using the
   #on_init method won't work. Instead we use a cleanup_t3_window.
*/
cleanup_t3_window_ptr widget_t::default_parent(t3_win_new_unbacked(nullptr, 1, 1, 0, 0, 0));

bool widget_t::is_hotkey(key_t key) {
	(void) key;
	return false;
}

bool widget_t::accepts_focus() { return enabled && shown; }

widget_t::widget_t(int height, int width, bool register_as_mouse_target) : redraw(true), enabled(true), shown(true) {
	init_window(height, width, register_as_mouse_target);
}

widget_t::widget_t() : redraw(true), enabled(true), shown(true) {}

void widget_t::init_window(int height, int width, bool register_as_mouse_target) {
	if ((window = t3_win_new(default_parent, height, width, 0, 0, 0)) == nullptr)
		throw std::bad_alloc();
	t3_win_show(window);
	if (register_as_mouse_target)
		register_mouse_target(window);
}

void widget_t::init_unbacked_window(int height, int width, bool register_as_mouse_target) {
	if ((window = t3_win_new_unbacked(default_parent, height, width, 0, 0, 0)) == nullptr)
		throw std::bad_alloc();
	t3_win_show(window);
	if (register_as_mouse_target)
		register_mouse_target(window);
}

void widget_t::set_anchor(window_component_t *anchor, int relation) {
	t3_win_set_anchor(window, anchor->get_base_window(), relation);
}

void widget_t::set_position(optint top, optint left) {
	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);

	t3_win_move(window, top, left);
}

void widget_t::show() {
	t3_win_show(window);
	shown = true;
}

void widget_t::hide() {
	t3_win_hide(window);
	shown = false;
}

void widget_t::force_redraw() {
	redraw = true;
}

void widget_t::set_enabled(bool enable) {
	enabled = enable;
}

bool widget_t::is_enabled() {
	return enabled;
}

bool widget_t::is_shown() {
	return shown;
}

void widget_t::set_focus(focus_t focus) { (void) focus; return; }

bool widget_t::process_mouse_event(mouse_event_t event) {
	lprintf("Default mouse handling for %s (%d)\n", typeid(*this).name(), accepts_focus());
	return accepts_focus() && (event.button_state & EMOUSE_CLICK_BUTTONS);
}

}; // namespace
