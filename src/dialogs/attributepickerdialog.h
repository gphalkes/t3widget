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
#ifndef T3_WIDGET_ATTRIBUTEPICKERDIALOG_H
#define T3_WIDGET_ATTRIBUTEPICKERDIALOG_H

#include <string>
#include <t3widget/dialogs/dialog.h>
#include <t3widget/util.h>

#include <t3widget/key.h>
#include <t3widget/signals.h>
#include <t3widget/widget_api.h>
#include <t3widget/widgets/widget.h>
#include <t3window/terminal.h>

namespace t3widget {

class T3_WIDGET_API attribute_test_line_t;

class T3_WIDGET_API attribute_picker_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

  void attribute_changed();
  void ok_activate();
  void default_activate();
  void group_expanded(bool state);
  t3_attr_t get_attribute();

 public:
  attribute_picker_dialog_t(optional<std::string> _title = "Attribute", bool with_default = true);
  ~attribute_picker_dialog_t() override;
  void show() override;

  void set_attribute(t3_attr_t attr);
  /** Set the base attributes for the attribute picker.
      @param attr The base attributes to use

      When selecting attributes, sometimes the result will be combined with
      another set of attributes. To show the user what the effect of choosing
      the a set of attributes is, you can set the base attributes with this function.
  */
  void set_base_attributes(t3_attr_t attr);

  T3_WIDGET_DECLARE_SIGNAL(attribute_selected, t3_attr_t);
  T3_WIDGET_DECLARE_SIGNAL(default_selected);
};

class T3_WIDGET_API attribute_test_line_t : public widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

 public:
  attribute_test_line_t();
  ~attribute_test_line_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  bool accepts_focus() const override;

  void set_attribute(t3_attr_t attr);
};

}  // namespace t3widget
#endif
