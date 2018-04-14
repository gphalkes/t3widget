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
#include <t3widget/widgets/dummywidget.h>
#include <t3widget/widgets/widget.h>

namespace t3_widget {

class dialog_base_t;
typedef std::list<dialog_base_t *> dialog_base_list_t;
class dialog_t;

class T3_WIDGET_API dialog_base_t : public virtual window_component_t, public container_t {
 private:
  friend class dialog_t;

  static dialog_base_list_t dialog_base_list; /**< List of all dialogs in the application. */
  static dummy_widget_t
      *dummy; /**< Dummy widget to ensure that a dialog is never empty when shown. */

  static void init(bool _init); /**< Function to initialize the dummy widget. */
  /** Dummy value to allow static connection_t of the @c on_init signal to #init. */
  static connection_t init_connected;

  /** Default constructor, made private to avoid use. */
  dialog_base_t();

  bool redraw;                       /**< Boolean indicating whether redrawing is necessary. */
  t3_window::window_t shadow_window; /**< t3_window_t used to draw the shadow under a dialog. */

 protected:
  widgets_t::iterator
      current_widget; /**< Iterator indicating the widget that has the input focus. */
  /** List of widgets on this dialog. This list should only be filled using #push_back. */
  widgets_t widgets;

  /** Create a new dialog with @p height and @p width, and with title @p _title. */
  dialog_base_t(int height, int width, bool has_shadow);
  /** Focus the previous widget, wrapping around if necessary. */
  void focus_next();
  /** Focus the next widget, wrapping around if necessary. */
  void focus_previous();
  /** Add a widget to this dialog.
      If a widget is not added through #push_back, it will not be
      displayed, or receive input. */
  void push_back(widget_t *widget);

  bool is_child(window_component_t *widget) override;
  void set_child_focus(window_component_t *target) override;
  void set_redraw(bool _redraw);
  bool get_redraw() const;
  void set_depth(int depth);
  widget_t *get_current_widget();
  void focus_widget(size_t idx);
  bool focus_hotkey_widget(key_t key);

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
  void force_redraw() override;
  /** Set the position and anchoring for this dialog such that it is centered over a
   * window_component_t. */
  virtual void center_over(const window_component_t *center);

  /** Call #force_redraw on all dialogs. */
  static void force_redraw_all();
};

}  // namespace

#endif  // T3_DIALOG_BASE_H
