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
#include "widgets/expandergroup.h"
#include "internal.h"
#include "log.h"

namespace t3widget {

struct expander_group_t::implementation_t {
  expander_t *expanded_widget = nullptr;
  int height = 0, expanded_height;
  signal_t<bool> expanded;

  void widget_expanded(bool is_expanded, expander_t *source) {
    if (is_expanded) {
      if (expanded_widget != nullptr) {
        expanded_widget->set_expanded(false);
        /* This will generate another signal which will reduce the height. */
      }
      expanded_height = source->get_base_window()->get_height() - 1;
      height += expanded_height;
      expanded_widget = source;
    } else {
      if (source == expanded_widget) {
        expanded_widget = nullptr;
        height -= expanded_height;
      }
    }
    expanded(is_expanded);
  }

  void collapse() {
    if (expanded_widget != nullptr) {
      expanded_widget->set_expanded(false);
      expanded_widget = nullptr;
      expanded(false);
    }
  }
};

expander_group_t::expander_group_t() : impl(new implementation_t) {}
expander_group_t::~expander_group_t() {}

void expander_group_t::add_expander(expander_t *expander) {
  if (expander == nullptr) {
    return;
  }
  expander->connect_expanded(
      [this, expander](bool is_expanded) { widget_expanded(is_expanded, expander); });
  expander->set_expanded(false);
  impl->height++;
}

void expander_group_t::widget_expanded(bool is_expanded, expander_t *source) {
  impl->widget_expanded(is_expanded, source);
}

void expander_group_t::collapse() { impl->collapse(); }

int expander_group_t::get_group_height() { return impl->height; }

_T3_WIDGET_IMPL_SIGNAL(expander_group_t, expanded, bool)

}  // namespace t3widget
