/* Copyright (C) 2011,2018 G.P. Halkes
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
#include "t3widget/widgets/dummywidget.h"

namespace t3_widget {

bool dummy_widget_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool dummy_widget_t::resize(optint height, optint width, optint top, optint left) {
  (void)height;
  (void)width;
  (void)top;
  (void)left;
  return true;
}

void dummy_widget_t::update_contents() { return; }

void dummy_widget_t::show() {}

void dummy_widget_t::hide() {}

void dummy_widget_t::set_position(optint top, optint left) {
  (void)top;
  (void)left;
}

bool dummy_widget_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

t3_window_t *dummy_widget_t::get_base_window() { return nullptr; }

};  // namespace
