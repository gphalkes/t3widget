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
#include "dialogs/attributepickerdialog.h"
#include "colorscheme.h"
#include "widgets/button.h"
#include "widgets/expander.h"
#include "widgets/frame.h"

#define ATTRIBUTE_PICKER_DIALOG_HEIGHT 8
#define ATTRIBUTE_PICKER_DIALOG_WIDTH 43

namespace t3_widget {

struct attribute_picker_dialog_t::implementation_t {
  checkbox_t *bold_box, *reverse_box, *blink_box, *underline_box, *dim_box;
  attribute_test_line_t *test_line;
  color_picker_base_t *fg_picker, *bg_picker;
  std::unique_ptr<expander_group_t> expander_group;
  expander_t *fg_expander, *bg_expander;
  t3_attr_t base_attributes;
  signal_t<t3_attr_t> attribute_selected;
  signal_t<> default_selected;
  implementation_t() : fg_picker(nullptr), bg_picker(nullptr), base_attributes(0) {}
};

attribute_picker_dialog_t::attribute_picker_dialog_t(optional<std::string> _title,
                                                     bool with_default)
    : dialog_t(ATTRIBUTE_PICKER_DIALOG_HEIGHT + 2, ATTRIBUTE_PICKER_DIALOG_WIDTH, std::move(_title),
               impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>()) {
  smart_label_t *underline_label, *bold_label, *dim_label, *reverse_label, *blink_label;
  frame_t *test_line_frame;
  button_t *ok_button, *cancel_button, *default_button = nullptr;
  t3_term_caps_t capabilities;

  t3_term_get_caps(&capabilities);

  impl->underline_box = new checkbox_t(false);
  impl->underline_box->set_position(1, 2);
  underline_label = new smart_label_t("_Underline");
  underline_label->set_anchor(impl->underline_box,
                              T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  underline_label->set_position(0, 1);
  impl->underline_box->set_label(underline_label);
  impl->underline_box->connect_move_focus_up([this] { focus_previous(); });
  impl->underline_box->connect_move_focus_down([this] { focus_next(); });
  impl->underline_box->connect_toggled([this] { attribute_changed(); });
  impl->underline_box->connect_activate([this] { ok_activate(); });

  impl->bold_box = new checkbox_t(false);
  impl->bold_box->set_anchor(impl->underline_box,
                             T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->bold_box->set_position(1, 0);
  bold_label = new smart_label_t("_Bold");
  bold_label->set_anchor(impl->bold_box,
                         T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  bold_label->set_position(0, 1);
  impl->bold_box->set_label(bold_label);
  impl->bold_box->connect_move_focus_up([this] { focus_previous(); });
  impl->bold_box->connect_move_focus_down([this] { focus_next(); });
  impl->bold_box->connect_toggled([this] { attribute_changed(); });
  impl->bold_box->connect_activate([this] { ok_activate(); });

  impl->dim_box = new checkbox_t(false);
  impl->dim_box->set_anchor(impl->bold_box,
                            T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->dim_box->set_position(1, 0);
  dim_label = new smart_label_t("Di_m");
  dim_label->set_anchor(impl->dim_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  dim_label->set_position(0, 1);
  impl->dim_box->set_label(dim_label);
  impl->dim_box->connect_move_focus_up([this] { focus_previous(); });
  impl->dim_box->connect_move_focus_down([this] { focus_next(); });
  impl->dim_box->connect_toggled([this] { attribute_changed(); });
  impl->dim_box->connect_activate([this] { ok_activate(); });

  impl->reverse_box = new checkbox_t(false);
  impl->reverse_box->set_anchor(impl->dim_box,
                                T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->reverse_box->set_position(1, 0);
  reverse_label = new smart_label_t("_Reverse video");
  reverse_label->set_anchor(impl->reverse_box,
                            T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  reverse_label->set_position(0, 1);
  impl->reverse_box->set_label(reverse_label);
  impl->reverse_box->connect_move_focus_up([this] { focus_previous(); });
  impl->reverse_box->connect_move_focus_down([this] { focus_next(); });
  impl->reverse_box->connect_toggled([this] { attribute_changed(); });
  impl->reverse_box->connect_activate([this] { ok_activate(); });

  impl->blink_box = new checkbox_t(false);
  impl->blink_box->set_anchor(impl->reverse_box,
                              T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->blink_box->set_position(1, 0);
  blink_label = new smart_label_t("Bl_ink");
  blink_label->set_anchor(impl->blink_box,
                          T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  blink_label->set_position(0, 1);
  impl->blink_box->set_label(blink_label);
  impl->blink_box->connect_move_focus_up([this] { focus_previous(); });
  impl->blink_box->connect_move_focus_down([this] { focus_next(); });
  impl->blink_box->connect_toggled([this] { attribute_changed(); });
  impl->blink_box->connect_activate([this] { ok_activate(); });

  test_line_frame = new frame_t();
  test_line_frame->set_anchor(this, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  test_line_frame->set_position(1, -2);
  impl->test_line = new attribute_test_line_t();
  test_line_frame->set_size(3, 6);
  test_line_frame->set_child(impl->test_line);

  if (capabilities.cap_flags & (T3_TERM_CAP_FG | T3_TERM_CAP_CP)) {
    impl->expander_group.reset(new expander_group_t());

    if (capabilities.cap_flags & T3_TERM_CAP_FG) {
      impl->fg_picker = new color_picker_t(true);
      impl->fg_expander = new expander_t("_Foreground color");
    } else {
      impl->fg_picker = new color_pair_picker_t();
      impl->fg_expander = new expander_t("Color _pair");
    }
    impl->fg_picker->connect_activated([this] { ok_activate(); });
    impl->fg_picker->connect_selection_changed([this] { attribute_changed(); });
    impl->fg_expander->set_child(impl->fg_picker);
    impl->fg_expander->set_anchor(impl->blink_box,
                                  T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
    impl->fg_expander->set_position(1, 0);
    impl->fg_expander->connect_move_focus_up([this] { focus_previous(); });
    impl->fg_expander->connect_move_focus_down([this] { focus_next(); });
    impl->expander_group->add_expander(impl->fg_expander);

    if (capabilities.cap_flags & T3_TERM_CAP_BG) {
      impl->bg_picker = new color_picker_t(false);
      impl->bg_picker->connect_activated([this] { ok_activate(); });
      impl->bg_picker->connect_selection_changed(([this] { attribute_changed(); }));
      impl->bg_expander = new expander_t("B_ackground color");
      impl->bg_expander->set_child(impl->bg_picker);
      impl->bg_expander->set_anchor(impl->fg_expander,
                                    T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
      impl->bg_expander->set_position(0, 0);
      impl->bg_expander->connect_move_focus_up([this] { focus_previous(); });
      impl->bg_expander->connect_move_focus_down([this] { focus_next(); });
      impl->expander_group->add_expander(impl->bg_expander);
    }

    impl->expander_group->connect_expanded([this](bool expanded) { group_expanded(expanded); });
  }

  cancel_button = new button_t("_Cancel", false);
  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });
  /* Nasty trick: registering a callback three times will call the callback three times. We need to
     do FOCUS_PREVIOUS three times here to emulate moving up, because the ok_button and
     default_button are in the way. */
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_down([this] { focus_next(); });

  if (with_default) {
    default_button = new button_t("_Default");
    default_button->set_anchor(cancel_button,
                               T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
    default_button->set_position(0, -2);
    default_button->connect_activate(impl->default_selected.get_trigger());
    default_button->connect_move_focus_left([this] { focus_previous(); });
    default_button->connect_move_focus_right([this] { focus_next(); });
    default_button->connect_move_focus_up([this] { focus_previous(); });
    default_button->connect_move_focus_up([this] { focus_previous(); });
    default_button->connect_move_focus_down([this] { focus_next(); });
    default_button->connect_move_focus_down([this] { focus_next(); });
  }

  ok_button = new button_t("_OK", true);
  ok_button->set_anchor(with_default ? default_button : cancel_button,
                        T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  ok_button->set_position(0, -2);
  ok_button->connect_activate([this] { ok_activate(); });
  ok_button->connect_move_focus_up([this] { focus_previous(); });
  ok_button->connect_move_focus_right([this] { focus_next(); });
  ok_button->connect_move_focus_down([this] { focus_next(); });
  ok_button->connect_move_focus_down([this] { focus_next(); });
  ok_button->connect_move_focus_down([this] { focus_next(); });

  push_back(impl->underline_box);
  push_back(underline_label);
  push_back(impl->bold_box);
  push_back(bold_label);
  push_back(impl->dim_box);
  push_back(dim_label);
  push_back(impl->reverse_box);
  push_back(reverse_label);
  push_back(impl->blink_box);
  push_back(blink_label);
  if (capabilities.cap_flags & (T3_TERM_CAP_FG | T3_TERM_CAP_CP)) {
    push_back(impl->fg_expander);
    if (capabilities.cap_flags & T3_TERM_CAP_BG) {
      push_back(impl->bg_expander);
    }
  }
  push_back(test_line_frame);
  push_back(ok_button);
  if (with_default) {
    push_back(default_button);
  }
  push_back(cancel_button);
}

attribute_picker_dialog_t::~attribute_picker_dialog_t() {}

void attribute_picker_dialog_t::attribute_changed() {
  impl->test_line->set_attribute(t3_term_combine_attrs(get_attribute(), impl->base_attributes));
}

void attribute_picker_dialog_t::ok_activate() { impl->attribute_selected(get_attribute()); }

void attribute_picker_dialog_t::group_expanded(bool state) {
  (void)state;
  set_size(impl->expander_group->get_group_height() + ATTRIBUTE_PICKER_DIALOG_HEIGHT, None);
}

t3_attr_t attribute_picker_dialog_t::get_attribute() {
  t3_attr_t result = 0;
  if (impl->underline_box->get_state()) {
    result |= T3_ATTR_UNDERLINE;
  }
  if (impl->bold_box->get_state()) {
    result |= T3_ATTR_BOLD;
  }
  if (impl->dim_box->get_state()) {
    result |= T3_ATTR_DIM;
  }
  if (impl->blink_box->get_state()) {
    result |= T3_ATTR_BLINK;
  }
  if (impl->reverse_box->get_state()) {
    result |= T3_ATTR_REVERSE;
  }
  if (impl->fg_picker != nullptr) {
    result |= impl->fg_picker->get_color();
  }
  if (impl->bg_picker != nullptr) {
    result |= impl->bg_picker->get_color();
  }
  return result;
}

void attribute_picker_dialog_t::set_attribute(t3_attr_t attr) {
  impl->underline_box->set_state(attr & T3_ATTR_UNDERLINE);
  impl->bold_box->set_state(attr & T3_ATTR_BOLD);
  impl->dim_box->set_state(attr & T3_ATTR_DIM);
  impl->blink_box->set_state(attr & T3_ATTR_BLINK);
  impl->reverse_box->set_state(attr & T3_ATTR_REVERSE);
  if (impl->fg_picker != nullptr) {
    impl->fg_picker->set_color(attr);
  }
  if (impl->bg_picker != nullptr) {
    impl->bg_picker->set_color(attr);
  }
  attribute_changed();
}

void attribute_picker_dialog_t::set_base_attributes(t3_attr_t attr) {
  impl->base_attributes = attr;
  if (impl->fg_picker != nullptr) {
    impl->fg_picker->set_undefined_colors(attr);
  }
  if (impl->bg_picker != nullptr) {
    impl->bg_picker->set_undefined_colors(attr);
  }
  attribute_changed();
}

void attribute_picker_dialog_t::show() {
  if (impl->expander_group != nullptr) {
    impl->expander_group->collapse();
  }
  dialog_t::show();
}

connection_t attribute_picker_dialog_t::connect_attribute_selected(
    std::function<void(t3_attr_t)> cb) {
  return impl->attribute_selected.connect(cb);
}

connection_t attribute_picker_dialog_t::connect_default_selected(std::function<void()> cb) {
  return impl->default_selected.connect(cb);
}

//================================================================================
struct attribute_test_line_t::implementation_t {
  t3_attr_t attr = 0;
};

attribute_test_line_t::attribute_test_line_t()
    : widget_t(1, 4, false, impl_alloc<implementation_t>(0)), impl(new_impl<implementation_t>()) {}
attribute_test_line_t::~attribute_test_line_t() {}

bool attribute_test_line_t::process_key(key_t key) {
  (void)key;
  return false;
}

bool attribute_test_line_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void attribute_test_line_t::update_contents() {
  if (!reset_redraw()) {
    return;
  }
  window.set_default_attrs(impl->attr);
  window.set_paint(0, 0);
  window.clrtoeol();
  window.set_paint(0, 0);
  window.addstr("Abcd", 0);
}

bool attribute_test_line_t::accepts_focus() { return false; }

void attribute_test_line_t::set_attribute(t3_attr_t attr) {
  force_redraw();
  impl->attr = attr;
}

}  // namespace
