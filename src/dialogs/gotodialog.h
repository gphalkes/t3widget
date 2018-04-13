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
#ifndef T3_WIDGET_GOTODIALOG_H
#define T3_WIDGET_GOTODIALOG_H

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/textfield.h>

namespace t3_widget {

class T3_WIDGET_API goto_dialog_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  std::unique_ptr<implementation_t> impl;

  void ok_activate();

 public:
  goto_dialog_t();
  ~goto_dialog_t() override;
  bool set_size(optint height, optint width) override;
  void reset();

  connection_t connect_activate(std::function<void(int)> cb);
};

}  // namespace
#endif
