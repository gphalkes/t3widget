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
#ifndef T3_WIDGET_DIALOGS_H
#define T3_WIDGET_DIALOGS_H

#include <cstddef>
#include <list>
#include <string>
#include <t3widget/dialogs/dialogbase.h>
#include <t3widget/dialogs/popup.h>

#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/signals.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>

namespace t3widget {

class dialog_t;

typedef std::list<dialog_t *> dialogs_t;

/** Base class for dialogs. */
class T3_WIDGET_API dialog_t : public dialog_base_t {
 private:
  friend void iterate();
  friend bool mouse_target_t::handle_mouse_event(mouse_event_t event);
  // main_window_base_t should be allowed to call dialog_t(), but no others should
  friend class main_window_base_t;
  friend class popup_t;

  static dialogs_t active_dialogs; /**< Dialog stack. */
  static popup_t *active_popup;    /**< Currently active ::popup_t. */
  static int dialog_depth;         /**< Depth of the top most dialog in the window stack. */

  static void set_active_popup(popup_t *popup);
  static void update_dialogs();

  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

  void activate_dialog(); /**< Move this dialog up to the top of the dialog and window stack. Called
                             from #show. */
  void deactivate_dialog(); /**< Remove this dialog from the dialog stack. Called from #hide. */

  /** Default constructor, made private to avoid use. */
  dialog_t();

 protected:
  /** Create a new dialog with @p height and @p width, and with title @p _title. */
  dialog_t(int height, int width, optional<std::string> title, size_t impl_size = 0);
  /** Close the dialog.
      This function should be called when the dialog is closed by some event originating from this
      dialog. */
  virtual void close();

  bool is_child(const window_component_t *widget) const override;
  void set_child_focus(window_component_t *target) override;
  void set_title(std::string title);

 public:
  ~dialog_t() override;
  bool process_key(key_t key) override;
  void update_contents() override;
  void show() override;
  void hide() override;

  /** Connect a callback to the #closed signal. */
  T3_WIDGET_DECLARE_SIGNAL(closed);
};

}  // namespace t3widget
#endif
