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
#include <sys/time.h>

#include "interfaces.h"
#include "widgets/widget.h"
#include "main.h"
#include "log.h"

using namespace std;

namespace t3_widget {

window_component_t::window_component_t(void) : window(NULL) {}
window_component_t::~window_component_t(void) {}
t3_window_t *window_component_t::get_base_window(void) { return window; }

bool container_t::set_widget_parent(widget_t *widget) {
	return t3_win_set_parent(widget->get_base_window(), window);
}

void container_t::unset_widget_parent(widget_t *widget) {
	t3_win_set_parent(widget->get_base_window(), widget_t::default_parent);
}

center_component_t::center_component_t(void) : center_window(this) {}
void center_component_t::set_center_window(window_component_t *_center_window) { center_window = _center_window; }

mouse_target_t::mouse_target_map_t mouse_target_t::targets;

mouse_target_t::mouse_target_t(bool use_window) {
	if (use_window && window != NULL)
		register_mouse_target(window);
}

void mouse_target_t::register_mouse_target(t3_window_t *target) {
	if (target == NULL)
		lprintf("Registering mouse target for NULL window in %s\n", typeid(*this).name());
	else
		targets[target] = this;
}

mouse_target_t::~mouse_target_t(void) {
	for (mouse_target_map_t::iterator iter = targets.begin(); iter != targets.end(); ) {
		/* Can't use iter anymore after erasing, so we have to start our search all over
		   once we have found ourselves. */
		for (iter = targets.begin(); iter != targets.end(); iter++) {
			if (iter->second == this) {
				targets.erase(iter);
				break;
			}
		}
	}
}

static long timediff(struct timeval a, struct timeval b) {
	long result = a.tv_sec - b.tv_sec;
	if (result > 10)
		return 10 * 1000000;
	return result * 1000000 + (a.tv_usec - b.tv_usec);
}
#define DOUBLECLICK_TIMEOUT 200000

bool mouse_target_t::handle_mouse_event(mouse_event_t event) {
	static t3_window_t *button_down_win = NULL;
	/* Information for handling double clicks. */
	static t3_window_t *last_click_win = NULL;
	static struct timeval last_click_time = { 0, 0 };
	static short last_click_buttons = 0;

	bool handled = false;
	t3_window_t *win;
	mouse_target_map_t::iterator iter;
	dialog_t *active_dialog;
	mouse_target_t *active_dialog_target;

	win = event.window;
	if (event.type == EMOUSE_BUTTON_PRESS && event.previous_button_state == 0 && (event.button_state & 7) != 0) {
		button_down_win = event.window;
		if (button_down_win != last_click_win)
			last_click_win = NULL;
	} else if (event.type == EMOUSE_BUTTON_RELEASE) {
		if (button_down_win == win) {
			struct timeval now;
			/* Set CLICKED event for released buttons */
			event.button_state |= (event.previous_button_state & 7) << 5;

			gettimeofday(&now, NULL);
			//FIXME: make DOUBLECLICK_TIMEOUT configurable
			if (last_click_win == button_down_win && timediff(now, last_click_time) < DOUBLECLICK_TIMEOUT &&
					(last_click_buttons & event.button_state) != 0)
			{
				/* Set DOUBLE_CLICKED event for appropriate buttons */
				event.button_state |= (event.button_state & last_click_buttons) << 3;
				last_click_win = NULL;
			} else {
				last_click_win = button_down_win;
				last_click_time = now;
				last_click_buttons = event.button_state & (7 << 5);
			}
		} else {
			last_click_win = NULL;
		}

		win = button_down_win;
		button_down_win = NULL;
	} else if (button_down_win != NULL) {
		win = button_down_win;
	}

	active_dialog = dialog_t::active_dialogs.back();
	active_dialog_target = dynamic_cast<mouse_target_t *>(active_dialog);

	/* FIXME: events should be passed to parents as well, but marked. Same goes
	   for events that were not caught by any children. */
	//FIXME: should notify dialog of outside-dialog clicks
	while (win != NULL) {
		iter = targets.find(win);
		if (iter != targets.end()) {
			widget_t *widget = dynamic_cast<widget_t *>(iter->second);

			if (iter->second != active_dialog_target) {
				if (widget == NULL)
					continue;

				if (!active_dialog->is_child(widget))
					return false;
			}

			mouse_event_t local_event = event;
			local_event.x -= t3_win_get_abs_x(win);
			local_event.y -= t3_win_get_abs_y(win);
			if (iter->second->process_mouse_event(local_event)) {
				/* If the active dialog has not changed by processing the event,
				   and the event is a button press, we should focus the widget that
				   received the event. */
				if (!handled && widget != NULL && active_dialog == dialog_t::active_dialogs.back() &&
						event.type == EMOUSE_BUTTON_PRESS && event.previous_button_state == 0 &&
						(event.button_state & 7) != 0)
					active_dialog->focus_set(widget);
				handled = true;
			}
		}
		win = t3_win_get_parent(win);
	}
	return handled;
}

list<bad_draw_recheck_t *> bad_draw_recheck_t::to_signal;
sigc::connection bad_draw_recheck_t::initialized =
	connect_terminal_settings_changed(sigc::ptr_fun(bad_draw_recheck_t::bad_draw_recheck_all));

void bad_draw_recheck_t::bad_draw_recheck_all(void) {
	for (list<bad_draw_recheck_t *>::iterator iter = to_signal.begin(); iter != to_signal.end(); iter++)
		(*iter)->bad_draw_recheck();
}

bad_draw_recheck_t::bad_draw_recheck_t(void) {
	to_signal.push_back(this);
}

bad_draw_recheck_t::~bad_draw_recheck_t(void) {
	for (list<bad_draw_recheck_t *>::iterator iter = to_signal.begin(); iter != to_signal.end(); iter++) {
		if (*iter == this) {
			to_signal.erase(iter);
			return;
		}
	}
}

}; // namespace
