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
#include <cmath>
#include <cstring>

#include "colorscheme.h"
#include "log.h"
#include "widgets/scrollbar.h"

namespace t3widget {

struct scrollbar_t::implementation_t {
  int length;
  int range, start, used;
  int before, slider_size;
  int button_down_pos;
  bool vertical;
  bool dragging;
  signal_t<step_t> clicked;
  signal_t<int> dragged;

  implementation_t(bool _vertical)
      : length(3),
        range(1),
        start(0),
        used(1),
        before(0),
        slider_size(length - 2),
        button_down_pos(0),
        vertical(_vertical),
        dragging(false) {}
};

scrollbar_t::scrollbar_t(bool _vertical)
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>(_vertical)) {
  int width, height;

  if (impl->vertical) {
    width = 1;
    height = impl->length;
  } else {
    width = impl->length;
    height = 1;
  }

  init_window(height, width);
}

scrollbar_t::~scrollbar_t() {}

bool scrollbar_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool scrollbar_t::set_size(optint height, optint width) {
  bool result = true;
  if (impl->vertical) {
    if (height.is_valid()) {
      impl->length = height.value();
      result = window.resize(impl->length, 1);
    }
  } else {
    if (width.is_valid()) {
      impl->length = width.value();
      result = window.resize(1, impl->length);
    }
  }
  force_redraw();
  return result;
}

void scrollbar_t::update_contents() {
  int i;
  double blocks_per_line;

  if (!reset_redraw()) {
    return;
  }

  blocks_per_line = static_cast<double>(impl->length - 2) / impl->range;
  impl->slider_size = static_cast<int>(blocks_per_line * impl->used);
  if (impl->slider_size == 0) {
    impl->slider_size = 1;
    /* Rounding errors may cause incorrect slider sizing. This is normally not a
       problem, except for the case where the slider should be maximal. */
  } else if (impl->range <= impl->used) {
    impl->slider_size = impl->length - 2;
  }

  /* Recalulate the number of blocks per line, because the slider may actually
     be larger than it should be. */
  if (impl->range <= impl->used) {
    blocks_per_line = strtod("Inf", nullptr);
  } else {
    blocks_per_line =
        static_cast<double>(impl->length - 2 - impl->slider_size) / (impl->range - impl->used + 1);
  }

  impl->before = static_cast<int>(ceil(blocks_per_line * impl->start));
  if (impl->before >= impl->length - 2) {
    impl->before = impl->length - 3;
  }
  if (impl->before > 1 && impl->before + impl->slider_size == impl->length - 2 &&
      impl->used + impl->start < impl->range) {
    impl->before--;
  } else if (impl->used + impl->start == impl->range) {
    impl->before = impl->length - 2 - impl->slider_size;
  }

  window.set_paint(0, 0);
  window.addch(impl->vertical ? T3_ACS_UARROW : T3_ACS_LARROW, T3_ATTR_ACS | attributes.scrollbar);

  for (i = 1; i < impl->length - 1 && i < impl->before + 1; i++) {
    if (impl->vertical) {
      window.set_paint(i, 0);
    }
    window.addch(T3_ACS_CKBOARD, T3_ATTR_ACS | attributes.scrollbar);
  }
  for (; i < impl->length - 1 && i < impl->before + impl->slider_size + 1; i++) {
    if (impl->vertical) {
      window.set_paint(i, 0);
    }
    window.addch(' ', attributes.scrollbar);
  }
  for (; i < impl->length - 1; i++) {
    if (impl->vertical) {
      window.set_paint(i, 0);
    }
    window.addch(T3_ACS_CKBOARD, T3_ATTR_ACS | attributes.scrollbar);
  }

  if (impl->vertical) {
    window.set_paint(impl->length - 1, 0);
  }
  window.addch(impl->vertical ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS | attributes.scrollbar);
}

bool scrollbar_t::accepts_focus() { return false; }
void scrollbar_t::set_focus(focus_t focus) { (void)focus; }

bool scrollbar_t::process_mouse_event(mouse_event_t event) {
  if (event.type == EMOUSE_MOTION && impl->dragging) {
    int location = (impl->vertical ? event.y : event.x) - 1 - impl->button_down_pos;

    if (impl->used >= impl->range) {
      /* Do nothing */
    }
    if (location == 0) {
      impl->dragged(0);
    } else if (location >= impl->length - 2 - impl->slider_size) {
      impl->dragged(impl->range - impl->used);
    } else {
      double lines_per_block = static_cast<double>(impl->range - impl->used + 1) /
                               (impl->length - 2 - impl->slider_size);
      impl->dragged(floor(location * lines_per_block));
    }
  } else if (event.type == EMOUSE_BUTTON_PRESS && event.button_state & EMOUSE_BUTTON_LEFT) {
    int location = (impl->vertical ? event.y : event.x) - 1;
    if (location >= impl->before && location < impl->before + impl->slider_size) {
      impl->dragging = true;
      impl->button_down_pos = location - impl->before;
    }
  } else if (event.type == EMOUSE_BUTTON_RELEASE && (event.button_state & EMOUSE_CLICKED_LEFT) &&
             !impl->dragging) {
    int pos;
    pos = impl->vertical ? event.y : event.x;

    if (pos == 0) {
      impl->clicked(BACK_SMALL);
    } else if (pos == impl->length - 1) {
      impl->clicked(FWD_SMALL);
    } else if (pos <= impl->before) {
      impl->clicked(BACK_PAGE);
    } else if (pos > impl->before + impl->slider_size) {
      impl->clicked(FWD_PAGE);
    }
  } else if (event.type == EMOUSE_BUTTON_RELEASE && impl->dragging) {
    impl->dragging = false;
  } else if (event.type == EMOUSE_BUTTON_PRESS &&
             (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
    impl->clicked((event.button_state & EMOUSE_SCROLL_UP) ? BACK_MEDIUM : FWD_MEDIUM);
  }
  /* We don't take focus, so return false. */
  return false;
}

void scrollbar_t::set_parameters(int _range, int _start, int _used) {
  if (impl->range == _range && impl->start == _start && impl->used == _used) {
    return;
  }

  force_redraw();
  impl->range = _range;
  impl->start = _start;
  impl->used = _used;
}

connection_t scrollbar_t::connect_clicked(std::function<void(step_t)> cb) {
  return impl->clicked.connect(cb);
}

connection_t scrollbar_t::connect_dragged(std::function<void(int)> cb) {
  return impl->dragged.connect(cb);
}

}  // namespace t3widget
