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
#ifndef T3_WIDGET_CHECKBOX_H
#define T3_WIDGET_CHECKBOX_H

#include <t3widget/widgets/smartlabel.h>
#include <t3widget/widgets/widget.h>

#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/mouse.h>
#include <t3widget/signals.h>
#include <t3widget/util.h>
#include <t3widget/widget_api.h>

namespace t3widget {

/** Class implementing a checkbox. */
class T3_WIDGET_API checkbox_t : public widget_t, public focus_widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;

  single_alloc_pimpl_t<implementation_t> impl;

  void next_state();

 public:
  /** Enum indicating the different states the checkbox can be in. */
  enum TriState { UNCHECKED, CHECKED, INDERMINATE };
  /** Create a new checkbox_t.
      @param _state The initial state of the checkbox_t.
  */
  checkbox_t(bool state = false);
  checkbox_t(TriState tristate);
  ~checkbox_t() override;
  bool process_key(key_t key) override;
  /** Set the size of this checkbox_t (ignored).
      A checkbox_t has a fixed size, so both @p height and @p width are ignored. */
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool is_hotkey(key_t key) const override;
  bool process_mouse_event(mouse_event_t event) override;
  /** Set the enabled status of this widget.
      When the checkbox_t is not enabled, it does not accept focus and the
      contents will be shown as a dash (-).
  */
  void set_enabled(bool enable) override;
  /** Retrieve the current state of the checkbox_t.

      If the checkbox_t is in tri-state mode and the value is INDETERMINATE, false is returned. */
  bool get_state();
  /** Set the current state of the checkbox_t. */
  void set_state(bool _state);
  /** Associate this checkbox_t with a smart_label_t. */
  void set_label(smart_label_t *_label);

  /** Sets whether the checkbox_t should be in tri-state mode.

      When switching the checkbox_t to non-tri-state mode and the value is currently INDETERMINATE,
      the value will be set to UNCHECKED.
  */
  void set_tristate_mode(bool is_tristate);
  /** Retrieve the current state of the checkbox_t. */
  TriState get_tristate() const;
  /** Set the current state of the checkbox_t. */
  void set_tristate(TriState state);

  /** @fn connection_t connect_activate(std::function<void()> func)
      Connect a callback to the #activate signal.
  */
  /** Signal emitted when the button is pressed. */
  T3_WIDGET_DECLARE_SIGNAL(activate);
  /** @fn connection_t connect_toggled(std::function<void()> func)
      Connect a callback to the #toggled signal.
  */
  /** Signal emitted when the state of the checkbox_t is toggled. */
  T3_WIDGET_DECLARE_SIGNAL(toggled);
};

}  // namespace t3widget
#endif
