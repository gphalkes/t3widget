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
#ifndef T3_WIDGET_EXPANDER_H
#define T3_WIDGET_EXPANDER_H

#include <memory>
#include <t3widget/widgets/widget.h>
#include <utility>

#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/signals.h>
#include <t3widget/string_view.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>

namespace t3widget {

/** A widget showing an expander, which allows hiding another widget. */
class T3_WIDGET_API expander_t : public widget_t, public widget_container_t, public focus_widget_t {
 private:
  enum expander_focus_t {
    FOCUS_NONE,
    FOCUS_SELF,
    FOCUS_CHILD,
  };

  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

  void focus_up_from_child();

 public:
  /** Create a new expander_t.
      @param _text The text for the smart_label_t shown next to the expander symbol.
  */
  expander_t(string_view text);
  ~expander_t() override;
  /** Set the child widget.

      @note This also automatically sets the size of the expander_t to wrap
      the child widget.
  */
  void set_child(std::unique_ptr<widget_t> _child);

  /** Create a child widget and return the pointer to it. */
  template <typename T, typename... Args>
  T *emplace_child(Args &&... args) {
    T *result = new T(std::forward<Args>(args)...);
    set_child(std::unique_ptr<widget_t>(result));
    return result;
  }

  /** Set the size of the widget to wrap the child widget. */
  void set_size_from_child();
  void set_expanded(bool expand);

  bool process_key(key_t key) override;
  void update_contents() override;
  void set_focus(focus_t _focus) override;
  bool set_size(optint height, optint width) override;
  bool is_hotkey(key_t key) const override;
  void set_enabled(bool enable) override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(const window_component_t *component) const override;
  widget_t *is_child_hotkey(key_t key) const override;
  bool process_mouse_event(mouse_event_t event) override;

  T3_WIDGET_DECLARE_SIGNAL(expanded, bool);
};

}  // namespace t3widget
#endif
