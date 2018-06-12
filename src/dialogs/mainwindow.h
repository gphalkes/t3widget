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
#ifndef T3_WIDGET_MAINWINDOW_H
#define T3_WIDGET_MAINWINDOW_H

#include <t3widget/dialogs/dialog.h>
#include <t3widget/interfaces.h>

namespace t3widget {

/** Base class for the application's main window.

    An application's main window is a special type of dialog. It can not be
    resized by calling the #set_size member, but is instead resized by a call to
    the #set_size_real function initiated from the @c resize signal. The
    #set_size function is called on a resize however, and should be overriden
    to perform resizing of child widgets.
*/
class T3_WIDGET_API main_window_base_t : public dialog_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;
  /** Resize the main_window_base_t.
      Called from the @c resize signal.
  */
  void set_size_real(int height, int width);

 protected:
  /** Construct a new main_window_base_t. */
  main_window_base_t();
  void close() override;

 public:
  ~main_window_base_t() override;
  void update_contents() override;
  bool set_size(optint height, optint width) override;
  void set_position(optint top, optint left) override;
  void show() override;
};

}  // namespace t3widget
#endif
