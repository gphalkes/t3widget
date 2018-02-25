/* Copyright (C) 2012-2013,2018 G.P. Halkes
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
#ifndef T3_WIDGET_WIDGETGROUP_H
#define T3_WIDGET_WIDGETGROUP_H

#include <t3widget/widgets/widget.h>

namespace t3_widget {

/** A widget aggregating several widgets into a single widget.

    This widget is useful for packing multiple widgets into for example a
    single frame_t or expander_t.
*/
class T3_WIDGET_API widget_group_t : public widget_t, public container_t, public focus_widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t {
    widgets_t children;
    int current_child;
    bool has_focus;
    implementation_t() : current_child(-1), has_focus(false) {}
  };
  std::unique_ptr<implementation_t> impl;

  bool focus_next_int();
  bool focus_previous_int();

 public:
  widget_group_t();
  ~widget_group_t() override;

  bool process_key(key_t key) override;
  void update_contents() override;
  void set_focus(focus_t _focus) override;
  bool set_size(optint height, optint width) override;
  bool accepts_focus() override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(window_component_t *component) override;
  bool is_hotkey(key_t key) override;

  /** Add a child widget. */
  virtual void add_child(widget_t *child);

  void focus_next();
  void focus_previous();
};

}  // namespace
#endif
