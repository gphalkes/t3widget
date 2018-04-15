/* Copyright (C) 2011-2013,2018 G.P. Halkes
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
#include "mainwindow.h"
#include "main.h"

namespace t3_widget {

/* The implementation struct currently is empty, so we don't even allocated it. But the pointer to
   it has been reserved in the class, such that if a new member is needed we can allocate it in the
   implementation without changing anything about the interface. */
struct main_window_base_t::implementation_t {};

main_window_base_t::main_window_base_t() : dialog_t() {
  int height, width;
  t3_term_get_size(&height, &width);

  /* If the main window was created before init, we may get funny results. To
     prevent crashes, correct them here. */
  if (height <= 0 || width <= 0) {
    height = 24;
    width = 80;
  }

  window.alloc(nullptr, height, width, 0, 0, INT_MAX);
  window.show();
  connect_resize(bind_front(&main_window_base_t::set_size_real, this));
}

main_window_base_t::~main_window_base_t() {}

bool main_window_base_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void main_window_base_t::set_position(optint top, optint left) {
  (void)top;
  (void)left;
}

void main_window_base_t::update_contents() {
  set_redraw(false);
  dialog_t::update_contents();
}

void main_window_base_t::show() {
  dialog_base_t::show();

  dialog_t::active_dialogs.push_front(this);
  if (dialog_t::active_dialogs.back() == this) {
    set_focus(window_component_t::FOCUS_SET);
  }
}

void main_window_base_t::close() {}

void main_window_base_t::set_size_real(int height, int width) {
  window.resize(height, width);
  set_size(height, width);
}

}  // namespace
