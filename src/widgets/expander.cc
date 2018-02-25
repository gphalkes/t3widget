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
#include "widgets/expander.h"
#include "colorscheme.h"
#include "log.h"

namespace t3_widget {

expander_t::expander_t(const char *text) : impl(new implementation_t(text)) {
  init_unbacked_window(1, impl->label.get_width() + 2);
  impl->symbol_window.alloc(window.get(), 1, 2 + impl->label.get_width(), 0, 0, 0);
  impl->symbol_window.show();
  register_mouse_target(&impl->symbol_window);
}

void expander_t::focus_up_from_child() {
  if (impl->focus != FOCUS_CHILD || impl->child == nullptr) {
    return;
  }
  impl->child->set_focus(window_component_t::FOCUS_OUT);
  impl->focus = FOCUS_SELF;
  redraw = true;
}

void expander_t::set_child(widget_t *_child) {
  focus_widget_t *focus_child;
  /* FIXME: connect to move_focus_XXX events. (requires dynamic_cast'ing to focus_widget_t) */
  if (impl->child != nullptr) {
    unset_widget_parent(impl->child.get());
    impl->move_up_connection.disconnect();
    impl->move_down_connection.disconnect();
    impl->move_right_connection.disconnect();
    impl->move_left_connection.disconnect();
  }

  if (_child == nullptr) {
    if (impl->is_expanded) {
      window.resize(1, window.get_width());
      impl->is_expanded = false;
      redraw = true;
      expanded(false);
    }
    impl->child.reset(_child);
    return;
  }
  impl->child.reset(_child);
  set_widget_parent(impl->child.get());
  impl->child->set_anchor(this, 0);
  impl->child->set_position(1, 0);
  impl->full_height = impl->child->get_base_window()->get_height() + 1;
  focus_child = dynamic_cast<focus_widget_t *>(impl->child.get());
  if (focus_child != nullptr) {
    impl->move_up_connection =
        focus_child->connect_move_focus_up([this] { focus_up_from_child(); });
    impl->move_down_connection =
        focus_child->connect_move_focus_down(move_focus_down.get_trigger());
    impl->move_right_connection =
        focus_child->connect_move_focus_right(move_focus_right.get_trigger());
    impl->move_left_connection =
        focus_child->connect_move_focus_left(move_focus_left.get_trigger());
  }
  set_size(None, impl->child->get_base_window()->get_width());
}

void expander_t::set_expanded(bool expand) {
  if (!expand && impl->is_expanded) {
    if (impl->focus == FOCUS_CHILD) {
      impl->child->set_focus(window_component_t::FOCUS_OUT);
      impl->focus = FOCUS_SELF;
    }
    impl->child->hide();
    impl->is_expanded = false;
    window.resize(1, window.get_width());
    redraw = true;
    expanded(false);
  } else if (expand && !impl->is_expanded && impl->child != nullptr) {
    impl->child->show();
    impl->is_expanded = true;
    window.resize(impl->full_height, window.get_width());
    redraw = true;
    expanded(true);
  }
}

bool expander_t::process_key(key_t key) {
  lprintf("Handling key %08x impl->focus: %d\n", key, impl->focus);
  if (impl->focus == FOCUS_SELF) {
    if (impl->is_expanded && impl->child != nullptr && (key == '\t' || key == EKEY_DOWN)) {
      if (impl->child->accepts_focus()) {
        impl->focus = FOCUS_CHILD;
        impl->child->set_focus(window_component_t::FOCUS_IN_FWD);
      }
      return true;
    } else if (key == EKEY_DOWN && !impl->is_expanded) {
      move_focus_down();
    } else if (key == EKEY_UP) {
      move_focus_up();
    } else if (key == EKEY_RIGHT) {
      move_focus_right();
    } else if (key == EKEY_LEFT) {
      move_focus_left();
    } else if (key == ' ' || key == EKEY_NL || key == EKEY_HOTKEY) {
      if (!impl->is_expanded && impl->child != nullptr) {
        window.resize(impl->full_height, window.get_width());
        impl->is_expanded = true;
        redraw = true;
        impl->child->show();
        if (impl->child->accepts_focus()) {
          impl->focus = FOCUS_CHILD;
          impl->child->set_focus(window_component_t::FOCUS_SET);
        }
        expanded(true);
      } else if (impl->is_expanded) {
        if (impl->child != nullptr) {
          impl->child->hide();
        }
        window.resize(1, window.get_width());
        impl->is_expanded = false;
        redraw = true;
        expanded(false);
      }
      return true;
    }
  } else if (impl->focus == FOCUS_CHILD) {
    bool result = impl->child->process_key(key);
    if (!result && key == (EKEY_SHIFT | '\t')) {
      impl->focus = FOCUS_SELF;
      result = true;
      impl->child->set_focus(window_component_t::FOCUS_OUT);
    }
    return result;
  }
  return false;
}

void expander_t::update_contents() {
  if (impl->is_expanded && impl->child != nullptr) {
    impl->child->update_contents();
  }
  if (!redraw) {
    return;
  }

  impl->symbol_window.set_paint(0, 0);
  impl->symbol_window.set_default_attrs(
      (impl->focus == FOCUS_SELF ? attributes.dialog_selected : attributes.dialog));
  impl->symbol_window.addch(impl->is_expanded ? T3_ACS_DARROW : T3_ACS_RARROW, T3_ATTR_ACS);
  impl->symbol_window.addch(' ', 0);
  impl->label.draw(&impl->symbol_window, 0, impl->focus == FOCUS_SELF);
}

void expander_t::set_focus(focus_t _focus) {
  if (_focus == window_component_t::FOCUS_OUT) {
    if (impl->focus == FOCUS_CHILD && impl->child != nullptr) {
      impl->child->set_focus(window_component_t::FOCUS_OUT);
    }
    impl->last_focus = impl->focus;
    impl->focus = FOCUS_NONE;
  } else if (_focus == window_component_t::FOCUS_SET ||
             _focus == window_component_t::FOCUS_IN_FWD ||
             (_focus == window_component_t::FOCUS_REVERT && impl->last_focus == FOCUS_SELF) ||
             impl->child == nullptr || !impl->is_expanded) {
    impl->focus = FOCUS_SELF;
  } else {
    impl->focus = FOCUS_CHILD;
    impl->child->set_focus(_focus);
  }
  redraw = true;
}

bool expander_t::set_size(optint height, optint width) {
  bool result = true;
  if (height.is_valid() && height > 1) {
    impl->full_height = height;
  }

  if (impl->child != nullptr) {
    result &= impl->child->set_size(impl->full_height - 1, width);
  }

  if (!width.is_valid()) {
    width = window.get_width();
  }
  if (impl->is_expanded) {
    result &= window.resize(impl->full_height, width);
  } else {
    result &= window.resize(1, width);
  }
  return result;
}

bool expander_t::is_hotkey(key_t key) { return impl->label.is_hotkey(key); }

void expander_t::set_enabled(bool enable) {
  enabled = enable;
  if (impl->child != nullptr) {
    impl->child->set_enabled(enable);
  }
}

void expander_t::force_redraw() {
  if (impl->child != nullptr) {
    impl->child->force_redraw();
  }
}

void expander_t::set_child_focus(window_component_t *target) {
  if (target == impl->child.get()) {
    impl->focus = FOCUS_CHILD;
    impl->child->set_focus(window_component_t::FOCUS_SET);
  } else {
    container_t *container = dynamic_cast<container_t *>(impl->child.get());
    if (container != nullptr) {
      impl->focus = FOCUS_CHILD;
      container->set_child_focus(target);
    }
  }
}

bool expander_t::is_child(window_component_t *component) {
  container_t *container;
  if (component == impl->child.get()) {
    return true;
  }
  container = dynamic_cast<container_t *>(impl->child.get());
  return container != nullptr && container->is_child(component);
}

widget_t *expander_t::is_child_hotkey(key_t key) {
  widget_container_t *widget_container;

  if (!impl->is_expanded || impl->child == nullptr || !impl->child->is_shown() ||
      !impl->child->is_enabled()) {
    return nullptr;
  }

  if (impl->child->is_hotkey(key)) {
    return impl->child.get();
  }
  widget_container = dynamic_cast<widget_container_t *>(impl->child.get());
  return widget_container == nullptr ? nullptr : widget_container->is_child_hotkey(key);
}

bool expander_t::process_mouse_event(mouse_event_t event) {
  if (event.button_state & EMOUSE_CLICKED_LEFT) {
    if (!impl->is_expanded && impl->child != nullptr) {
      window.resize(impl->full_height, window.get_width());
      impl->is_expanded = true;
      redraw = true;
      impl->child->show();
      expanded(true);
      if (impl->focus == FOCUS_SELF) {
        impl->focus = FOCUS_CHILD;
        impl->child->set_focus(window_component_t::FOCUS_SET);
      }
    } else if (impl->is_expanded) {
      /* No need to handle impl->focus, because we got a set_focus(SET) event
         before this call anyway. */
      if (impl->child != nullptr) {
        impl->child->hide();
      }
      window.resize(1, window.get_width());
      impl->is_expanded = false;
      redraw = true;
      expanded(false);
    }
  }
  return true;
}

}  // namespace
