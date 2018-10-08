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
#include "t3widget/widgets/menuitem.h"

#include <string>
#include <type_traits>

#include "t3widget/colorscheme.h"
#include "t3widget/dialogs/menupanel.h"
#include "t3widget/widgets/smartlabel.h"
#include "t3window/terminal.h"
#include "t3window/window.h"

namespace t3widget {

void menu_item_base_t::set_position(optint top, optint left) {
  (void)left;
  if (!top.is_valid()) {
    return;
  }
  window.move(top.value(), 1);
}

bool menu_item_base_t::set_size(optint height, optint width) {
  (void)height;
  if (!width.is_valid()) {
    return true;
  }
  force_redraw();
  return window.resize(1, width.value());
}

void menu_item_base_t::process_mouse_event_from_menu(mouse_event_t event) { (void)event; }

struct menu_item_t::implementation_t {
  smart_label_text_t label;
  std::string shortcut_key;
  int id;
  bool has_focus = false;
  implementation_t(string_view _label, string_view _shortcut_key, int _id,
                   impl_allocator_t *allocator)
      : label(_label, false, allocator), shortcut_key(_shortcut_key), id(_id) {}
};

menu_item_t::menu_item_t(menu_panel_t *_parent, string_view _label, string_view _shortcut_key,
                         int _id)
    : menu_item_base_t(_parent, impl_alloc<implementation_t>(smart_label_text_t::impl_alloc(0))),
      impl(new_impl<implementation_t>(_label, _shortcut_key, _id, this)) {}

menu_item_t::~menu_item_t() {}

bool menu_item_t::process_key(key_t key) {
  switch (key) {
    case EKEY_NL:
    case ' ':
    case EKEY_HOTKEY:
      parent->close();
      parent->activate(impl->id);
      break;
    default:
      return false;
  }
  return true;
}

void menu_item_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }

  window.set_paint(0, 0);
  window.clrtoeol();
  window.set_paint(0, 1);
  window.set_default_attrs(impl->has_focus ? attributes.dialog_selected : attributes.dialog);
  impl->label.draw(&window, 0, impl->has_focus);

  if (!impl->shortcut_key.empty()) {
    window.set_paint(
        0, window.get_width() -
               t3_term_strncwidth(impl->shortcut_key.data(), impl->shortcut_key.size()) - 1);
    window.addnstr(impl->shortcut_key.data(), impl->shortcut_key.size(), 0);
  }
}

void menu_item_t::set_focus(focus_t focus) {
  menu_item_base_t::set_focus(focus);
  if (focus != impl->has_focus) {
    force_redraw();
  }
  impl->has_focus = focus;
}

void menu_item_t::show() {}
void menu_item_t::hide() {}

bool menu_item_t::is_hotkey(key_t key) const { return impl->label.is_hotkey(key); }

void menu_item_t::process_mouse_event_from_menu(mouse_event_t event) {
  if (event.type == EMOUSE_BUTTON_RELEASE &&
      (event.previous_button_state & (EMOUSE_BUTTON_LEFT | EMOUSE_BUTTON_RIGHT))) {
    parent->activate(impl->id);
    parent->close();
  }
  return;
}

text_pos_t menu_item_t::get_label_width() const { return impl->label.get_width() + 2; }

text_pos_t menu_item_t::get_shortcut_key_width() const {
  return impl->shortcut_key.empty()
             ? 0
             : (t3_term_strncwidth(impl->shortcut_key.data(), impl->shortcut_key.size()) + 2);
}

menu_separator_t::menu_separator_t(menu_panel_t *_parent) : menu_item_base_t(_parent) {}

bool menu_separator_t::process_key(key_t key) {
  (void)key;
  return false;
}

void menu_separator_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }
  window.set_paint(0, 0);
  window.addchrep(T3_ACS_HLINE, T3_ATTR_ACS | attributes.dialog, window.get_width());
}

void menu_separator_t::set_focus(focus_t focus) { (void)focus; }

void menu_separator_t::show() {}
void menu_separator_t::hide() {}

bool menu_separator_t::accepts_focus() const { return false; }

}  // namespace t3widget
