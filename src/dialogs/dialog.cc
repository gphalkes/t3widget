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
#include "dialogs/dialog.h"
#include "colorscheme.h"
#include "dialogs/mainwindow.h"
#include "internal.h"
#include "main.h"

namespace t3_widget {

dialogs_t dialog_t::active_dialogs;
popup_t *dialog_t::active_popup;
int dialog_t::dialog_depth;

struct dialog_t::implementation_t {
  bool active =
      false; /**< Boolean indicating whether this dialog is currently being shown on screen. */
  signal_t<> closed;           /**< Signal emitted when the dialog is closed by calling #close. */
  optional<std::string> title; /**< The title of this dialog. */

  implementation_t(optional<std::string> _title) : title(std::move(_title)) {}
};

dialog_t::dialog_t(int height, int width, optional<std::string> title, size_t impl_size)
    : dialog_base_t(height, width, true, impl_alloc<implementation_t>(impl_size)),
      impl(new_impl<implementation_t>(std::move(title))) {}

/** Create a new ::dialog_t.

    This constructor should only be called by ::main_window_base_t.
*/
dialog_t::dialog_t() : impl(new implementation_t(nullopt)) {}

void dialog_t::activate_dialog() {
  if (!active_dialogs.empty()) {
    if (this == active_dialogs.back()) {
      return;
    }

    active_dialogs.back()->set_focus(window_component_t::FOCUS_OUT);
    if (impl->active) {
      remove_element(active_dialogs, this);
    }
  }

  impl->active = true;
  set_focus(window_component_t::FOCUS_SET);
  dialog_depth -= 2;
  window.set_depth(dialog_depth);
  if (shadow_window() != nullptr) {
    shadow_window().set_depth(dialog_depth + 1);
  }
  active_dialogs.push_back(this);
  if (active_popup != nullptr) {
    active_popup->hide();
  }
}

void dialog_t::deactivate_dialog() {
  impl->active = false;

  if (active_popup != nullptr) {
    active_popup->hide();
  }

  if (this == active_dialogs.back()) {
    this->set_focus(window_component_t::FOCUS_OUT);
    active_dialogs.pop_back();
    active_dialogs.back()->set_focus(window_component_t::FOCUS_REVERT);
    dialog_depth = active_dialogs.back()->window.get_depth();
    return;
  }

  remove_element(active_dialogs, this);
}

dialog_t::~dialog_t() {}

bool dialog_t::process_key(key_t key) {
  if (active_popup != nullptr) {
    if (active_popup->process_key(key)) {
      return true;
    }
  }

  if ((key & EKEY_META) || key == EKEY_F10) {
    if (focus_hotkey_widget(key)) {
      return true;
    }
  }

  if (get_current_widget()->process_key(key)) {
    return true;
  }

  switch (key) {
    case EKEY_ESC:
      close();
      break;
    case '\t':
      focus_next();
      break;
    case EKEY_SHIFT | '\t':
      focus_previous();
      break;
    default:
      return false;
  }
  return true;
}

void dialog_t::update_contents() {
  bool redraw_title = get_redraw();

  dialog_base_t::update_contents();

  if (redraw_title && impl->title.is_valid()) {
    window.set_paint(0, 3);
    window.addstr(" ", 0);
    window.addstr(impl->title.value().c_str(), 0);
    window.addstr(" ", 0);
  }
}

void dialog_t::show() {
  dialog_base_t::show();
  activate_dialog();
}

void dialog_t::hide() {
  deactivate_dialog();
  dialog_base_t::hide();
}

void dialog_t::close() {
  hide();
  impl->closed();
}

bool dialog_t::is_child(window_component_t *widget) {
  if (active_popup != nullptr && active_popup->is_child(widget)) {
    return true;
  }
  return dialog_base_t::is_child(widget);
}

void dialog_t::set_child_focus(window_component_t *target) {
  if (active_popup != nullptr && active_popup->is_child(target)) {
    active_popup->set_child_focus(target);
    return;
  }
  dialog_base_t::set_child_focus(target);
}

void dialog_t::set_title(std::string title) { impl->title = std::move(title); }

void dialog_t::set_active_popup(popup_t *popup) {
  if (popup == active_popup) {
    return;
  }
  popup_t *prev_active = active_popup;
  active_popup = popup;
  if (prev_active != nullptr) {
    prev_active->dialog_base_t::hide();
  }
}

void dialog_t::update_dialogs() {
  for (dialog_t *active_dialog : dialog_t::active_dialogs) {
    active_dialog->update_contents();
  }
  if (active_popup) {
    active_popup->update_contents();
  }
}

_T3_WIDGET_IMPL_SIGNAL(dialog_t, closed)

}  // namespace
