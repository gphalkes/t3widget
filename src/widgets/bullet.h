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
#ifndef T3_WIDGET_BULLET_H
#define T3_WIDGET_BULLET_H

#include <t3widget/widgets/widget.h>

namespace t3_widget {

/** Class implementing a bullet driven by a boolean valued callback.

    The main use of this class is to show boolean information about an item in
    a list. An example is the list of open buffers in the Tilde text editor,
    where a bullet is used to indicate whether a buffer is currently shown in
    an edit window.
*/
class T3_WIDGET_API bullet_t : public widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

 public:
  /** Create a new bullet_t.
      @param _source Callback to determine required display state.
  */
  bullet_t(std::function<bool()> _source);
  ~bullet_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
};

}  // namespace
#endif
