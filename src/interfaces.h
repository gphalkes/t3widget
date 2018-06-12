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
#ifndef T3_WIDGET_INTERFACES_H
#define T3_WIDGET_INTERFACES_H

#include <list>
#include <map>
#include <t3window/window.h>

#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/util.h>

namespace t3widget {

/** Abstract base class for all items displayed on screen. */
class T3_WIDGET_API window_component_t {
 protected:
  /** The t3_window_t used for presenting this item on screen (see libt3window). */
  t3_window::window_t window;

 public:
  enum focus_t { FOCUS_OUT = 0, FOCUS_SET, FOCUS_IN_FWD, FOCUS_IN_BCK, FOCUS_REVERT };

  /** Base constructor. */
  window_component_t();
  /** Base destructor. */
  /* Virtual destructor is required for proper functioning of the delete
     operator in multiple-inheritance situations. */
  virtual ~window_component_t();
  /** Retrieve the window_t for this window_component_t.
      The returned pointer should be used only for setting anchor
      positions of other window_component_t's and similar operations.
  */
  virtual const t3_window::window_t *get_base_window() const;
  /** Handle a key press by the user.
      @return A boolean indicating whether this window_component_t handled the
          key press.
  */
  virtual bool process_key(key_t key) = 0;
  /** Move the window_component_t to a specified position.
      Note that the position is relative to the anchor point. */
  virtual void set_position(optint top, optint left) = 0;
  /** Set the size of this window_component_t.
      @param height The new height of the window_component_t in cells, or #None if it should remain
     unchanged.
      @param width The new width of the window_component_t in cells, or #None if it should remain
     unchanged.
          @return A boolean indicating whether all allocations required for the resize
          operation succeeded.
  */
  virtual bool set_size(optint height, optint width) = 0;
  /** Update the contents of the window. */
  virtual void update_contents() = 0;
  /** Set whether this window_component_t has the input focus.
      Note that this merely notifies the window_component_t that it should change it's
      appearance to indicate the user that this window_component_t has the input focus.
      Which window_component_t actually receives the input events is handled outside of
      the window_component_t.
  */
  virtual void set_focus(focus_t focus) = 0;
  /** Display the window_component_t. */
  virtual void show() = 0;
  /** Hide the window_component_t. */
  virtual void hide() = 0;
  /** Request that this window_component_t be completely redrawn. */
  virtual void force_redraw() = 0;
};

class widget_t;
/** Base class for window_component_t's that are the parents of other window_component_t's */
class T3_WIDGET_API container_t : protected virtual window_component_t {
 protected:
  /** Make @p widget a child window of this container_t, by setting its parent window. */
  virtual bool set_widget_parent(window_component_t *widget);
  /** Unset the parent window for a @p widget. */
  virtual void unset_widget_parent(window_component_t *widget);

 public:
  /** Set the focus to a specific window component. */
  virtual void set_child_focus(window_component_t *target) = 0;
  /** Determine whether a window_compnent_t is a child of the container_t. */
  virtual bool is_child(window_component_t *component) = 0;
};

/** Base class for components which need to center dialogs.

    This base class is specifically made for widgets like edit_window_t which need to show dialogs.
    In some cases it is better to center those dialogs over the widget itself, but in other cases it
    is more intuitive to center those dialogs over the containing window_component_t of the widget.
    The latter may be the case when the widget is itself part of a dialog. To allow both cases, this
    interface defines a function to set the window_component_t used for centering. */
class T3_WIDGET_API center_component_t : protected virtual window_component_t {
 protected:
  /** The window_component_t to center over. */
  const window_component_t *center_window;

 public:
  /** Create a new center_component_t.
      The #center_window member will be set to @c this.
  */
  center_component_t();
  /** Set the window_component_t to center over. */
  virtual void set_center_window(const window_component_t *_center_window);
};

class T3_WIDGET_API mouse_target_t : protected virtual window_component_t {
 private:
  using mouse_target_map_t = std::map<const t3_window_t *, mouse_target_t *>;

  static mouse_target_map_t targets;
  static mouse_target_t *grab_target;
  static const t3_window_t *grab_window;

 protected:
  mouse_target_t(bool use_window = true);

 public:
  /** Register a window to receive mouse events. */
  void register_mouse_target(const t3_window::window_t *target);

  /** Unregister a window to receive mouse events. */
  void unregister_mouse_target(const t3_window::window_t *target);

  /** Process a mouse event.
      @return A boolean indicating whether this mouse_target_t handled the
          mouse event.
  */
  virtual bool process_mouse_event(mouse_event_t event) = 0;
  ~mouse_target_t() override;

  /** Grab all future mouse events.

      Ensure that all mouse events are reported to the current ::mouse_target_t,
      and not to the ::mouse_target_t that should normally receive them. The
      grab should released using #release_mouse_grab.
  */
  void grab_mouse();
  /** Release a previous mouse grab. */
  void release_mouse_grab();

  static bool handle_mouse_event(mouse_event_t event);
};

/** Base class for widgets that need handle user text and draw differently based on the
    t3_win_can_draw function.

    The terminal capability detection code in libt3window may indicate that the terminal is or is
    not capable of drawing some characters that were believed to be (un)drawable based on the
    setting of @c LANG alone. This means that any attributes based on the t3_win_can_draw function
    need to be recalculated. Any widgets implementing this interface will be automatically signalled
    when the terminal capability detection is complete. */
class T3_WIDGET_API bad_draw_recheck_t {
 private:
  /** List of widgets to signal on completion of the terminal capability detection. */
  static std::list<bad_draw_recheck_t *> to_signal;
  /** connection_t used to initialize the connection_t to the @c terminal_settings_changed
      signal. */
  static connection_t initialized;
  /** Callback function called on the @c terminal_settings_changed signal. */
  static void bad_draw_recheck_all();

 public:
  /** Base constructor.
      Automatically adds this object to the list of objects to signal.
  */
  bad_draw_recheck_t();
  /** Base destructor.
      Detaches this object from the list of objects to signal. */
  virtual ~bad_draw_recheck_t();

  /** Function called on reception of the @c terminal_settings_changed signal. */
  virtual void bad_draw_recheck() = 0;
};

/* Class which allows allocating all implementation objects in a single malloc call. Essentially,
   it is a memory pool which is allocated with just enough space to hold all the implementation
   objects. */
class T3_WIDGET_API impl_allocator_t {
 public:
  impl_allocator_t(size_t size) {
    if (size > 0) {
      space_.reset(reinterpret_cast<char *>(std::malloc(size)));
      uint16_t remaining_space = size;
      std::memcpy(space_.get(), &remaining_space, sizeof(remaining_space));
    }
  }

  /** Returns a new T object allocated in the pool, with the arguments forwarded to the constructor.
      The returned pointer should have its destructor called, but should never be deleted. The
      destruction of the impl_allocator_t will free the memory. Hence, it is also important that the
      destructors of all allocated objects are called before this object is destroyed. Typically,
      this is used in combination with single_alloc_pimpl_t to hold the pointer. */
  template <typename T, typename... Args>
  T *new_impl(Args &&... args) {
    const size_t align = alignof(T);
    const size_t size = sizeof(T);
    if (size == 0) {
      return nullptr;
    }

    uint16_t remaining_space;
    std::memcpy(&remaining_space, space_.get(), sizeof(remaining_space));

    /* Using this computation can cause shifts of allocated memory compared to the intended
       locations, if the value can be put in multiple locations. For example, when allocating a
       4-byte int in a space that is 8 bytes large due to alignment requirements of the following
       value, the int will be allocated immediately before the 8 byte value, rather than keeping a 4
       byte padding. The padding will be inserted when allocating the next value. */
    remaining_space = ((remaining_space - size) / align) * align;
    if (remaining_space < 0) {
      return nullptr;
    }

    char *result = space_.get() + remaining_space;
    std::memcpy(space_.get(), &remaining_space, sizeof(remaining_space));
    new (result) T(std::forward<Args>(args)...);
    return reinterpret_cast<T *>(result);
  }

  /** Returns size of the malloc request needed to allocate T after an allocation of size @p req. */
  template <typename T>
  static size_t impl_alloc(size_t req) {
    return impl_alloc_internal<T>(req);
  }

 private:
  /** Returns size of the malloc request needed to allocate T after an allocation of size @p req.
      This is the actual implementation. This is separate from the public interface to allow
      explicit instantation to be used for some types like focus_widget_t::implementation_t. */
  template <typename T>
  static size_t impl_alloc_internal(size_t req) {
    const size_t align = alignof(T);
    const size_t size = sizeof(T);
    return req > 0 ? ((req - 1) / align + 1) * align + size : std::max(size, sizeof(uint16_t));
  }

  std::unique_ptr<char, free_deleter> space_;
};

}  // namespace t3widget
#endif
