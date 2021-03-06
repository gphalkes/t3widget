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
#ifndef T3_WIDGET_FRAME_H
#define T3_WIDGET_FRAME_H

#include <memory>
#include <t3widget/widgets/widget.h>
#include <utility>

#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>

namespace t3widget {

/** A widget showing a frame.

    This widget is primarily meant to draw frames around other widgets. The
    child widget is automatically resized to fit the frame.
*/
class T3_WIDGET_API frame_t : public widget_t, public container_t {
 public:
  /** Constants defining bits determining which parts of the frame should be covered by the child
   * widget. */
  enum frame_dimension_t {
    AROUND_ALL = 0, /**< Constant indicating the frame should go around the widget on all sides. */
    COVER_BOTTOM = 1, /**< Bit indicating that the bottom line of the frame should be covered by the
                         child widget. */
    COVER_RIGHT = 2,  /**< Bit indicating that the right line of the frame should be covered by the
                         child widget. */
    COVER_LEFT = 4,   /**< Bit indicating that the left line of the frame should be covered by the
                         child widget. */
    COVER_TOP = 8 /**< Bit indicating that the top line of the frame should be covered by the child
                     widget. */
  };

  /** Create a new frame_t.
      @param _dimension Bit map indicating which parts of the frame should be covered by the child
     widget.

      For some widgets, it may provide a more aesthetic appearance if part
      of the widget overlaps the frame. This is mostly the case for widgets
      that have a scrollbar on one side, like for example the list_pane_t.
      Making the scrollbar overlap the frame looks better and leaves more
      space while still clearly delinating the edges of the list.
  */
  frame_t(frame_dimension_t _dimension = AROUND_ALL);
  ~frame_t() override;
  /** Set the child widget. */
  void set_child(std::unique_ptr<widget_t> _child);

  /** Create a child widget and return the pointer to it. */
  template <typename T, typename... Args>
  T *emplace_child(Args &&... args) {
    T *result = new T(std::forward<Args>(args)...);
    set_child(std::unique_ptr<widget_t>(result));
    return result;
  }

  bool process_key(key_t key) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool set_size(optint height, optint width) override;
  bool accepts_focus() const override;
  bool is_hotkey(key_t key) const override;
  void set_enabled(bool enable) override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(const window_component_t *component) const override;

 private:
  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;
};

}  // namespace t3widget
#endif
