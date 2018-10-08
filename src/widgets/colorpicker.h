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
#ifndef T3_WIDGET_COLORPICKER_H
#define T3_WIDGET_COLORPICKER_H

#include <t3widget/widgets/widget.h>

namespace t3widget {

class T3_WIDGET_API color_picker_base_t : public widget_t {
 protected:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  color_picker_base_t(bool _fg);
  virtual int xy_to_color(int x, int y) const = 0;
  virtual void color_to_xy(int color, int &x, int &y) const = 0;
  virtual t3_attr_t get_paint_attr(int color) const = 0;
  virtual void paint_color_name(int color);

 public:
  ~color_picker_base_t() override;
  bool process_key(key_t key) override;
  void update_contents() override;
  bool set_size(optint height, optint width) override;
  bool process_mouse_event(mouse_event_t event) override;
  void set_focus(focus_t focus) override;

  /** Change the rendering of the default colors.
      @param attr The colors to use for the default colors.

      When selecting colors, sometimes the result will be combined with
      another set of colors. The undefined color will then be overriden with
      the color to combine with. To show the user what the effect of choosing
      the undefined color is, you can set the colors to use for the undefined
      colors with this function.
  */
  void set_undefined_colors(t3_attr_t attr);
  t3_attr_t get_color();
  void set_color(t3_attr_t attr);

  T3_WIDGET_DECLARE_SIGNAL(activated);
  T3_WIDGET_DECLARE_SIGNAL(selection_changed);
};

class T3_WIDGET_API color_picker_t : public color_picker_base_t {
 protected:
  int xy_to_color(int x, int y) const override;
  void color_to_xy(int color, int &x, int &y) const override;
  t3_attr_t get_paint_attr(int color) const override;
  void paint_color_name(int color) override;

 public:
  color_picker_t(bool _fg);
};

class T3_WIDGET_API color_pair_picker_t : public color_picker_base_t {
 protected:
  int xy_to_color(int x, int y) const override;
  void color_to_xy(int color, int &x, int &y) const override;
  t3_attr_t get_paint_attr(int color) const override;

 public:
  color_pair_picker_t();
};

}  // namespace t3widget
#endif
