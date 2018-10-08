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
#ifndef T3_WIDGET_FILEDIALOG_H
#define T3_WIDGET_FILEDIALOG_H

#include <t3widget/dialogs/dialog.h>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/button.h>
#include <t3widget/widgets/checkbox.h>
#include <t3widget/widgets/filepane.h>
#include <t3widget/widgets/frame.h>

namespace t3widget {

class T3_WIDGET_API file_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

 protected:
  file_dialog_t(int height, int width, optional<std::string> _title, size_t impl_size = 0);

  widget_t *get_anchor_widget() const;
  const widget_t *get_insert_before_widget() const;
  void ok_callback();
  void ok_callback(const std::string &file);
  virtual const std::string &get_filter() const = 0;

 public:
  ~file_dialog_t() override;
  bool set_size(optint height, optint width) override;
  void change_dir(const std::string &dir);
  /** Sets the file listing and text input from the file name.
      @returns 0 on success or the value of @c errno on error. */
  virtual int set_from_file(string_view file);
  void refresh_view();
  void set_options_widget(std::unique_ptr<widget_t> options);
  virtual void reset();

  T3_WIDGET_DECLARE_SIGNAL(file_selected, const std::string &);
};

class T3_WIDGET_API open_file_dialog_t : public file_dialog_t {
 private:
  class T3_WIDGET_LOCAL filter_text_field_t;

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  const std::string &get_filter() const override;

 public:
  open_file_dialog_t(int height, int width);
  ~open_file_dialog_t() override;
  bool set_size(optint height, optint width) override;
  void reset() override;
};

class T3_WIDGET_API save_as_dialog_t : public file_dialog_t {
 private:
  static std::string empty_filter;

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

 protected:
  const std::string &get_filter() const override { return empty_filter; }

 public:
  save_as_dialog_t(int height, int width);
  ~save_as_dialog_t() override;
  void create_folder();
};

}  // namespace t3widget
#endif
