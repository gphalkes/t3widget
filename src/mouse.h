/* Copyright (C) 2012,2018 G.P. Halkes
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
#ifndef T3_WIDGET_MOUSE_H
#define T3_WIDGET_MOUSE_H

#include <t3widget/widget_api.h>
#include <t3window/window.h>

namespace t3_widget {

/** Structure holding the relevant elements of a mouse event. */
struct mouse_event_t {
  t3_window_t *window;
  short type, x, y, previous_button_state, button_state, modifier_state;
};

enum {
  EMOUSE_BUTTON_LEFT = (1 << 0),
  EMOUSE_BUTTON_MIDDLE = (1 << 1),
  EMOUSE_BUTTON_RIGHT = (1 << 2),
  EMOUSE_SCROLL_UP = (1 << 3),
  EMOUSE_SCROLL_DOWN = (1 << 4),
  EMOUSE_CLICKED_LEFT = (1 << 5),
  EMOUSE_CLICKED_MIDDLE = (1 << 6),
  EMOUSE_CLICKED_RIGHT = (1 << 7),
  EMOUSE_DOUBLE_CLICKED_LEFT = (1 << 8),
  EMOUSE_DOUBLE_CLICKED_MIDDLE = (1 << 9),
  EMOUSE_DOUBLE_CLICKED_RIGHT = (1 << 10),
  EMOUSE_TRIPLE_CLICKED_LEFT = (1 << 11),
  EMOUSE_TRIPLE_CLICKED_MIDDLE = (1 << 12),
  EMOUSE_TRIPLE_CLICKED_RIGHT = (1 << 13),
  EMOUSE_ALL_BUTTONS = EMOUSE_BUTTON_LEFT | EMOUSE_BUTTON_MIDDLE | EMOUSE_BUTTON_RIGHT |
                       EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN,
  EMOUSE_ALL_BUTTONS_COUNT = 5,
  EMOUSE_CLICK_BUTTONS_COUNT = 3,
  EMOUSE_CLICK_BUTTONS = EMOUSE_BUTTON_LEFT | EMOUSE_BUTTON_MIDDLE | EMOUSE_BUTTON_RIGHT,
};

enum {
  EMOUSE_SHIFT = (1 << 0),
  EMOUSE_META = (1 << 1),
  EMOUSE_CTRL = (1 << 2),
};

enum {
  EMOUSE_BUTTON_PRESS,
  EMOUSE_BUTTON_RELEASE,
  EMOUSE_MOTION,

  /* Bit to set when reporting events outside the grabing window. */
  EMOUSE_OUTSIDE_GRAB = (1 << 14)
};

/** Retrieve a mouse event from the input queue. */
T3_WIDGET_API mouse_event_t read_mouse_event();
}  // namespace
#endif
