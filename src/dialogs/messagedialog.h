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

#include <string>
#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/button.h>
#include <t3widget/widgets/textwindow.h>

namespace t3_widget {

class text_line_t;

class T3_WIDGET_API message_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t {
    text_window_t *text_window;
    int height, max_text_height;

    implementation_t();
  };
  pimpl_ptr<implementation_t>::t impl;

  bool process_key(key_t key) override;

  T3_WIDGET_SIGNAL(activate_internal, void);

 public:
  message_dialog_t(int width, const char *_title, ...);
  ~message_dialog_t() override;
  void set_message(const char *message, size_t length);
  void set_message(const char *message);
  void set_message(const std::string *message);
  void set_max_text_height(int max);

  signals::connection connect_activate(const signals::slot<void> &_slot, size_t idx);
};

};  // namespace
#endif
