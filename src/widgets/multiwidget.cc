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
#include "widgets/multiwidget.h"
#include "log.h"
namespace t3_widget {

multi_widget_t::multi_widget_t() : fixed_sum(0), proportion_sum(0), send_key_widget(nullptr) {
  init_unbacked_window(1, 1, true);
}

multi_widget_t::~multi_widget_t() {
  for (const item_t &widget : widgets) {
    delete widget.widget;
  }
}

bool multi_widget_t::process_key(key_t key) {
  if (send_key_widget != nullptr) {
    return send_key_widget->process_key(key);
  }
  return false;
}

bool multi_widget_t::set_size(optint height, optint width) {
  (void)height;
  if (width.is_valid() && t3_win_get_width(window) != width) {
    t3_win_resize(window, 1, width);
    resize_widgets();
  }
  return true;  // FIXME: use result of widgets
}

void multi_widget_t::update_contents() {
  for (const item_t &widget : widgets) {
    widget.widget->update_contents();
  }
}

void multi_widget_t::set_focus(focus_t focus) {
  for (const item_t &widget : widgets) {
    if (widget.takes_focus) {
      widget.widget->set_focus(focus);
    }
  }
}

bool multi_widget_t::accepts_focus() {
  if (!enabled) {
    return false;
  }
  for (const item_t &widget : widgets) {
    if (widget.takes_focus) {
      return true;
    }
  }
  return false;
}

void multi_widget_t::force_redraw() {
  for (const item_t &widget : widgets) {
    widget.widget->force_redraw();
  }
}

/* Width is negative for fixed width widgets, positive for proportion */
void multi_widget_t::push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys) {
  item_t item;

  if (takes_focus && !widget->accepts_focus()) {
    takes_focus = false;
  }

  if (_width < 0) {
    widget->set_size(None, -_width);
    fixed_sum += -_width;
  } else {
    proportion_sum += _width;
  }

  item.widget = widget;
  item.width = _width;
  item.takes_focus = takes_focus;

  if (send_keys && send_key_widget == nullptr) {
    focus_widget_t *focus_widget;
    send_key_widget = widget;
    if ((focus_widget = dynamic_cast<focus_widget_t *>(widget)) != nullptr) {
      /* We don't have to save the connections, because the widget will not outlive
         this widget. The destructor for multi_widget_t destroys all the widgets
         it contains, and there is no way to remove a widget from a multi_widget_t.
      */
      focus_widget->connect_move_focus_left(move_focus_left.make_slot());
      focus_widget->connect_move_focus_right(move_focus_right.make_slot());
      focus_widget->connect_move_focus_up(move_focus_up.make_slot());
      focus_widget->connect_move_focus_down(move_focus_down.make_slot());
    }
  }

  set_widget_parent(widget);
  if (widgets.size() > 0) {
    widget->set_anchor(widgets.back().widget,
                       T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  }
  widget->set_position(0, 0);
  widgets.push_back(item);
  resize_widgets();
}

void multi_widget_t::resize_widgets() {
  if (proportion_sum > 0) {
    int width = t3_win_get_width(window);
    double scale = static_cast<double>(width - fixed_sum) / proportion_sum;
    int size = 0;

    for (item_t &widget : widgets) {
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
    if (size > width - fixed_sum) {
      for (std::list<item_t>::iterator iter = widgets.begin();
           iter != widgets.end() && size > width - fixed_sum; iter++) {
        if (iter->width < 0) {
          continue;
        }
        if (iter->calculated_width > 1) {
          iter->calculated_width--;
          size--;
        }
      }
    } else {
      while (size < width - fixed_sum) {
        for (std::list<item_t>::iterator iter = widgets.begin();
             iter != widgets.end() && size < width - fixed_sum; iter++) {
          if (iter->width < 0) {
            continue;
          }
          iter->calculated_width++;
          size++;
        }
      }
    }
    for (const item_t &widget : widgets) {
      if (widget.width < 0) {
        continue;
      }
      widget.widget->set_size(1, widget.calculated_width > 0 ? widget.calculated_width : 1);
    }
  }
}

void multi_widget_t::set_enabled(bool enable) {
  enabled = enable;
  for (const item_t &widget : widgets) {
    widget.widget->set_enabled(enable);
  }
}

void multi_widget_t::set_child_focus(window_component_t *target) {
  (void)target;
  set_focus(window_component_t::FOCUS_SET);
}

bool multi_widget_t::is_child(window_component_t *widget) {
  for (const item_t &iter : widgets) {
    if (iter.widget == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter.widget);
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

};  // namespace
