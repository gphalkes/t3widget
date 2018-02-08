/* Copyright (C) 2013,2018 G.P. Halkes
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
#include "popup.h"
#include "dialog.h"

namespace t3_widget {

/** Base class for dialogs. */
popup_t::popup_t(int height, int width, bool shadow, bool _draw)
    : dialog_base_t(height, width, shadow), draw(_draw), shown(false) {
  t3_win_set_depth(window, INT_MIN);
  if (shadow) {
    t3_win_set_depth(shadow_window, INT_MIN + 1);
  }
}

bool popup_t::process_key(key_t key) {
  if ((*current_widget)->process_key(key)) {
    return true;
  }

  if (key == EKEY_ESC) {
    hide();
    return true;
  }
  return false;
}

void popup_t::update_contents() {
  if (!draw) {
    redraw = false;
  }
  dialog_base_t::update_contents();
}

void popup_t::hide() {
  shown = false;
  dialog_base_t::hide();
  dialog_t::set_active_popup(nullptr);
  release_mouse_grab();
}

void popup_t::show() {
  shown = true;
  dialog_base_t::show();
  dialog_t::set_active_popup(this);
  grab_mouse();
  set_focus(FOCUS_SET);
  redraw = true;
}

bool popup_t::process_mouse_event(mouse_event_t event) {
  if ((event.type & EMOUSE_OUTSIDE_GRAB) &&
      (event.type & ~EMOUSE_OUTSIDE_GRAB) == EMOUSE_BUTTON_RELEASE) {
    hide();
  }
  return true;
}

bool popup_t::is_shown() { return shown; }

}  // namespace
