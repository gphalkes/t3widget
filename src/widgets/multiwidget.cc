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
#include "widgets/multiwidget.h"
#include "log.h"
namespace t3_widget {

struct T3_WIDGET_LOCAL item_t {
  std::unique_ptr<widget_t> widget;
  int width;
  int calculated_width;
  bool takes_focus;
};

struct multi_widget_t::implementation_t {
  std::list<item_t> widgets;
  int fixed_sum = 0, proportion_sum = 0;
  widget_t *send_key_widget = nullptr;
};

multi_widget_t::multi_widget_t()
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()) {
  init_unbacked_window(1, 1, true);
}

multi_widget_t::~multi_widget_t() {}

bool multi_widget_t::process_key(key_t key) {
  if (impl->send_key_widget != nullptr) {
    return impl->send_key_widget->process_key(key);
  }
  return false;
}

bool multi_widget_t::set_size(optint height, optint width) {
  (void)height;
  if (width.is_valid() && window.get_width() != width.value()) {
    window.resize(1, width.value());
    resize_widgets();
  }
  return true;  // FIXME: use result of widgets
}

void multi_widget_t::update_contents() {
  for (const item_t &widget : impl->widgets) {
    widget.widget->update_contents();
  }
}

void multi_widget_t::set_focus(focus_t focus) {
  for (const item_t &item : impl->widgets) {
    if (item.takes_focus && item.widget->accepts_focus()) {
      item.widget->set_focus(focus);
    }
  }
}

bool multi_widget_t::accepts_focus() {
  if (!is_enabled()) {
    return false;
  }
  for (const item_t &item : impl->widgets) {
    if (item.takes_focus && item.widget->accepts_focus()) {
      return true;
    }
  }
  return false;
}

void multi_widget_t::force_redraw() {
  for (const item_t &widget : impl->widgets) {
    widget.widget->force_redraw();
  }
}

/* Width is negative for fixed width widgets, positive for proportion */
void multi_widget_t::push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys) {
  if (_width < 0) {
    widget->set_size(None, -_width);
    impl->fixed_sum += -_width;
  } else {
    impl->proportion_sum += _width;
  }

  if (send_keys && impl->send_key_widget == nullptr) {
    focus_widget_t *focus_widget;
    impl->send_key_widget = widget;
    if ((focus_widget = dynamic_cast<focus_widget_t *>(widget)) != nullptr) {
      /* We don't have to save the connections, because the widget will not outlive
         this widget. The destructor for multi_widget_t destroys all the widgets
         it contains, and there is no way to remove a widget from a multi_widget_t.
      */
      focus_widget->connect_move_focus_left(move_focus_left.get_trigger());
      focus_widget->connect_move_focus_right(move_focus_right.get_trigger());
      focus_widget->connect_move_focus_up(move_focus_up.get_trigger());
      focus_widget->connect_move_focus_down(move_focus_down.get_trigger());
    }
  }

  set_widget_parent(widget);
  if (impl->widgets.size() > 0) {
    widget->set_anchor(impl->widgets.back().widget.get(),
                       T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  }
  widget->set_position(0, 0);
  item_t item{std::unique_ptr<widget_t>(widget), _width, 0, takes_focus};
  impl->widgets.push_back(std::move(item));
  resize_widgets();
}

void multi_widget_t::resize_widgets() {
  if (impl->proportion_sum > 0) {
    int width = window.get_width();
    double scale = static_cast<double>(width - impl->fixed_sum) / impl->proportion_sum;
    int size = 0;

    for (item_t &widget : impl->widgets) {
      if (widget.width < 0) {
        continue;
      }
      widget.calculated_width = static_cast<int>(scale * widget.width);
      if (widget.calculated_width == 0) {
        widget.calculated_width = 1;
      }
      size += widget.calculated_width;
    }
    // FIXME: this will do for now, but should be slightly smarter
    if (size > width - impl->fixed_sum) {
      for (item_t &item : impl->widgets) {
        if (size <= width - impl->fixed_sum) {
          break;
        }

        if (item.width < 0) {
          continue;
        }
        if (item.calculated_width > 1) {
          item.calculated_width--;
          size--;
        }
      }
    } else {
      while (size < width - impl->fixed_sum) {
        for (item_t &item : impl->widgets) {
          if (size >= width - impl->fixed_sum) {
            break;
          }
          if (item.width < 0) {
            continue;
          }
          item.calculated_width++;
          size++;
        }
      }
    }
    for (const item_t &widget : impl->widgets) {
      if (widget.width < 0) {
        continue;
      }
      widget.widget->set_size(1, widget.calculated_width > 0 ? widget.calculated_width : 1);
    }
  }
}

void multi_widget_t::set_enabled(bool enable) {
  widget_t::set_enabled(enable);
  for (const item_t &widget : impl->widgets) {
    widget.widget->set_enabled(enable);
  }
}

void multi_widget_t::set_child_focus(window_component_t *target) {
  (void)target;
  set_focus(window_component_t::FOCUS_SET);
}

bool multi_widget_t::is_child(window_component_t *widget) {
  for (const item_t &item : impl->widgets) {
    if (item.widget.get() == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(item.widget.get());
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace
