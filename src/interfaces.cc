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

bool container_t::set_widget_parent(window_component_t *widget) {
	return t3_win_set_parent(widget->get_base_window(), window);
}

void container_t::unset_widget_parent(window_component_t *widget) {
	t3_win_set_parent(widget->get_base_window(), widget_t::default_parent);
}

center_component_t::center_component_t(void) : center_window(this) {}
void center_component_t::set_center_window(window_component_t *_center_window) { center_window = _center_window; }

mouse_target_t::mouse_target_map_t mouse_target_t::targets;
mouse_target_t *mouse_target_t::grab_target;
t3_window_t *mouse_target_t::grab_window;

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

void mouse_target_t::unregister_mouse_target(t3_window_t *target) {
	targets.erase(target);
}

mouse_target_t::~mouse_target_t(void) {
	/* Can't use iter anymore after erasing, so we have to start our search all over
	   once we have found ourselves. */
start_over:
	for (mouse_target_map_t::iterator iter = targets.begin(); iter != targets.end(); iter++) {
		if (iter->second == this) {
			targets.erase(iter);
			goto start_over;
		}
	}

	if (grab_target == this)
		grab_target = NULL;
}

void mouse_target_t::grab_mouse(void) {
	if (grab_target != NULL)
		return;

	for (mouse_target_map_t::iterator iter = targets.begin(); iter != targets.end(); iter++) {
		if (iter->second == this) {
			grab_target = this;
			grab_window = iter->first;
			return;
		}
	}
}

void mouse_target_t::release_mouse_grab(void) {
	if (grab_target == this)
		grab_target = NULL;
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
	static int button_down_x, button_down_y;
	/* Information for handling double clicks. */
	static struct {
		t3_window_t *win;
		int x, y;
		struct timeval time;
		short buttons;
		bool was_double;
	} last_click = { NULL, 0, 0, { 0, 0 }, 0, false };

	bool handled = false;
	t3_window_t *win;
	mouse_target_map_t::iterator iter;
	dialog_t *active_dialog;

	win = t3_win_at_location(event.y, event.x);

	if (event.type == EMOUSE_BUTTON_PRESS && event.previous_button_state == 0 && (event.button_state & 7) != 0) {
		button_down_win = win;
		button_down_x = event.x;
		button_down_y = event.y;
		if (button_down_win != last_click.win)
			last_click.win = NULL;
	} else if (event.type == EMOUSE_BUTTON_RELEASE) {
		if (button_down_win == win && button_down_x == event.x && button_down_y == event.y) {
			struct timeval now;
			/* Set CLICKED event for released buttons */
			event.button_state |= (event.previous_button_state & EMOUSE_CLICK_BUTTONS) << EMOUSE_ALL_BUTTONS_COUNT;

			gettimeofday(&now, NULL);
			//FIXME: make DOUBLECLICK_TIMEOUT configurable
			if (last_click.win == button_down_win && timediff(now, last_click.time) < DOUBLECLICK_TIMEOUT &&
					(last_click.buttons & event.button_state) != 0 && last_click.x == event.x && last_click.y == event.y)
			{
				/* Set DOUBLE_CLICKED or TRIPLE_CLICKED event for appropriate buttons */
				event.button_state |= (event.button_state & last_click.buttons) <<
					(EMOUSE_CLICK_BUTTONS_COUNT * (last_click.was_double ? 2 : 1));
				if (last_click.was_double) {
					last_click.win = NULL;
				} else {
					last_click.was_double = !last_click.was_double;
					last_click.time = now;
				}
			} else {
				last_click.win = button_down_win;
				last_click.time = now;
				last_click.buttons = event.button_state & (EMOUSE_CLICK_BUTTONS << EMOUSE_ALL_BUTTONS_COUNT);
				last_click.x = event.x;
				last_click.y = event.y;
				last_click.was_double = false;
			}
		} else {
			last_click.win = NULL;
		}

		win = button_down_win;
		button_down_win = NULL;
	} else if (button_down_win != NULL) {
		win = button_down_win;
	}

	/* Set the window member of the event. In principle this is simply the window
	   visible at the location the event happened. However, button event handling
	   above may have changed this. */
	event.window = win;

	active_dialog = dialog_t::active_dialogs.back();

	//FIXME: should notify dialog of outside-dialog clicks
	while (win != NULL) {
		iter = targets.find(win);
		if (iter != targets.end()) {
			mouse_event_t local_event = event;

			if (grab_target == NULL) {
				if (iter->second != NULL && !active_dialog->is_child(iter->second))
					return handled;
			} else {
				container_t *grab_container = dynamic_cast<container_t *>(grab_target);
				if (((grab_container != NULL) && (iter->second == NULL || (!grab_container->is_child(iter->second) &&
						(window_component_t *) grab_container != iter->second))) ||
						(grab_container == NULL && grab_target != iter->second))
				{
					local_event.type |= EMOUSE_OUTSIDE_GRAB;
					local_event.x -= t3_win_get_abs_x(grab_window);
					local_event.y -= t3_win_get_abs_y(grab_window);
					return handled | grab_target->process_mouse_event(local_event);
				}
			}

			local_event.x -= t3_win_get_abs_x(win);
			local_event.y -= t3_win_get_abs_y(win);
			if (iter->second->process_mouse_event(local_event)) {
				/* If the active dialog has not changed by processing the event,
				   and the event is a button press, we should focus the widget that
				   received the event. */
				if (!handled && iter->second != NULL && active_dialog == dialog_t::active_dialogs.back() &&
						event.type == EMOUSE_BUTTON_PRESS && event.previous_button_state == 0 &&
						(event.button_state & EMOUSE_ALL_BUTTONS) != 0)
					active_dialog->set_child_focus(iter->second);
				handled = true;
				/* Stop handling if the dialog is no longer active. This happens for
				   for example when the active dialog is closed by the button, or worse,
				   deleted. In the latter case, trying to go up the hierarchy will
				   access free'd memory, which may cause a segfault. */
				if (active_dialog != dialog_t::active_dialogs.back())
					return handled;
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
