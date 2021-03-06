/* Copyright (C) 2011-2013,2018 G.P. Halkes
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

#ifndef T3_DIALOG_BASE_H
#define T3_DIALOG_BASE_H

#include <list>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/widget.h>

namespace t3widget {

class dialog_base_t;
using dialog_base_list_t = std::list<dialog_base_t *>;
class dialog_t;

class T3_WIDGET_API dialog_base_t : public virtual window_component_t,
                                    public container_t,
                                    public impl_allocator_t {
 private:
  friend class dialog_t;

  static dialog_base_list_t dialog_base_list; /**< List of all dialogs in the application. */

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  /** Default constructor, made private to avoid use. */
  dialog_base_t(size_t impl_size = 0);

  t3window::window_t &shadow_window();

  void push_back(widget_t *widget);

 protected:
  /** Create a new dialog with @p height and @p width, and with title @p _title. */
  dialog_base_t(int height, int width, bool has_shadow, size_t impl_size = 0);
  /** Focus the previous widget, wrapping around if necessary. */
  void focus_next();
  /** Focus the next widget, wrapping around if necessary. */
  void focus_previous();
  /** Add a widget to this dialog.
      If a widget is not added through #push_back or #insert, it will not be displayed, or receive
      input. */
  void push_back(std::unique_ptr<widget_t> widget);

  /** Add a widget to this dialog, before the given widget.
      If a widget is not added through #push_back or #insert, it will not be displayed, or receive
      input. */
  void insert(const widget_t *before, std::unique_ptr<widget_t> widget);

  template <typename T, typename... Args>
  T *emplace_back(Args &&... args) {
    T *result = new T(std::forward<Args>(args)...);
    push_back(std::unique_ptr<widget_t>(result));
    return result;
  }

  template <typename T, typename... Args>
  T *emplace(const widget_t *before, Args &&... args) {
    T *result = new T(std::forward<Args>(args)...);
    insert(before, std::unique_ptr<widget_t>(result));
    return result;
  }

  std::unique_ptr<widget_t> erase(size_t idx);

  bool is_child(const window_component_t *widget) const override;
  void set_child_focus(window_component_t *target) override;
  /** Request that the dialog itself (not its children) is redrawn. */
  void set_redraw(bool _redraw);
  /** Get the current request state for redrawing the dialog itself. */
  bool get_redraw() const;
  void set_depth(int depth);
  widget_t *get_current_widget();
  void focus_widget(size_t idx);
  bool focus_hotkey_widget(key_t key);
  const widgets_t &widgets();

 public:
  /** Destroy this dialog.
      Any widgets on the dialog are deleted as well.
  */
  ~dialog_base_t() override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  void show() override;
  void hide() override;
  /** Force a redraw of the dialog and its children. */
  void force_redraw() override;
  /** Set the position and anchoring for this dialog such that it is centered over a
      window_component_t. */
  virtual void center_over(const window_component_t *center);

  /** Call #force_redraw on all dialogs. */
  static void force_redraw_all();
};

}  // namespace t3widget

#endif  // T3_DIALOG_BASE_H
