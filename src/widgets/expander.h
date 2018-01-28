/* Copyright (C) 2012 G.P. Halkes
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
#ifndef T3_WIDGET_EXPANDER_H
#define T3_WIDGET_EXPANDER_H

#include <memory>

#include <t3widget/widgets/smartlabel.h>
#include <t3widget/widgets/widget.h>

namespace t3_widget {

/** A widget showing an expander, which allows hiding another widget. */
class T3_WIDGET_API expander_t : public widget_t, public widget_container_t, public focus_widget_t {
 private:
  enum expander_focus_t {
    FOCUS_NONE,
    FOCUS_SELF,
    FOCUS_CHILD,
  };

  struct implementation_t {
    expander_focus_t focus, last_focus;
    bool is_expanded;
    smart_label_text_t label;
    unique_t3_window_ptr symbol_window;
    std::unique_ptr<widget_t> child; /**< The widget to enclose. */
    int full_height;
    signals::connection move_up_connection, move_down_connection, move_right_connection,
        move_left_connection;
    implementation_t(const char *text)
        : focus(FOCUS_NONE),
          last_focus(FOCUS_NONE),
          is_expanded(false),
          label(text),
          full_height(2) {}
  };
  pimpl_ptr<implementation_t>::t impl;

  void focus_up_from_child();

 public:
  /** Create a new expander_t.
      @param _text The text for the smart_label_t shown next to the expander symbol.
  */
  expander_t(const char *text);
  /** Set the child widget.

      @note This also automatically sets the size of the expander_t to wrap
      the child widget.
  */
  void set_child(widget_t *_child);
  void set_expanded(bool expand);

  bool process_key(key_t key) override;
  void update_contents() override;
  void set_focus(focus_t _focus) override;
  bool set_size(optint height, optint width) override;
  bool is_hotkey(key_t key) override;
  void set_enabled(bool enable) override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(window_component_t *component) override;
  widget_t *is_child_hotkey(key_t key) override;
  bool process_mouse_event(mouse_event_t event) override;

  T3_WIDGET_SIGNAL(expanded, void, bool);
};

};  // namespace
#endif
