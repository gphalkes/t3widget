/* Copyright (C) 2011,2018 G.P. Halkes
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
#ifndef T3_WIDGET_DUMMYWIDGET_H
#define T3_WIDGET_DUMMYWIDGET_H

#include <t3widget/widgets/widget.h>

namespace t3widget {

/** A widget that does not actually show anything on screen.

    This widget exists mainly to allow dialogs to be written such that there is
    always a widget available, even if no proper widget has been added.
*/
class T3_WIDGET_API dummy_widget_t : public widget_t {
 public:
  bool process_key(key_t key) override;
  virtual bool resize(optint height, optint width, optint top, optint left);
  void update_contents() override;
  void show() override;
  void hide() override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;

  const t3window::window_t *get_base_window() const override;
};

}  // namespace t3widget
#endif
