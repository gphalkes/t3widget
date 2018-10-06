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
#include <cstring>
#include <limits>

#include "t3widget/dialogs/menupanel.h"
#include "t3widget/log.h"
#include "t3widget/main.h"
#include "t3widget/util.h"
#include "t3widget/widgets/menuitem.h"

namespace t3widget {

struct menu_panel_t::implementation_t {
  int width, label_width, shortcut_key_width;
  smart_label_text_t label;
  menu_bar_t *menu_bar;
  signal_t<int> activate;

  implementation_t(string_view name, impl_allocator_t *allocator)
      : label(name, false, allocator), menu_bar(nullptr) {}
};

menu_panel_t::menu_panel_t(string_view name)
    : dialog_t(3, 5, nullopt, impl_alloc<implementation_t>(smart_label_text_t::impl_alloc(0))),
      impl(new_impl<implementation_t>(name, this)) {
  impl->width = 5;
  impl->label_width = 1;
  impl->shortcut_key_width = 0;
}

menu_panel_t::~menu_panel_t() {}

bool menu_panel_t::process_key(key_t key) {
  switch (key) {
    case EKEY_LEFT:
      if (impl->menu_bar != nullptr) {
        impl->menu_bar->previous_menu();
      }
      break;
    case EKEY_RIGHT:
      if (impl->menu_bar != nullptr) {
        impl->menu_bar->next_menu();
      }
      break;
    case EKEY_UP:
      focus_previous();
      break;
    case EKEY_DOWN:
      focus_next();
      break;
    case EKEY_HOME:
      focus_widget(0);
      break;
    case EKEY_END:
      focus_widget(std::numeric_limits<size_t>::max());
      break;
    case '\t':
    case EKEY_SHIFT | '\t':
      break;
    case EKEY_ESC:
      close();
      break;
    case EKEY_NL:
    case ' ':
      get_current_widget()->process_key(key);
      break;
    default:
      return focus_hotkey_widget(key);
  }
  return true;
}

void menu_panel_t::set_position(optint top, optint left) {
  dialog_t::set_position(impl->menu_bar == nullptr ? top.value() : 1, left);
}

bool menu_panel_t::set_size(optint height, optint _width) {
  (void)_width;
  bool result;
  int i = 0;
  for (std::unique_ptr<widget_t> &widget : widgets()) {
    widget->set_size(None, impl->width - 2);
    ++i;
  }

  result = dialog_t::set_size(height, impl->width);
  return result;
}

bool menu_panel_t::process_mouse_event(mouse_event_t event) {
  if ((event.x < 0 || event.y < 0 || event.x > window.get_width() ||
       event.y > window.get_height()) &&
      event.type & EMOUSE_OUTSIDE_GRAB &&
      (event.type & ~EMOUSE_OUTSIDE_GRAB) == EMOUSE_BUTTON_RELEASE) {
    close();
    return true;
  }
  if (event.x < 1 || event.x > window.get_width() - 2 || event.y < 1 ||
      event.y > window.get_height() - 2) {
    return true;
  }
  focus_widget(event.y - 1);
  event.type &= ~EMOUSE_OUTSIDE_GRAB;
  event.y = 0;
  event.x--;
  static_cast<menu_item_base_t *>(get_current_widget())->process_mouse_event_from_menu(event);
  return true;
}

void menu_panel_t::show() {
  dialog_t::show();
  if (impl->menu_bar == nullptr) {
    grab_mouse();
  }
}

void menu_panel_t::hide() {
  dialog_t::hide();
  if (impl->menu_bar == nullptr) {
    release_mouse_grab();
  }
}

void menu_panel_t::close() {
  if (impl->menu_bar != nullptr) {
    impl->menu_bar->close();
  } else {
    dialog_t::close();
  }
}

menu_item_base_t *menu_panel_t::insert_item(const menu_item_base_t *before, string_view _label,
                                            string_view shortcut_key, int id) {
  std::unique_ptr<menu_item_t> item =
      t3widget::make_unique<menu_item_t>(this, _label, shortcut_key, id);
  return insert_item(before, std::move(item));
}

menu_item_base_t *menu_panel_t::insert_item(const menu_item_base_t *before,
                                            std::unique_ptr<menu_item_t> item) {
  menu_item_t *item_ptr = item.get();
  if (before) {
    insert(before, std::move(item));
  } else {
    push_back(std::move(item));
  }
  item_ptr->set_position(widgets().size(), None);

  impl->shortcut_key_width = std::max(impl->shortcut_key_width, item_ptr->get_shortcut_key_width());
  impl->label_width = std::max(impl->label_width, item_ptr->get_label_width());
  if (impl->shortcut_key_width + impl->label_width > impl->width - 2) {
    impl->width = impl->shortcut_key_width + impl->label_width + 2;
  }
  set_size(widgets().size() + 2, impl->width);
  return item_ptr;
}

menu_item_base_t *menu_panel_t::insert_separator(const menu_item_base_t *before) {
  menu_separator_t *sep = new menu_separator_t(this);
  if (before) {
    insert(before, wrap_unique(sep));
  } else {
    push_back(wrap_unique(sep));
  }
  sep->set_position(widgets().size(), None);
  return sep;
}

std::unique_ptr<menu_item_base_t> menu_panel_t::remove_item(menu_item_base_t *item) {
  for (auto iter = widgets().begin(); iter != widgets().end(); iter++) {
    if (iter->get() == item) {
      unset_widget_parent(item);
      iter->release();
      widgets().erase(iter);
      recompute_panel_dimensions();
      return wrap_unique(item);
    }
  }
  return nullptr;
}

void menu_panel_t::recompute_panel_dimensions() {
  impl->width = 5;
  impl->label_width = 1;
  impl->shortcut_key_width = 0;
  int i = 1;
  for (auto iter = widgets().begin(); iter != widgets().end(); iter++, i++) {
    (*iter)->set_position(i, None);
    menu_item_t *label_item = dynamic_cast<menu_item_t *>(iter->get());
    if (label_item != nullptr) {
      impl->shortcut_key_width =
          std::max(impl->shortcut_key_width, label_item->get_shortcut_key_width());
      impl->label_width = std::max(impl->label_width, label_item->get_label_width());
    }
    if (impl->shortcut_key_width + impl->label_width > impl->width - 2) {
      impl->width = impl->shortcut_key_width + impl->label_width + 2;
    }
  }
  set_size(widgets().size() + 2, impl->width);
}

void menu_panel_t::set_menu_bar(menu_bar_t *_menu_bar) {
  if (impl->menu_bar == _menu_bar) {
    return;
  }

  if (_menu_bar == nullptr) {
    impl->menu_bar = nullptr;
    window.set_anchor(nullptr, 0);
  } else {
    if (impl->menu_bar != nullptr) {
      impl->menu_bar->remove_menu(this);
    }
    impl->menu_bar = _menu_bar;
    window.set_anchor(impl->menu_bar->get_base_window(), 0);
  }
}

const menu_bar_t *menu_panel_t::get_menu_bar() const { return impl->menu_bar; }

void menu_panel_t::draw_label(t3window::window_t *draw_window, t3_attr_t attr,
                              bool selected) const {
  impl->label.draw(draw_window, attr, selected);
}

int menu_panel_t::get_label_width() const { return impl->label.get_width(); }

bool menu_panel_t::is_hotkey(key_t key) const { return impl->label.is_hotkey(key); }

bool menu_panel_t::is_child(const window_component_t *widget) const {
  return dialog_t::is_child(widget);
}

void menu_panel_t::activate(int idx) { impl->activate(idx); }

connection_t menu_panel_t::connect_activate(std::function<void(int)> cb) {
  return impl->activate.connect(cb);
}

}  // namespace t3widget
