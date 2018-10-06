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
#ifndef T3_WIDGET_WIDGETS_H
#define T3_WIDGET_WIDGETS_H

#include <functional>
#include <cstddef>
#include <deque>
#include <memory>
#include <t3widget/interfaces.h>

#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/signals.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>
#include <t3window/window.h>

namespace t3widget {

/** Base class for widgets. */
class T3_WIDGET_API widget_t : public virtual window_component_t,
                               public mouse_target_t,
                               public impl_allocator_t {
 private:
  friend class container_t;

  /** Default parent for widgets, making them invisible. */
  static t3window::window_t default_parent;

  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

 protected:
  bool reset_redraw();

  /** Constructor which creates a default @c t3_window_t with @p height and @p width. */
  widget_t(int height, int width, bool register_as_mouse_target = true, size_t impl_size = 0);
  /** Constructor which does not create a default t3_window_t.
      This constructor should only rarely be necessary. Widgets using this
      constructor should call either #init_window, or #init_unbacked_window.
  */
  widget_t(size_t impl_size = 0);

  /** Initialize the #window with a @c t3_window_t with @p height and @p width. */
  void init_window(int height, int width, bool register_as_mouse_target = true);
  /** Initialize the #window with an unbacked @c t3_window_t with @p height and @p width. */
  void init_unbacked_window(int height, int width, bool register_as_mouse_target = false);

 public:
  ~widget_t() override;

  /** Query whether key is a hotkey for this widget. */
  virtual bool is_hotkey(key_t key) const;
  /** Query whether this widget accepts focus. */
  virtual bool accepts_focus() const;
  void set_position(optint top, optint left) override;
  void show() override;
  void hide() override;
  /** Set this widget's anchor.
      Use @p anchor to position this window. See libt3window's
      t3_win_set_anchor for details on the @p relation parameter.
  */
  virtual void set_anchor(window_component_t *anchor, int relation);
  void force_redraw() override;
  /** Set the enabled status of this widget.
      When a widget is not enabled, it will not accept focus.
  */
  virtual void set_enabled(bool enable);
  /** Query the enabled status of this widget. */
  virtual bool is_enabled() const;
  /** Query the visibility status of this widget. */
  virtual bool is_shown() const;
  void set_focus(focus_t focus) override;
  bool process_mouse_event(mouse_event_t event) override;
};

/** Base class for widgets that take focus. */
class T3_WIDGET_API focus_widget_t {
 public:
  focus_widget_t(impl_allocator_t *allocator = nullptr);
  ~focus_widget_t();

  /** Connect a callback to be called on emission of the move_focus_left_signal. */
  T3_WIDGET_DECLARE_SIGNAL(move_focus_left);
  /** Connect a callback to be called on emission of the move_focus_right_signal. */
  T3_WIDGET_DECLARE_SIGNAL(move_focus_right);
  /** Connect a callback to be called on emission of the move_focus_up_signal. */
  T3_WIDGET_DECLARE_SIGNAL(move_focus_up);
  /** Connect a callback to be called on emission of the move_focus_down_signal. */
  T3_WIDGET_DECLARE_SIGNAL(move_focus_down);

 protected:
  /** Emit signal when the user pressed the left arrow key and focus should move. */
  void move_focus_left() const;
  /** Emit signal when the user pressed the right arrow key and focus should move. */
  void move_focus_right() const;
  /** Emit signal when the user pressed the up arrow key and focus should move. */
  void move_focus_up() const;
  /** Emit signal when the user pressed the down arrow key and focus should move. */
  void move_focus_down() const;

  std::function<void()> get_move_focus_left_trigger() const;
  std::function<void()> get_move_focus_right_trigger() const;
  std::function<void()> get_move_focus_up_trigger() const;
  std::function<void()> get_move_focus_down_trigger() const;

  struct T3_WIDGET_LOCAL implementation_t;

  implementation_t *impl;

  friend class impl_allocator_t;
};

template <>
size_t impl_allocator_t::impl_alloc<focus_widget_t::implementation_t>(size_t req);

class T3_WIDGET_API widget_container_t : public container_t {
 public:
  virtual widget_t *is_child_hotkey(key_t key) const = 0;
};

using widgets_t = std::deque<std::unique_ptr<widget_t>>;

}  // namespace t3widget
#endif
