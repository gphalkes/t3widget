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
#include <cerrno>
#include <cstring>
#include <string>

#include <t3window/window.h>

#include "t3widget/dialogs/filedialog.h"
#include "t3widget/internal.h"
#include "t3widget/main.h"
#include "t3widget/util.h"

namespace t3_widget {

static key_t nul = 0;

/* FIXME: TODO:
        - path-name cleansing ( /foo/../bar -> /bar, ////usr -> /usr etc.)
        - optimize the case where filter is "*"
*/
file_dialog_t::file_dialog_t(int height, int width, const char *_title)
    : dialog_t(height, width, _title), impl(new implementation_t()) {
  smart_label_t *name_label;

  name_label = new smart_label_t("_Name", true);
  name_label->set_position(1, 2);
  impl->name_offset =
      name_label->get_width() + 2 + 1;  // 2 for offset of "Name", 1 for space in ": ["
  impl->file_line = new text_field_t();
  impl->file_line->set_anchor(name_label,
                              T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->file_line->set_position(0, 1);
  impl->file_line->set_size(None, width - 2 - impl->name_offset);
  impl->file_line->connect_activate(signals::mem_fun0(this, &file_dialog_t::ok_callback));
  impl->file_line->set_label(name_label);
  impl->file_line->set_key_filter(&nul, 1, false);
  impl->file_line->set_autocomplete(&impl->view);

  impl->file_pane = new file_pane_t();
  impl->file_pane->set_file_list(&impl->names);
  impl->file_pane->set_text_field(impl->file_line);
  impl->file_pane->connect_activate(signals::mem_fun1(this, &file_dialog_t::ok_callback));
  impl->file_pane->set_file_list(&impl->view);

  impl->file_pane_frame = new frame_t(frame_t::COVER_BOTTOM);
  impl->file_pane_frame->set_size(height - 4, width - 4);
  impl->file_pane_frame->set_position(2, 2);
  impl->file_pane_frame->set_child(impl->file_pane);

  impl->show_hidden_box = new checkbox_t(false);
  impl->show_hidden_box->set_anchor(impl->file_pane_frame,
                                    T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->show_hidden_box->set_position(0, 0);
  impl->show_hidden_box->connect_toggled(signals::mem_fun(this, &file_dialog_t::refresh_view));
  impl->show_hidden_box->connect_activate(signals::mem_fun0(this, &file_dialog_t::ok_callback));
  impl->show_hidden_box->connect_move_focus_up(
      signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->show_hidden_box->connect_move_focus_right(
      signals::mem_fun(this, &file_dialog_t::focus_next));

  impl->show_hidden_label = new smart_label_t("_Show hidden");
  impl->show_hidden_label->set_anchor(impl->show_hidden_box,
                                      T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->show_hidden_label->set_position(0, 1);
  impl->show_hidden_box->set_label(impl->show_hidden_label);

  impl->cancel_button = new button_t("_Cancel");
  impl->cancel_button->set_anchor(
      this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
  impl->cancel_button->set_position(-1, -2);
  impl->cancel_button->connect_activate(signals::mem_fun(this, &file_dialog_t::close));
  impl->cancel_button->connect_move_focus_left(
      signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->cancel_button_up_connection = impl->cancel_button->connect_move_focus_up(signals::bind(
      signals::mem_fun(this, &file_dialog_t::set_child_focus), impl->file_pane_frame));
  impl->ok_button = new button_t("_OK", true);
  impl->ok_button->set_anchor(impl->cancel_button,
                              T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  impl->ok_button->set_position(0, -2);
  impl->ok_button->connect_activate(signals::mem_fun0(this, &file_dialog_t::ok_callback));
  impl->ok_button->connect_move_focus_left(signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->ok_button->connect_move_focus_right(signals::mem_fun(this, &file_dialog_t::focus_next));
  impl->ok_button_up_connection = impl->ok_button->connect_move_focus_up(signals::bind(
      signals::mem_fun(this, &file_dialog_t::set_child_focus), impl->file_pane_frame));

  push_back(name_label);
  push_back(impl->file_line);
  push_back(impl->file_pane_frame);
  push_back(impl->show_hidden_box);
  push_back(impl->show_hidden_label);
  push_back(impl->ok_button);
  push_back(impl->cancel_button);
}

widget_t *file_dialog_t::get_anchor_widget() { return impl->show_hidden_label; }

void file_dialog_t::insert_extras(widget_t *widget) { widgets.insert(widgets.end() - 2, widget); }

void file_dialog_t::set_options_widget(widget_t *options) {
  focus_widget_t *focus_widget;

  if (impl->option_widget_set) {
    return;
  }

  set_widget_parent(options);
  /* Make the file pane one line less high. */
  impl->file_pane_frame->set_size(t3_win_get_height(window) - 5, None);

  impl->option_widget_set = true;
  insert_extras(options);
  options->set_anchor(impl->file_pane_frame,
                      T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
  options->set_position(0, 0);

  impl->cancel_button_up_connection.disconnect();
  impl->cancel_button->connect_move_focus_up(
      signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->cancel_button->connect_move_focus_up(
      signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->ok_button->connect_move_focus_left(signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->ok_button_up_connection.disconnect();
  impl->ok_button->connect_move_focus_up(signals::mem_fun(this, &file_dialog_t::focus_previous));
  impl->show_hidden_box->connect_move_focus_down(
      signals::bind(signals::mem_fun(this, &file_dialog_t::set_child_focus), impl->ok_button));
  dynamic_cast<focus_widget_t *>(*(widgets.end() - 4))
      ->connect_move_focus_down(
          signals::bind(signals::mem_fun(this, &file_dialog_t::set_child_focus), impl->ok_button));

  if ((focus_widget = dynamic_cast<focus_widget_t *>(options)) != nullptr) {
    focus_widget->connect_move_focus_up(signals::bind(
        signals::mem_fun(this, &file_dialog_t::set_child_focus), impl->file_pane_frame));
    focus_widget->connect_move_focus_left(signals::mem_fun(this, &file_dialog_t::focus_previous));
    focus_widget->connect_move_focus_down(signals::mem_fun(this, &file_dialog_t::focus_next));
  }
}

bool file_dialog_t::set_size(optint height, optint width) {
  bool result = true;
  result &= dialog_t::set_size(height, width);

  result &= impl->file_line->set_size(None, t3_win_get_width(window) - 3 - impl->name_offset);
  result &= impl->file_pane_frame->set_size(height - 4 - impl->option_widget_set, width - 4);
  return result;
}

int file_dialog_t::set_file(const char *file) {
  size_t idx;
  std::string file_string;
  int result;

  impl->current_dir = get_directory(file);
  sanitize_dir(&impl->current_dir);

  if (file == nullptr) {
    file_string.clear();
  } else {
    file_string = file;
  }

  result = impl->names.load_directory(&impl->current_dir);

  idx = file_string.rfind('/');
  if (idx != std::string::npos) {
    file_string.erase(0, idx + 1);
  }

  impl->file_line->set_autocomplete(&impl->names);
  impl->file_line->set_text(&file_string);
  refresh_view();
  return result;
}

void file_dialog_t::reset() {
  impl->file_line->set_text("");
  impl->file_pane->reset();
}

void file_dialog_t::ok_callback() {
  std::string pass_result;
  convert_lang_codeset(impl->file_line->get_text(), &pass_result, false);
  ok_callback(&pass_result);
}

void file_dialog_t::ok_callback(const std::string *file) {
  if (file->size() == 0) {
    return;
  }

  if (is_dir(&impl->current_dir, file->c_str()) || file->compare("..") == 0) {
    change_dir(file);
    impl->file_line->set_text("");
  } else {
    std::string full_name;
    if ((*file)[0] != '/') {
      full_name += impl->current_dir;
      full_name += "/";
    }
    full_name += *file;
    hide();
    file_selected(&full_name);
  }
}

void file_dialog_t::change_dir(const std::string *dir) {
  file_name_list_t new_names;
  std::string new_dir, file_string;
  int error;

  if (dir->compare("..") == 0) {
    size_t idx = impl->current_dir.rfind('/');

    if (idx == std::string::npos || idx == impl->current_dir.size() - 1) {
      return;
    }

    file_string = impl->current_dir.substr(idx + 1);
    if (idx == 0) {
      idx++;
    }
    new_dir = impl->current_dir.substr(0, idx);
  } else if ((*dir)[0] == '/') {
    new_dir = *dir;
  } else {
    new_dir = impl->current_dir;
    if (impl->current_dir.compare("/") != 0) {
      new_dir += "/";
    }
    new_dir += *dir;
  }

  sanitize_dir(&new_dir);

  /* Check whether we can load the dir. If not, show message and don't change state. */
  if ((error = new_names.load_directory(&new_dir)) != 0) {
    std::string message = _("Couldn't change to directory '");
    message += dir->c_str();
    message += "': ";
    message += strerror(errno);
    message_dialog->set_message(&message);
    message_dialog->center_over(this);
    message_dialog->show();
    return;
  }

  impl->names = new_names;
  impl->current_dir = new_dir;
  impl->view.set_filter(signals::bind(signals::ptr_fun(glob_filter), get_filter(),
                                      impl->show_hidden_box->get_state()));
  impl->file_pane->reset();
}

void file_dialog_t::refresh_view() {
  convert_lang_codeset(get_filter(), &impl->lang_codeset_filter, false);
  if (impl->lang_codeset_filter.size() == 0) {
    impl->lang_codeset_filter = "*";
  }
  impl->view.set_filter(signals::bind(signals::ptr_fun(glob_filter), &impl->lang_codeset_filter,
                                      impl->show_hidden_box->get_state()));

  impl->file_pane->set_file(impl->file_line->get_text());
}

//=========================== open_file_dialog_t ============================
void open_file_dialog_t::filter_text_field_t::set_focus(focus_t _focus) {
  bool old_focus = has_focus();
  text_field_t::set_focus(_focus);
  if (old_focus && !has_focus()) {
    lose_focus();
  }
}

open_file_dialog_t::open_file_dialog_t(int height, int width)
    : file_dialog_t(height, width, "Open File"), impl(new implementation_t()) {
  impl->filter_label = new smart_label_t("_Filter", true);
  container_t::set_widget_parent(impl->filter_label);
  impl->filter_label->set_anchor(get_anchor_widget(),
                                 T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->filter_label->set_position(0, 2);
  impl->filter_offset = impl->filter_label->get_width() + 1;
  impl->filter_width = std::min(std::max(10, width - 60), 25);
  impl->filter_line = new filter_text_field_t();
  container_t::set_widget_parent(impl->filter_line);
  impl->filter_line->set_anchor(impl->filter_label,
                                T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->filter_line->set_position(0, 1);
  impl->filter_line->set_size(None, impl->filter_width);
  impl->filter_line->set_text("*");
  impl->filter_line->connect_activate(signals::mem_fun(this, &open_file_dialog_t::refresh_view));
  impl->filter_line->connect_lose_focus(signals::mem_fun(this, &open_file_dialog_t::refresh_view));
  impl->filter_line->connect_move_focus_up(
      signals::mem_fun(this, &open_file_dialog_t::focus_previous));
  impl->filter_line->connect_move_focus_up(
      signals::mem_fun(this, &open_file_dialog_t::focus_previous));

  impl->filter_line->set_label(impl->filter_label);
  impl->filter_line->set_key_filter(&nul, 1, false);

  insert_extras(impl->filter_label);
  insert_extras(impl->filter_line);
}

const std::string *open_file_dialog_t::get_filter() { return impl->filter_line->get_text(); }

bool open_file_dialog_t::set_size(optint height, optint width) {
  bool result = file_dialog_t::set_size(height, width);
  if (width.is_valid()) {
    result &= impl->filter_line->set_size(None, std::min(std::max(10, width - 60), 25));
  }
  return result;
}

void open_file_dialog_t::reset() {
  file_dialog_t::reset();
  impl->filter_line->set_text("*");
  refresh_view();
}

//=========================== save_as_dialog_t ============================
std::string save_as_dialog_t::empty_filter("*");

save_as_dialog_t::save_as_dialog_t(int height, int width)
    : file_dialog_t(height, width, "Save File As"), impl(new implementation_t()) {
  impl->create_button = new button_t("Create Folder");
  container_t::set_widget_parent(impl->create_button);
  impl->create_button->set_anchor(get_anchor_widget(),
                                  T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
  impl->create_button->set_position(0, 2);
  impl->create_button->connect_activate(signals::mem_fun(this, &save_as_dialog_t::create_folder));
  insert_extras(impl->create_button);
}

void save_as_dialog_t::create_folder() {
  message_dialog->center_over(this);
  message_dialog->set_message("This function has not been implemented yet. Sorry.");
  message_dialog->show();
  // FIXME: create folder here
}

};  // namespace
