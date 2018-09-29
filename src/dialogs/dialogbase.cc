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
#include <algorithm>

#include "t3widget/colorscheme.h"
#include "t3widget/dialogs/dialog.h"
#include "t3widget/dialogs/dialogbase.h"
#include "t3widget/dialogs/mainwindow.h"
#include "t3widget/internal.h"
#include "t3widget/main.h"

namespace t3widget {

dialog_base_list_t dialog_base_t::dialog_base_list;

dummy_widget_t *dialog_base_t::dummy;
connection_t dialog_base_t::init_connected = connect_on_init(dialog_base_t::init);

struct dialog_base_t::implementation_t {
  bool redraw = true;               /**< Boolean indicating whether redrawing is necessary. */
  t3window::window_t shadow_window; /**< t3_window_t used to draw the shadow under a dialog. */
  widgets_t::iterator
      current_widget; /**< Iterator indicating the widget that has the input focus. */
  /** List of widgets on this dialog. This list should only be filled using #push_back. */
  widgets_t widgets;
};

void dialog_base_t::init(bool _init) {
  if (_init) {
    if (dummy == nullptr) {
      dummy = new dummy_widget_t();
    }
  } else {
    if (dummy != nullptr) {
      delete dummy;
    }
  }
}

dialog_base_t::dialog_base_t(int height, int width, bool has_shadow, size_t impl_size)
    : impl_allocator_t(impl_alloc<implementation_t>(impl_size)),
      impl(new_impl<implementation_t>()) {
  window.alloc(nullptr, height, width, 0, 0, 0);
  if (has_shadow) {
    impl->shadow_window.alloc(nullptr, height, width, 1, 1, 1);
    impl->shadow_window.set_anchor(&window, 0);
  }
  dialog_base_list.push_back(this);
  window.set_restrict(nullptr);
  impl->current_widget = impl->widgets.begin();
}

/** Create a new ::dialog_base_t.

    This constructor should only be called by ::main_window_base_t (through ::dialog_t).
*/
dialog_base_t::dialog_base_t(size_t impl_size)
    : impl_allocator_t(impl_alloc<implementation_t>(impl_size)),
      impl(new_impl<implementation_t>()) {
  dialog_base_list.push_back(this);
}

dialog_base_t::~dialog_base_t() {
  for (dialog_base_list_t::iterator iter = dialog_base_list.begin(); iter != dialog_base_list.end();
       iter++) {
    if ((*iter) == this) {
      dialog_base_list.erase(iter);
      break;
    }
  }
  for (widget_t *widget : impl->widgets) {
    if (widget != dummy) {
      delete widget;
    }
  }
}

void dialog_base_t::set_position(optint top, optint left) {
  if (!top.is_valid()) {
    top = window.get_y();
  }
  if (!left.is_valid()) {
    left = window.get_x();
  }

  window.move(top.value(), left.value());
}

bool dialog_base_t::set_size(optint height, optint width) {
  bool result = true;

  impl->redraw = true;
  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  result &= (window.resize(height.value(), width.value()) == 0);
  if (impl->shadow_window != nullptr) {
    result &= (impl->shadow_window.resize(height.value(), width.value()) == 0);
  }
  return result;
}

void dialog_base_t::update_contents() {
  if (get_redraw()) {
    int i, x;

    impl->redraw = false;
    window.set_default_attrs(attributes.dialog);

    /* Just clear the whole thing and redraw */
    window.set_paint(0, 0);
    window.clrtobot();

    window.box(0, 0, window.get_height(), window.get_width(), 0);

    auto &shadow_window = impl->shadow_window;
    if (shadow_window != nullptr) {
      shadow_window.set_default_attrs(attributes.shadow);
      x = shadow_window.get_width() - 1;
      for (i = shadow_window.get_height() - 1; i > 0; i--) {
        shadow_window.set_paint(i - 1, x);
        shadow_window.addch(' ', 0);
      }
      shadow_window.set_paint(shadow_window.get_height() - 1, 0);
      shadow_window.addchrep(' ', 0, shadow_window.get_width());
    }
  }

  for (widget_t *widget : impl->widgets) {
    widget->update_contents();
  }
}

void dialog_base_t::set_focus(focus_t focus) {
  if (impl->current_widget != impl->widgets.end()) {
    (*impl->current_widget)->set_focus(focus);
  }
}

void dialog_base_t::show() {
  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;
  for (current_widget = widgets.begin();
       current_widget != widgets.end() && !(*current_widget)->accepts_focus(); current_widget++) {
  }

  if (current_widget == widgets.end()) {
    widgets.push_front(dummy);
    current_widget = widgets.begin();
  }

  window.show();
  if (impl->shadow_window != nullptr) {
    impl->shadow_window.show();
  }
}

void dialog_base_t::hide() {
  window.hide();
  if (impl->shadow_window != nullptr) {
    impl->shadow_window.hide();
  }
  if (impl->widgets.front() == dummy) {
    impl->widgets.pop_front();
  }
}

void dialog_base_t::focus_next() {
  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;
  (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
  do {
    current_widget++;
    if (current_widget == widgets.end()) {
      current_widget = widgets.begin();
    }
  } while (!(*current_widget)->accepts_focus());

  (*current_widget)->set_focus(window_component_t::FOCUS_IN_FWD);
}

void dialog_base_t::focus_previous() {
  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;
  (*current_widget)->set_focus(window_component_t::FOCUS_OUT);

  do {
    if (current_widget == widgets.begin()) {
      current_widget = widgets.end();
    }

    current_widget--;
  } while (!(*current_widget)->accepts_focus());

  (*current_widget)->set_focus(window_component_t::FOCUS_IN_BCK);
}

void dialog_base_t::set_child_focus(window_component_t *target) {
  widget_t *target_widget = dynamic_cast<widget_t *>(target);
  if (target_widget == nullptr || !target_widget->accepts_focus()) {
    return;
  }

  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;
  for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
    if (*iter == target) {
      if (*current_widget != *iter) {
        (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
        current_widget = iter;
        (*current_widget)->set_focus(window_component_t::FOCUS_SET);
      }
      return;
    } else {
      container_t *container = dynamic_cast<container_t *>(*iter);
      if (container != nullptr && container->is_child(target)) {
        if (*current_widget != *iter) {
          (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
          current_widget = iter;
        }
        container->set_child_focus(target);
        return;
      }
    }
  }
}

void dialog_base_t::set_redraw(bool _redraw) { impl->redraw = _redraw; }
bool dialog_base_t::get_redraw() const { return impl->redraw; }

void dialog_base_t::set_depth(int depth) {
  window.set_depth(depth);
  if (impl->shadow_window != nullptr) {
    impl->shadow_window.set_depth(depth + 1);
  }
}

widget_t *dialog_base_t::get_current_widget() { return *impl->current_widget; }

void dialog_base_t::focus_widget(size_t idx) {
  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;
  (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
  idx = std::min(widgets.size() - 1, idx);
  current_widget = widgets.begin() + idx;
  (*current_widget)->set_focus(window_component_t::FOCUS_SET);
}

bool dialog_base_t::focus_hotkey_widget(key_t key) {
  auto &current_widget = impl->current_widget;
  auto &widgets = impl->widgets;

  for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
    widget_container_t *widget_container;
    widget_t *hotkey_target;

    if (!(*iter)->is_enabled() || !(*iter)->is_shown()) {
      continue;
    }

    if ((*iter)->is_hotkey(key & ~EKEY_META)) {
      if ((*iter)->accepts_focus()) {
        (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
        current_widget = iter;
        (*current_widget)->set_focus(window_component_t::FOCUS_SET);
      }
      if ((*iter)->process_key(EKEY_HOTKEY)) {
        return true;
      }
    } else if ((widget_container = dynamic_cast<widget_container_t *>(*iter)) != nullptr &&
               (hotkey_target = widget_container->is_child_hotkey(key)) != nullptr) {
      if (hotkey_target->accepts_focus()) {
        (*current_widget)->set_focus(window_component_t::FOCUS_OUT);
        current_widget = iter;
        widget_container->set_child_focus(hotkey_target);
      }
      if ((*iter)->process_key(EKEY_HOTKEY)) {
        return true;
      }
    }
  }
  return false;
}

widgets_t &dialog_base_t::widgets() { return impl->widgets; }

t3window::window_t &dialog_base_t::shadow_window() { return impl->shadow_window; }

bool dialog_base_t::is_child(const window_component_t *widget) const {
  for (widget_t *iter : impl->widgets) {
    if (iter == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter);
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

void dialog_base_t::push_back(widget_t *widget) {
  if (!set_widget_parent(widget)) {
    return;
  }
  auto &widgets = impl->widgets;
  if (widgets.size() > 0 && widgets.front() == dummy) {
    widgets.pop_front();
  }
  widgets.push_back(widget);
}

void dialog_base_t::force_redraw() {
  impl->redraw = true;
  for (widget_t *widget : impl->widgets) {
    widget->force_redraw();
  }
}

void dialog_base_t::center_over(const window_component_t *center) {
  window.set_anchor(center->get_base_window(),
                    T3_PARENT(T3_ANCHOR_CENTER) | T3_CHILD(T3_ANCHOR_CENTER));
  window.move(0, 0);
}

void dialog_base_t::force_redraw_all() {
  for (dialog_base_t *dialog : dialog_base_list) {
    dialog->force_redraw();
  }
}

}  // namespace t3widget
