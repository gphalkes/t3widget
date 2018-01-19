/* Copyright (C) 2011-2013 G.P. Halkes
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
#ifndef T3_WIDGET_SCROLLBAR_H
#define T3_WIDGET_SCROLLBAR_H

#include <t3widget/widgets/widget.h>

namespace t3_widget {

class T3_WIDGET_API scrollbar_t : public widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t {
    int length;
    int range, start, used;
    int before, slider_size;
    int button_down_pos;
    bool vertical;
    bool dragging;
    implementation_t(bool _vertical)
        : length(3),
          range(1),
          start(0),
          used(1),
          before(0),
          slider_size(length - 2),
          button_down_pos(0),
          vertical(_vertical),
          dragging(false) {}
  };
  pimpl_ptr<implementation_t>::t impl;

 public:
  scrollbar_t(bool _vertical);
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  bool accepts_focus() override;
  void set_focus(focus_t focus) override;
  bool process_mouse_event(mouse_event_t event) override;

  void set_parameters(int _range, int _start, int _used);

  enum step_t {
    FWD_SMALL,   /**< Mouse click on arrow symbol. */
    FWD_MEDIUM,  /**< Scroll wheel over bar. */
    FWD_PAGE,    /**< Mouse click on space between arrow and indicator. */
    BACK_SMALL,  /**< Mouse click on arrow symbol. */
    BACK_MEDIUM, /**< Scroll wheel over bar. */
    BACK_PAGE    /**< Mouse click on space between arrow and indicator. */
  };

  T3_WIDGET_SIGNAL(clicked, void, step_t);
  T3_WIDGET_SIGNAL(dragged, void, int);
};

};  // namespace
#endif
