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

struct popup_t::implementation_t {
  bool draw;
  bool shown = false;

  implementation_t(bool _draw) : draw(_draw) {}
};

/** Base class for dialogs. */
popup_t::popup_t(int height, int width, bool shadow, bool _draw)
    : dialog_base_t(height, width, shadow, impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>(_draw)) {
  set_depth(INT_MIN);
}

popup_t::~popup_t() {}

bool popup_t::process_key(key_t key) {
  if (get_current_widget()->process_key(key)) {
    return true;
  }

  if (key == EKEY_ESC) {
    hide();
    return true;
  }
  return false;
}

void popup_t::update_contents() {
  if (!impl->draw) {
    set_redraw(false);
  }
  dialog_base_t::update_contents();
}

void popup_t::hide() {
  impl->shown = false;
  dialog_base_t::hide();
  dialog_t::set_active_popup(nullptr);
  release_mouse_grab();
}

void popup_t::show() {
  impl->shown = true;
  dialog_base_t::show();
  dialog_t::set_active_popup(this);
  grab_mouse();
  set_focus(FOCUS_SET);
  set_redraw(true);
}

bool popup_t::process_mouse_event(mouse_event_t event) {
  if ((event.type & EMOUSE_OUTSIDE_GRAB) &&
      (event.type & ~EMOUSE_OUTSIDE_GRAB) == EMOUSE_BUTTON_RELEASE) {
    hide();
  }
  return true;
}

bool popup_t::is_shown() { return impl->shown; }

}  // namespace
