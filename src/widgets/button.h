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
#ifndef T3_WIDGET_BUTTON_H
#define T3_WIDGET_BUTTON_H

#include <t3widget/widgets/smartlabel.h>
#include <t3widget/widgets/widget.h>

namespace t3_widget {

/** Button widget. */
class T3_WIDGET_API button_t : public widget_t, public focus_widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  pimpl_t<implementation_t> impl;

 public:
  /** Create a button_t.
      @param _text The text to show on the button.
      @param _is_default Boolean indicating whether this button should be drawn as the default
     button.

      The @p _text is used to initialize a smart_label_text_t, therefore the
      character following the underscore will be highlighted. */
  button_t(const char *_text, bool _is_default = false);
  ~button_t() override;
  bool process_key(key_t key) override;
  /** Set the size of this button_t.
      The @p height parameter is ignored. If width is negative, the natural size
      of the button_t based on the text width is used. */
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool is_hotkey(key_t key) const override;

  bool process_mouse_event(mouse_event_t event) override;
  /** Retrieve this button_t's width. */
  int get_width();

  /** @fn connection_t connect_activate(std::function<void()> func)
      Connect a callback to the #activate signal.
  */
  /** Signal emitted when the button is pressed. */
  T3_WIDGET_SIGNAL(activate);
};

}  // namespace
#endif
