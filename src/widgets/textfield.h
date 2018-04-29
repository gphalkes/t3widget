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
#ifndef T3_WIDGET_TEXTFIELD_H
#define T3_WIDGET_TEXTFIELD_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <t3widget/contentlist.h>
#include <t3widget/dialogs/popup.h>
#include <t3widget/key_binding.h>
#include <t3widget/string_view.h>
#include <t3widget/textline.h>
#include <t3widget/widgets/listpane.h>
#include <t3widget/widgets/smartlabel.h>
#include <t3widget/widgets/widget.h>

namespace t3_widget {

class T3_WIDGET_API text_field_t : public widget_t,
                                   public center_component_t,
                                   public focus_widget_t,
                                   public bad_draw_recheck_t {
 private:
  class T3_WIDGET_LOCAL drop_down_list_t;

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  /** Function to initialize the shared dialogs and data. */
  static void init(bool _init);

  /** Reset the selection. */
  void reset_selection();
  /** Change the current selection mode, based on @p key. */
  void set_selection(key_t key);
  /** Delete the current selection. */
  void delete_selection(bool save_to_copy_buffer);
  /** Make sure the text in the text_field_t is aligned such that the cursor is visible. */
  void ensure_cursor_on_screen();

  /** Set the end of the selection to the current position, updating the primary selection if so
   * requested. */
  void set_selection_end(bool update_primary = true);

 protected:
  bool has_focus() const;

 public:
  text_field_t();
  ~text_field_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t _focus) override;
  void show() override;
  void hide() override;
  /** Set the text of the text_field_t. */
  void set_text(string_view text);
  /** Set the autocompletion list. */
  void set_autocomplete(string_list_base_t *_completions);
  /** Set the list of keys to accept or reject.
      @param keys The list of keys to accept or reject.
      @param nr_of_keys The size of @p keys.
      @param accept Boolean indicating whether the keys in @p keys should be accepted or rejected.
  */
  void set_key_filter(key_t *keys, size_t nr_of_keys, bool accept);
  /** Retrieve the text shown by the text_field_t. */
  const std::string *get_text() const;
  /** Associate a label with this text_field_t.
      The reason to associate a smart_label_t with a text_field_t is that it
      allows the use of a hotkey to jump to the text_field_t. The #is_hotkey
      function will report the result of smart_label_t::is_hotkey.
  */
  void set_label(smart_label_t *_label);
  bool is_hotkey(key_t key) const override;

  void bad_draw_recheck() override;
  bool process_mouse_event(mouse_event_t event) override;

  T3_WIDGET_DECLARE_SIGNAL(activate);

#define _T3_ACTION_FILE <t3widget/widgets/textfield.actions.h>
#include <t3widget/key_binding_decl.h>
#undef _T3_ACTION_FILE
};

/** Drop-down list implementation for text_field_t. */
class T3_WIDGET_LOCAL text_field_t::drop_down_list_t : public popup_t {
 private:
  text_field_t *field; /**< text_field_t this drop-down list is created for. */

  std::unique_ptr<filtered_string_list_base_t> completions; /**< List of possible selections. */
  list_pane_t *list_pane;

  void update_list_pane();
  void item_activated();
  void selection_changed();

 public:
  drop_down_list_t(text_field_t *_field);
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  void show() override;
  void hide() override;
  bool process_mouse_event(mouse_event_t key) override;

  /** Request that the drop-down is filtered based on the contents of the text_field_t it is
   * asscociated with. */
  void update_view();
  /** Set the list of autocompletion options. */
  void set_autocomplete(string_list_base_t *completions);
  /** Return whether the autocompletion list is empty. */
  bool empty();
};

}  // namespace
#endif
