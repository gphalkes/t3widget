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
#include "t3widget/dialogs/finddialog.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "t3widget/dialogs/dialog.h"
#include "t3widget/dialogs/messagedialog.h"
#include "t3widget/findcontext.h"
#include "t3widget/internal.h"
#include "t3widget/main.h"
#include "t3widget/string_view.h"
#include "t3widget/util.h"
#include "t3widget/widgets/button.h"
#include "t3widget/widgets/checkbox.h"
#include "t3widget/widgets/smartlabel.h"
#include "t3widget/widgets/textfield.h"
#include "t3window/window.h"

namespace t3widget {

#define FIND_DIALOG_WIDTH 50
#define FIND_DIALOG_HEIGHT 10

struct find_dialog_t::implementation_t {
  smart_label_t *replace_label;
  text_field_t *find_line, *replace_line;
  checkbox_t *whole_word_checkbox, *match_case_checkbox, *regex_checkbox, *wrap_checkbox,
      *transform_backslash_checkbox, *reverse_direction_checkbox;
  button_t *in_selection_button, *replace_all_button;
  connection_t find_button_up_connection;
  int state;  // State of all the checkboxes converted to FIND_* flags
  signal_t<std::shared_ptr<finder_t>, find_action_t> activate;
};

// FIXME: keep (limited) history

find_dialog_t::find_dialog_t(int _state)
    : dialog_t(FIND_DIALOG_HEIGHT, FIND_DIALOG_WIDTH, _("Find")), impl(new implementation_t()) {
  smart_label_t *find_label = emplace_back<smart_label_t>("Fi_nd", true);
  find_label->set_position(1, 2);
  impl->find_line = emplace_back<text_field_t>();
  impl->find_line->set_anchor(find_label,
                              T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->find_line->set_size(None, FIND_DIALOG_WIDTH - find_label->get_width() - 5);
  impl->find_line->set_position(0, 1);
  impl->find_line->set_label(find_label);
  impl->find_line->connect_activate([this] { find_activated(); });

  impl->replace_label = emplace_back<smart_label_t>("Re_place with", true);
  impl->replace_label->set_position(2, 2);
  impl->replace_line = emplace_back<text_field_t>();
  impl->replace_line->set_anchor(impl->replace_label,
                                 T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->replace_line->set_size(None, FIND_DIALOG_WIDTH - impl->replace_label->get_width() - 5);
  impl->replace_line->set_position(0, 1);
  impl->replace_line->set_label(impl->replace_label);
  impl->replace_line->connect_activate([this] { find_activated(); });
  impl->replace_label->hide();
  impl->replace_line->hide();

  impl->whole_word_checkbox = emplace_back<checkbox_t>();
  impl->whole_word_checkbox->set_position(2, 2);
  smart_label_t *whole_word_label = emplace_back<smart_label_t>("Match _whole word only");
  whole_word_label->set_anchor(impl->whole_word_checkbox,
                               T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  whole_word_label->set_position(0, 1);
  impl->whole_word_checkbox->set_label(whole_word_label);
  impl->whole_word_checkbox->connect_toggled([this] { whole_word_toggled(); });
  impl->whole_word_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->whole_word_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->whole_word_checkbox->connect_activate([this] { find_activated(); });
  impl->match_case_checkbox = emplace_back<checkbox_t>();
  impl->match_case_checkbox->set_anchor(
      impl->whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->match_case_checkbox->set_position(0, 0);
  smart_label_t *match_case_label = emplace_back<smart_label_t>("Case sensiti_ve");
  match_case_label->set_anchor(impl->match_case_checkbox,
                               T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  match_case_label->set_position(0, 1);
  impl->match_case_checkbox->set_label(match_case_label);
  impl->match_case_checkbox->connect_toggled([this] { icase_toggled(); });
  impl->match_case_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->match_case_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->match_case_checkbox->connect_activate([this] { find_activated(); });
  impl->regex_checkbox = emplace_back<checkbox_t>();
  impl->regex_checkbox->set_anchor(impl->whole_word_checkbox,
                                   T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->regex_checkbox->set_position(1, 0);
  smart_label_t *regex_label = emplace_back<smart_label_t>("Regular _expression");
  regex_label->set_anchor(impl->regex_checkbox,
                          T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  regex_label->set_position(0, 1);
  impl->regex_checkbox->set_label(regex_label);
  impl->regex_checkbox->connect_toggled([this] { regex_toggled(); });
  impl->regex_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->regex_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->regex_checkbox->connect_activate([this] { find_activated(); });
  impl->wrap_checkbox = emplace_back<checkbox_t>();
  impl->wrap_checkbox->set_anchor(impl->whole_word_checkbox,
                                  T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->wrap_checkbox->set_position(2, 0);
  smart_label_t *wrap_label = emplace_back<smart_label_t>("Wrap ar_ound");
  wrap_label->set_anchor(impl->wrap_checkbox,
                         T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  wrap_label->set_position(0, 1);
  impl->wrap_checkbox->set_label(wrap_label);
  impl->wrap_checkbox->connect_toggled([this] { wrap_toggled(); });
  impl->wrap_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->wrap_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->wrap_checkbox->connect_activate([this] { find_activated(); });
  impl->transform_backslash_checkbox = emplace_back<checkbox_t>();
  impl->transform_backslash_checkbox->set_anchor(
      impl->whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->transform_backslash_checkbox->set_position(3, 0);
  smart_label_t *transform_backslash_label =
      emplace_back<smart_label_t>("Transform _backslash expressions");
  transform_backslash_label->set_anchor(
      impl->transform_backslash_checkbox,
      T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  transform_backslash_label->set_position(0, 1);
  impl->transform_backslash_checkbox->set_label(transform_backslash_label);
  impl->transform_backslash_checkbox->connect_toggled([this] { transform_backslash_toggled(); });
  impl->transform_backslash_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->transform_backslash_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->transform_backslash_checkbox->connect_activate([this] { find_activated(); });
  impl->reverse_direction_checkbox = emplace_back<checkbox_t>();
  impl->reverse_direction_checkbox->set_anchor(
      impl->whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->reverse_direction_checkbox->set_position(4, 0);
  smart_label_t *reverse_direction_label = emplace_back<smart_label_t>("_Up");
  reverse_direction_label->set_anchor(impl->reverse_direction_checkbox,
                                      T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  reverse_direction_label->set_position(0, 1);
  impl->reverse_direction_checkbox->set_label(reverse_direction_label);
  impl->reverse_direction_checkbox->connect_toggled([this] { backward_toggled(); });
  impl->reverse_direction_checkbox->connect_move_focus_up([this] { focus_previous(); });
  impl->reverse_direction_checkbox->connect_move_focus_down([this] { focus_next(); });
  impl->reverse_direction_checkbox->connect_activate([this] { find_activated(); });

  impl->in_selection_button = emplace_back<button_t>("In _Selection");
  impl->replace_all_button = emplace_back<button_t>("_All");
  button_t *find_button = emplace_back<button_t>("_Find", true);
  button_t *cancel_button = emplace_back<button_t>("_Cancel");

  cancel_button->set_anchor(this,
                            T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  cancel_button->set_position(-1, -2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_up([this] { focus_previous(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });
  find_button->set_anchor(cancel_button,
                          T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  find_button->set_position(0, -2);
  find_button->connect_activate([this] { find_activated(); });
  find_button->connect_move_focus_right([this] { focus_next(); });
  find_button->connect_move_focus_up([this] { focus_previous(); });
  impl->find_button_up_connection =
      find_button->connect_move_focus_up([this] { focus_previous(); });
  impl->find_button_up_connection.block();

  impl->replace_all_button->set_anchor(
      this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  impl->replace_all_button->set_position(-2, -2);
  impl->replace_all_button->connect_activate(
      [this] { find_activated(find_action_t::REPLACE_ALL); });
  impl->replace_all_button->connect_move_focus_up([this] { focus_previous(); });
  impl->replace_all_button->connect_move_focus_up([this] { focus_previous(); });
  impl->replace_all_button->connect_move_focus_down([this] { focus_next(); });
  impl->replace_all_button->connect_move_focus_down([this] { focus_next(); });
  impl->replace_all_button->connect_move_focus_left([this] { focus_previous(); });
  impl->replace_all_button->hide();

  impl->in_selection_button->set_anchor(
      impl->replace_all_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  impl->in_selection_button->set_position(0, -1);
  impl->in_selection_button->connect_activate(
      [this] { find_activated(find_action_t::REPLACE_IN_SELECTION); });
  impl->in_selection_button->connect_move_focus_up([this] { focus_previous(); });
  impl->in_selection_button->connect_move_focus_down([this] { focus_next(); });
  impl->in_selection_button->connect_move_focus_down([this] { focus_next(); });
  impl->in_selection_button->connect_move_focus_right([this] { focus_next(); });
  impl->in_selection_button->hide();

  find_dialog_t::set_state(_state);
}

find_dialog_t::~find_dialog_t() {}

bool find_dialog_t::set_size(optint height, optint width) {
  (void)height;
  (void)width;
  return true;
}

void find_dialog_t::set_text(string_view str) { impl->find_line->set_text(str); }

#define TOGGLED_CALLBACK(name, flag_name) \
  void find_dialog_t::name##_toggled() { impl->state ^= find_flags_t::flag_name; }
TOGGLED_CALLBACK(backward, BACKWARD)
TOGGLED_CALLBACK(icase, ICASE)
TOGGLED_CALLBACK(wrap, WRAP)
TOGGLED_CALLBACK(transform_backslash, TRANSFROM_BACKSLASH)
TOGGLED_CALLBACK(whole_word, WHOLE_WORD)
#undef TOGGLED_CALLBACK

void find_dialog_t::regex_toggled() {
  impl->state ^= find_flags_t::REGEX;
  impl->transform_backslash_checkbox->set_enabled(!(impl->state & find_flags_t::REGEX));
}

void find_dialog_t::find_activated() { find_activated(find_action_t::FIND); }

void find_dialog_t::find_activated(find_action_t action) {
  std::shared_ptr<finder_t> context;
  std::string error_message;
  context.reset(
      finder_t::create(impl->find_line->get_text(), impl->state, &error_message,
                       impl->replace_line->is_shown() ? &impl->replace_line->get_text() : nullptr)
          .release());
  if (context == nullptr) {
    std::string full_message("Error in search expression: ");
    full_message.append(error_message);
    message_dialog->set_message(full_message);
    message_dialog->center_over(this);
    message_dialog->show();
  }
  hide();
  if (context != nullptr) {
    impl->activate(context, action);
  }
}

void find_dialog_t::set_replace(bool replace) {
  if (replace == impl->replace_line->is_shown()) {
    return;
  }

  if (replace) {
    set_title(_("Replace"));
    dialog_t::set_size(FIND_DIALOG_HEIGHT + 2, None);  // Also forces redraw
    impl->replace_label->show();
    impl->replace_line->show();
    impl->whole_word_checkbox->set_position(3, 2);
    impl->replace_all_button->show();
    impl->in_selection_button->show();
    impl->find_button_up_connection.unblock();
  } else {
    set_title(_("Find"));
    dialog_t::set_size(FIND_DIALOG_HEIGHT, None);  // Also forces redraw
    impl->replace_label->hide();
    impl->replace_line->hide();
    impl->whole_word_checkbox->set_position(2, 2);
    impl->replace_all_button->hide();
    impl->in_selection_button->hide();
    impl->find_button_up_connection.block();
  }
}

void find_dialog_t::set_state(int _state) {
  impl->state = _state;
  impl->whole_word_checkbox->set_state(impl->state & find_flags_t::WHOLE_WORD);
  impl->match_case_checkbox->set_state(!(impl->state & find_flags_t::ICASE));
  impl->regex_checkbox->set_state(impl->state & find_flags_t::REGEX);
  impl->wrap_checkbox->set_state(impl->state & find_flags_t::WRAP);
  impl->transform_backslash_checkbox->set_state(impl->state & find_flags_t::TRANSFROM_BACKSLASH);
  impl->reverse_direction_checkbox->set_state(impl->state & find_flags_t::BACKWARD);
}

_T3_WIDGET_IMPL_SIGNAL(find_dialog_t, activate, std::shared_ptr<finder_t>, find_action_t)

//============= replace_buttons_dialog_t ===============
struct replace_buttons_dialog_t::implementation_t {
  button_t *find_button, *replace_button;
  signal_t<find_action_t> activate;
};

replace_buttons_dialog_t::replace_buttons_dialog_t()
    : dialog_t(3, 60, "Replace", impl_alloc<implementation_t>(0)),
      impl(new_impl<implementation_t>()) {
  int dialog_width;

  button_t *replace_all_button = emplace_back<button_t>("_All");
  replace_all_button->set_position(1, 2);
  replace_all_button->connect_activate([this] { hide(); });
  replace_all_button->connect_activate(
      bind_front(impl->activate.get_trigger(), find_action_t::REPLACE_ALL));
  replace_all_button->connect_move_focus_right([this] { focus_next(); });

  impl->replace_button = emplace_back<button_t>("_Replace");
  impl->replace_button->set_anchor(replace_all_button,
                                   T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->replace_button->set_position(0, 2);
  impl->replace_button->connect_activate([this] { hide(); });
  impl->replace_button->connect_activate(
      bind_front(impl->activate.get_trigger(), find_action_t::REPLACE));
  impl->replace_button->connect_move_focus_left([this] { focus_previous(); });
  impl->replace_button->connect_move_focus_right([this] { focus_next(); });

  impl->find_button = emplace_back<button_t>("_Find");
  impl->find_button->set_anchor(impl->replace_button,
                                T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->find_button->set_position(0, 2);
  impl->find_button->connect_activate([this] { hide(); });
  impl->find_button->connect_activate(
      bind_front(impl->activate.get_trigger(), find_action_t::SKIP));
  impl->find_button->connect_move_focus_left([this] { focus_previous(); });
  impl->find_button->connect_move_focus_right([this] { focus_next(); });

  button_t *cancel_button = emplace_back<button_t>("_Cancel");
  cancel_button->set_anchor(impl->find_button,
                            T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  cancel_button->set_position(0, 2);
  cancel_button->connect_activate([this] { close(); });
  cancel_button->connect_move_focus_left([this] { focus_previous(); });

  dialog_width = replace_all_button->get_width() + impl->replace_button->get_width() +
                 impl->find_button->get_width() + cancel_button->get_width() + 10;
  dialog_t::set_size(None, dialog_width);
}

replace_buttons_dialog_t::~replace_buttons_dialog_t() {}

void replace_buttons_dialog_t::reshow(find_action_t button) {
  show();
  switch (button) {
    case find_action_t::REPLACE:
      set_child_focus(impl->replace_button);
      break;
    case find_action_t::SKIP:
      set_child_focus(impl->find_button);
      break;
    default:
      break;
  }
}

_T3_WIDGET_IMPL_SIGNAL(replace_buttons_dialog_t, activate, find_action_t)

}  // namespace t3widget
