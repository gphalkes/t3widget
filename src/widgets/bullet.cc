/* Copyright (C) 2011-2012,2018 G.P. Halkes
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
#include "t3widget/widgets/bullet.h"

#include <type_traits>

#include "t3widget/colorscheme.h"
#include "t3widget/widgets/widget.h"
#include "t3window/terminal.h"
#include "t3window/window.h"

namespace t3widget {

struct bullet_t::implementation_t {
  /** Callback to determine required display state. */
  std::function<bool()> source;
  /** Boolean indicating whether this widget should be drawn as focuessed. */
  bool has_focus = false;
  implementation_t(std::function<bool()> _source) : source(_source) {}
};

bullet_t::bullet_t(std::function<bool()> _source)
    : widget_t(1, 1, false, impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>(_source)) {}

bullet_t::~bullet_t() {}

bool bullet_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

bool bullet_t::process_key(key_t key) {
  (void)key;
  return false;
}

void bullet_t::update_contents() {
  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  if (impl->source()) {
    window.addch(T3_ACS_DIAMOND, T3_ATTR_ACS | (impl->has_focus ? T3_ATTR_REVERSE : 0));
  } else {
    window.addch(' ', impl->has_focus ? T3_ATTR_REVERSE : 0);
  }
}

void bullet_t::set_focus(focus_t focus) { impl->has_focus = focus; }

}  // namespace t3widget
