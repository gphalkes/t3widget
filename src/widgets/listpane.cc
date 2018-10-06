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
#include "t3widget/widgets/listpane.h"
#include "t3widget/colorscheme.h"
#include "t3widget/log.h"

namespace t3widget {

class list_pane_t::indicator_widget_t : public widget_t {
 private:
  bool has_focus;

 public:
  indicator_widget_t();
  bool process_key(key_t key) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool set_size(optint height, optint width) override;
  bool accepts_focus() const override;
};

struct list_pane_t::implementation_t {
  size_t top_idx, current;
  t3window::window_t widgets_window;
  widgets_t widgets;
  bool has_focus;
  bool indicator;
  bool single_click_activate;
  scrollbar_t scrollbar;
  std::unique_ptr<indicator_widget_t> indicator_widget;
  signal_t<> activate;
  signal_t<> selection_changed;

  implementation_t(bool _indicator)
      : top_idx(0),
        current(0),
        has_focus(false),
        indicator(_indicator),
        single_click_activate(false),
        scrollbar(true) {}
};

list_pane_t::list_pane_t(bool _indicator)
    : widget_t(impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>(_indicator)) {
  init_unbacked_window(1, 4);

  impl->widgets_window.alloc_unbacked(&window, 1, 3, 0, 0, 0);
  impl->widgets_window.show();
  register_mouse_target(&impl->widgets_window);

  impl->widgets_window.set_parent(&window);
  impl->widgets_window.set_anchor(&window,
                                  T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));

  container_t::set_widget_parent(&impl->scrollbar);
  impl->scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  impl->scrollbar.set_size(1, None);
  impl->scrollbar.connect_clicked(bind_front(&list_pane_t::scrollbar_clicked, this));
  impl->scrollbar.connect_dragged(bind_front(&list_pane_t::scrollbar_dragged, this));

  if (impl->indicator) {
    impl->indicator_widget.reset(new indicator_widget_t());
    container_t::set_widget_parent(impl->indicator_widget.get());
  }
}

list_pane_t::~list_pane_t() {}

bool list_pane_t::set_widget_parent(window_component_t *widget) {
  return widget->get_base_window()->set_parent(&impl->widgets_window);
}

void list_pane_t::ensure_cursor_on_screen() {
  int height = window.get_height();
  if (impl->current >= impl->top_idx + height) {
    impl->top_idx = impl->current - height + 1;
  } else if (impl->current < impl->top_idx) {
    impl->top_idx = impl->current;
  }
}

bool list_pane_t::process_key(key_t key) {
  size_t old_current = impl->current;
  window_component_t::focus_t focus_type;
  int height;

  switch (key) {
    case EKEY_DOWN:
      if (impl->current + 1 >= impl->widgets.size()) {
        return true;
      }
      impl->current++;
      focus_type = window_component_t::FOCUS_IN_FWD;
      break;
    case EKEY_UP:
      if (impl->current == 0) {
        return true;
      }
      impl->current--;
      focus_type = window_component_t::FOCUS_IN_BCK;
      break;
    case EKEY_END:
      impl->current = impl->widgets.size() - 1;
      focus_type = window_component_t::FOCUS_SET;
      break;
    case EKEY_HOME:
      impl->current = 0;
      focus_type = window_component_t::FOCUS_SET;
      break;
    case EKEY_PGDN:
      height = window.get_height();
      if (impl->current + height >= impl->widgets.size()) {
        impl->current = impl->widgets.size() - 1;
      } else {
        impl->current += height;
        if (impl->top_idx + 2 * height < impl->widgets.size()) {
          impl->top_idx += height;
        } else {
          impl->top_idx = impl->widgets.size() - height;
        }
      }
      focus_type = window_component_t::FOCUS_SET;
      break;
    case EKEY_PGUP:
      height = window.get_height();
      if (impl->current < static_cast<size_t>(height)) {
        impl->current = 0;
      } else {
        impl->current -= height;
        impl->top_idx -= height;
      }
      focus_type = window_component_t::FOCUS_SET;
      break;
    case EKEY_NL:
      if (impl->widgets.size() > 0) {
        impl->activate();
      }
      return true;
    default:
      if (impl->widgets.size() > 0) {
        return impl->widgets[impl->current]->process_key(key);
      }
      return false;
  }
  if (impl->current != old_current) {
    impl->widgets[old_current]->set_focus(window_component_t::FOCUS_OUT);
    impl->widgets[impl->current]->set_focus(impl->has_focus ? focus_type
                                                            : window_component_t::FOCUS_OUT);
    impl->selection_changed();
  }
  ensure_cursor_on_screen();
  return true;
}

void list_pane_t::set_position(optint top, optint left) {
  if (!top.is_valid()) {
    top = window.get_y();
  }
  if (!left.is_valid()) {
    left = window.get_x();
  }
  window.move(top.value(), left.value());
}

bool list_pane_t::set_size(optint height, optint width) {
  int widget_width;
  bool result;

  if (!height.is_valid()) {
    height = window.get_height();
  }
  if (!width.is_valid()) {
    width = window.get_width();
  }

  result = window.resize(height.value(), width.value());
  result &= impl->widgets_window.resize(impl->widgets_window.get_height(), width.value() - 1);
  if (impl->indicator) {
    result &= impl->indicator_widget->set_size(None, width.value() - 1);
  }

  widget_width = impl->indicator ? width.value() - 3 : width.value() - 1;

  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    result &= widget->set_size(None, widget_width);
  }

  result &= impl->scrollbar.set_size(height, None);

  ensure_cursor_on_screen();
  return result;
}

void list_pane_t::update_contents() {
  if (impl->indicator) {
    impl->indicator_widget->update_contents();
    impl->indicator_widget->set_position(impl->current - impl->top_idx, 0);
  }

  impl->widgets_window.move(-impl->top_idx, 0);
  impl->scrollbar.set_parameters(impl->widgets.size(), impl->top_idx, window.get_height());
  impl->scrollbar.update_contents();
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    widget->update_contents();
  }
}

void list_pane_t::set_focus(focus_t focus) {
  impl->has_focus = focus;
  if (impl->current < impl->widgets.size()) {
    impl->widgets[impl->current]->set_focus(focus);
  }
  if (impl->indicator) {
    impl->indicator_widget->set_focus(focus);
  }
}

bool list_pane_t::process_mouse_event(mouse_event_t event) {
  if (event.type == EMOUSE_BUTTON_RELEASE && (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) &&
      event.window != impl->widgets_window) {
    if (!impl->single_click_activate) {
      impl->activate();
    }
  } else if (event.type == EMOUSE_BUTTON_RELEASE && (event.button_state & EMOUSE_CLICKED_LEFT) &&
             event.window != impl->widgets_window) {
    impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_OUT);
    impl->current = event.y;
    impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
    impl->selection_changed();
    if (impl->single_click_activate) {
      impl->activate();
    }
  } else if (event.type == EMOUSE_BUTTON_PRESS &&
             (event.button_state & (EMOUSE_SCROLL_UP | EMOUSE_SCROLL_DOWN))) {
    scroll((event.button_state & EMOUSE_SCROLL_UP) ? -3 : 3);
  }
  return true;
}

void list_pane_t::reset() {
  impl->top_idx = 0;
  impl->current = 0;
}

void list_pane_t::update_positions() {
  size_t idx = 0;

  impl->widgets_window.resize(impl->widgets.size(), impl->widgets_window.get_width());
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    widget->set_position(idx, impl->indicator ? 1 : 0);
    ++idx;
  }
}

void list_pane_t::set_anchor(window_component_t *anchor, int relation) {
  window.set_anchor(anchor->get_base_window(), relation);
}

void list_pane_t::force_redraw() {
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    widget->force_redraw();
  }
  if (impl->indicator) {
    impl->indicator_widget->force_redraw();
  }
}

void list_pane_t::set_child_focus(window_component_t *target) {
  size_t old_current = impl->current;

  if (target == &impl->scrollbar || target == impl->indicator_widget.get()) {
    set_focus(window_component_t::FOCUS_SET);
    return;
  }

  size_t idx = 0;
  for (const std::unique_ptr<widget_t> &widget : impl->widgets) {
    if (widget.get() == target) {
      break;
    } else {
      container_t *container = dynamic_cast<container_t *>(widget.get());
      if (container != nullptr && container->is_child(target)) {
        break;
      }
    }
    ++idx;
  }
  if (idx < impl->widgets.size()) {
    impl->current = idx;
    if (impl->has_focus) {
      if (impl->current != old_current) {
        impl->widgets[old_current]->set_focus(window_component_t::FOCUS_OUT);
        impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
        impl->selection_changed();
      }
    } else {
      set_focus(window_component_t::FOCUS_SET);
    }
  }
}

bool list_pane_t::is_child(const window_component_t *widget) const {
  if (widget == &impl->scrollbar || widget == impl->indicator_widget.get()) {
    return true;
  }

  for (const std::unique_ptr<widget_t> &iter : impl->widgets) {
    if (iter.get() == widget) {
      return true;
    } else {
      container_t *container = dynamic_cast<container_t *>(iter.get());
      if (container != nullptr && container->is_child(widget)) {
        return true;
      }
    }
  }
  return false;
}

void list_pane_t::push_back(std::unique_ptr<widget_t> widget) {
  widget->set_size(1, impl->widgets_window.get_width() - (impl->indicator ? 2 : 0));
  widget->set_position(impl->widgets.size(), impl->indicator ? 1 : 0);
  set_widget_parent(widget.get());
  impl->widgets.push_back(std::move(widget));
  impl->widgets_window.resize(impl->widgets.size(), impl->widgets_window.get_width());
}

void list_pane_t::push_front(std::unique_ptr<widget_t> widget) {
  widget->set_size(1, impl->widgets_window.get_width() - (impl->indicator ? 2 : 0));
  set_widget_parent(widget.get());
  impl->widgets.push_front(std::move(widget));
  if (impl->current + 1 < impl->widgets.size()) {
    impl->current++;
  }
  update_positions();
}

std::unique_ptr<widget_t> list_pane_t::pop_back() {
  if (impl->current + 1 == impl->widgets.size()) {
    impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_OUT);
    if (impl->current > 0) {
      impl->current--;
      if (impl->has_focus) {
        impl->widgets[impl->current]->set_focus(window_component_t::FOCUS_SET);
      }
    }
  }
  unset_widget_parent(impl->widgets.back().get());
  std::unique_ptr<widget_t> result(std::move(impl->widgets.back()));
  impl->widgets.pop_back();
  impl->widgets_window.resize(impl->widgets.size(), impl->widgets_window.get_width());
  return result;
}

std::unique_ptr<widget_t> list_pane_t::pop_front() {
  if (impl->current == 0) {
    impl->widgets[0]->set_focus(window_component_t::FOCUS_OUT);
    if (impl->widgets.size() > 1 && impl->has_focus) {
      impl->widgets[1]->set_focus(window_component_t::FOCUS_SET);
    }
  } else {
    impl->current--;
  }
  unset_widget_parent(impl->widgets.front().get());
  std::unique_ptr<widget_t> result = std::move(impl->widgets.front());
  impl->widgets.pop_front();
  update_positions();
  return result;
}

widget_t *list_pane_t::back() { return impl->widgets.back().get(); }

widget_t *list_pane_t::operator[](int idx) { return impl->widgets[idx].get(); }

size_t list_pane_t::size() const { return impl->widgets.size(); }

bool list_pane_t::empty() const { return impl->widgets.empty(); }

list_pane_t::iterator list_pane_t::erase(list_pane_t::iterator position) {
  if (impl->current == position && impl->current + 1 == impl->widgets.size()) {
    if (impl->current != 0) {
      impl->current--;
    }
  }
  unset_widget_parent(impl->widgets[position].get());
  impl->widgets.erase(impl->widgets.begin() + position);
  update_positions();
  return position;
}

list_pane_t::iterator list_pane_t::begin() { return 0; }

list_pane_t::iterator list_pane_t::end() { return impl->widgets.size(); }

size_t list_pane_t::get_current() const { return impl->current; }

void list_pane_t::set_current(size_t idx) {
  if (idx >= impl->widgets.size()) {
    return;
  }

  impl->current = idx;
  ensure_cursor_on_screen();
}

void list_pane_t::scroll(int change) {
  impl->top_idx =
      (change < 0 && impl->top_idx < static_cast<size_t>(-change))
          ? 0
          : (change > 0 && impl->top_idx + window.get_height() + change >= impl->widgets.size())
                ? impl->widgets.size() - window.get_height()
                : impl->top_idx + change;
}

void list_pane_t::scrollbar_clicked(scrollbar_t::step_t step) {
  scroll(step == scrollbar_t::BACK_SMALL
             ? -3
             : step == scrollbar_t::BACK_MEDIUM
                   ? -window.get_height() / 2
                   : step == scrollbar_t::BACK_PAGE
                         ? -window.get_height()
                         : step == scrollbar_t::FWD_SMALL
                               ? 3
                               : step == scrollbar_t::FWD_MEDIUM
                                     ? window.get_height() / 2
                                     : step == scrollbar_t::FWD_PAGE ? window.get_height() : 0);
}

void list_pane_t::scrollbar_dragged(text_pos_t start) {
  if (start >= 0 && static_cast<size_t>(start) <= impl->widgets.size()) {
    impl->top_idx = static_cast<size_t>(start);
    force_redraw();
  }
}

void list_pane_t::set_single_click_activate(bool sca) { impl->single_click_activate = sca; }

connection_t list_pane_t::connect_activate(std::function<void()> cb) {
  return impl->activate.connect(cb);
}

connection_t list_pane_t::connect_selection_changed(std::function<void()> cb) {
  return impl->selection_changed.connect(cb);
}

//=========== Indicator widget ================

list_pane_t::indicator_widget_t::indicator_widget_t() : widget_t(1, 3), has_focus(false) {
  window.set_depth(INT_MAX);
}

bool list_pane_t::indicator_widget_t::process_key(key_t key) {
  (void)key;
  return false;
}

void list_pane_t::indicator_widget_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }
  window.set_default_attrs(attributes.dialog);
  window.set_paint(0, 0);
  window.addch(T3_ACS_RARROW,
               T3_ATTR_ACS | (has_focus ? attributes.dialog_selected : attributes.dialog));
  window.set_paint(0, window.get_width() - 1);
  window.addch(T3_ACS_LARROW,
               T3_ATTR_ACS | (has_focus ? attributes.dialog_selected : attributes.dialog));
}

void list_pane_t::indicator_widget_t::set_focus(focus_t focus) {
  has_focus = focus;
  force_redraw();
}

bool list_pane_t::indicator_widget_t::set_size(optint _height, optint width) {
  (void)_height;

  if (!width.is_valid()) {
    return true;
  }
  force_redraw();
  return window.resize(1, width.value());
}

bool list_pane_t::indicator_widget_t::accepts_focus() const { return false; }

}  // namespace t3widget
