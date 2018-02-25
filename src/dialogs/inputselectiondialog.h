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
#ifndef T3_WIDGET_INPUTSELECTIONDIALOG_H
#define T3_WIDGET_INPUTSELECTIONDIALOG_H

#include <memory>
#include <string>

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/checkbox.h>
#include <t3widget/widgets/frame.h>
#include <t3widget/widgets/label.h>
#include <t3widget/widgets/textwindow.h>

namespace t3_widget {

class T3_WIDGET_API input_selection_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t {
    std::unique_ptr<text_buffer_t> text;
    frame_t *text_frame, *label_frame;
    text_window_t *text_window;
    label_t *key_label;
    checkbox_t *enable_simulate_box, *disable_timeout_box;
    int old_timeout;
  };
  std::unique_ptr<implementation_t> impl;

 public:
  input_selection_dialog_t(int height, int width, text_buffer_t *_text = nullptr);
  bool set_size(optint height, optint width) override;
  bool process_key(key_t key) override;
  void show() override;

  void cancel();
  void ok_activated();
  void check_state();

  static text_buffer_t *get_default_text();

  T3_WIDGET_SIGNAL(activate);
};

}  // namespace
#endif
