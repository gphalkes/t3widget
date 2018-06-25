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
#ifndef T3_WIDGET_MESSAGEDIALOG_H
#define T3_WIDGET_MESSAGEDIALOG_H

#include <initializer_list>
#include <string>

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/button.h>
#include <t3widget/widgets/textwindow.h>

namespace t3widget {

class text_line_t;

class T3_WIDGET_API message_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  bool process_key(key_t key) override;

  T3_WIDGET_DECLARE_SIGNAL(activate_internal);

 public:
  message_dialog_t(int width, optional<std::string> _title, std::initializer_list<string_view> buttons);
  ~message_dialog_t() override;
  void set_message(string_view message);
  void set_max_text_height(int max);

  connection_t connect_activate(std::function<void()> _slot, size_t idx);
};

}  // namespace t3widget
#endif
