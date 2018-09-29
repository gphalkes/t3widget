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
#ifndef T3_WIDGET_MULTIWIDGET_H
#define T3_WIDGET_MULTIWIDGET_H

#include <t3widget/widgets/widget.h>

namespace t3widget {

class T3_WIDGET_API multi_widget_t : public widget_t, public focus_widget_t, public container_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

 public:
  multi_widget_t();
  ~multi_widget_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool accepts_focus() override;
  void force_redraw() override;
  void set_enabled(bool enable) override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(const window_component_t *component) const override;

  /* Width is negative for fixed width widgets, positive for proportion */
  void push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys);
  void resize_widgets();
};

}  // namespace t3widget
#endif
