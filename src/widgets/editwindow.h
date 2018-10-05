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
#ifndef T3_WIDGET_EDITWINDOW_H
#define T3_WIDGET_EDITWINDOW_H

namespace t3widget {
class edit_window_t;
}  // namespace t3widget

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <t3widget/autocompleter.h>
#include <t3widget/dialogs/menupanel.h>
#include <t3widget/dialogs/popup.h>
#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/key_binding.h>
#include <t3widget/textbuffer.h>
#include <t3widget/util.h>
#include <t3widget/widgets/listpane.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/widgets/widget.h>

namespace t3widget {

class finder_t;
class wrap_info_t;
class goto_dialog_t;
class find_dialog_t;
class replace_buttons_dialog_t;

/** Class implementing an edit widget. */
class T3_WIDGET_API edit_window_t : public widget_t,
                                    public center_component_t,
                                    public container_t,
                                    public bad_draw_recheck_t {
 private:
  class T3_WIDGET_LOCAL autocomplete_panel_t;

  static goto_dialog_t *goto_dialog;
  static connection_t goto_connection;
  static find_dialog_t *global_find_dialog;
  static connection_t global_find_dialog_connection;
  static std::shared_ptr<finder_t> global_finder;
  static replace_buttons_dialog_t *replace_buttons;
  static connection_t replace_buttons_connection;
  static menu_panel_t *right_click_menu;
  static connection_t right_click_menu_connection;
  static connection_t init_connected;

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  /** Function to initialize the shared dialogs and data. */
  static void init(bool _init);
  /** Strings displayed in the bottom line when inserting/overwriting. */
  static const char *ins_string[];
  /** Function pointer for calling insert/replace depending on insert/overwrite status. */
  static bool (text_buffer_t::*proces_char[])(key_t);

  /** Redraw the contents of the edit_window_t. */
  void repaint_screen();
  /** Handle cursor right key. */
  void inc_x();
  /** Handle control-cursor right key. */
  void next_word();
  /** Handle cursor left key. */
  void dec_x();
  /** Handle control-cursor left key. */
  void previous_word();
  /** Handle cursor down key. */
  void inc_y();
  /** Handle cursor up key. */
  void dec_y();
  /** Handle page-down key. */
  void pgdn();
  /** Handle page-up key. */
  void pgup();
  /** Handle home key. */
  void home_key();
  /** Handle end key. */
  void end_key();
  /** Reset the selection. */
  void reset_selection();
  /** Set the selection mode based on the current key pressed by the user. */
  bool set_selection_mode(key_t key);
  /** Delete the selection. */
  void delete_selection();

  /** The find or replace action has been activated in the find or replace buttons dialog. */
  void find_activated(std::shared_ptr<finder_t> finder, find_action_t action);
  /** Handle setting of the wrap mode. */
  void set_wrap_internal(wrap_type_t wrap);

  void scroll(text_pos_t lines);
  void scrollbar_clicked(scrollbar_t::step_t step);
  void scrollbar_dragged(text_pos_t start);
  void autocomplete_activated();
  void mark_selection();
  /** Pastes either the selection, or the clipboard. */
  void paste(bool clipboard);

  void right_click_menu_activated(int action);

 protected:
  text_buffer_t *text;            /**< Buffer holding the text currently displayed. */
  t3window::window_t info_window; /**< Window for other information, such as buffer name. */

  /** Draw the information in the #info_window.

      This function is called whenever the #info_window changes size and
      therefore needs to be redrawn.
  */
  virtual void draw_info_window();

  /** Activate the autocomplete panel.
      @param autocomplete_single Should the autocomplete be automatic if
          there is only one option.

      The autocomplete panel activate, only if there is a possible autocompletion. */
  void activate_autocomplete(bool autocomplete_single);

  /** Convert coordinates relative to the edit window to a text_coordinate_t. */
  text_coordinate_t xy_to_text_coordinate(int x, int y);
  /** Ensure that the cursor is visible. */
  void ensure_cursor_on_screen();
  /** Change the lines to start and end repainting.

      It is acceptable to pass @p start and @p end in reverse order. */
  void update_repaint_lines(text_pos_t start, text_pos_t end);

  /** Change the lines to start and end repainting.

      Calls the two parameter version of this function with @p line repeated. */
  void update_repaint_lines(text_pos_t line);

 public:
  class T3_WIDGET_API view_parameters_t;

  /** Create a new edit_window_t.
      @param _text The text to display in the edit_window_t.
      @param params The view parameters to set.
  */
  edit_window_t(text_buffer_t *_text = nullptr, const view_parameters_t *params = nullptr);
  ~edit_window_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  void force_redraw() override;
  void bad_draw_recheck() override;

  void set_child_focus(window_component_t *target) override;
  bool is_child(const window_component_t *widget) const override;
  bool process_mouse_event(mouse_event_t event) override;

  /** Set the text to display.
      The previously displayed text will be replaced, without deleting. Use
      get_text to retrieve the current text_buffer_t before calling this
      function to make sure you have a reference to the current buffer that
      you can use to delete the old text_buffer_t.
  */
  void set_text(text_buffer_t *_text, const view_parameters_t *params = nullptr);
  /** Get the text currently displayed. */
  text_buffer_t *get_text() const;
  /** Apply the undo action. */
  void undo();
  /** Apply the redo action. */
  void redo();
  /** Apply the cut or copy action.
      @param cut A boolean indicating whether a cut (@c true) should be performed, or a copy (@c
     false).
      The selected text is stored in copy_buffer.
  */
  void cut_copy(bool cut);
  /** Apply the paste action.
      The text to paste is taken from copy_buffer.
  */
  void paste();
  /** Apply the paste-selection action.
      The text to paste is taken from the selection.
  */
  void paste_selection();
  /** Select the whole text. */
  void select_all();
  /** Show the Insert Special Character dialog. */
  void insert_special();
  /** Indent the current selection. */
  void indent_selection();
  /** Unindent the current selection. */
  void unindent_selection();
  /** Move the cursor to the specified line. */
  void goto_line(text_pos_t line);

  /** Show the Goto Line dialog. */
  void goto_line();
  /** Show the find or replace dialog. */
  void find_replace(bool replace);
  /** Apply the find-next action.
      @param backward Boolean indicating whether to reverse the direction of the search relative to
     the original search direction.
  */
  void find_next(bool backward);
  /** Set the find_dialog_t to use.
      In some cases it can be useful @e not to use the global find_dialog_t.
      This function allows one to set the find_dialog_t used.
  */
  void set_find_dialog(find_dialog_t *_find_dialog);
  /** Set whether to use a local @c finder_t instance, or to use the shared global @c finder_t.
      Because the finder_t stores the search information, such as the text searched
      for, it is sometimes useful to use different finder_t instances for different windows.
      The finder_t is used for example for the find-next action.
  */
  void set_use_local_finder(bool _use_local_finder);

  /** Set the size of a tab. */
  void set_tabsize(int _tabsize);
  /** Set the wrap type. */
  void set_wrap(wrap_type_t wrap);
  /** Set tab indents with spaces. */
  void set_tab_spaces(bool _tab_spaces);
  /** Set automatic indent. */
  void set_auto_indent(bool _auto_indent);
  /** Set indent aware home. */
  void set_indent_aware_home(bool _indent_aware_home);
  /** Set show_tabs. */
  void set_show_tabs(bool _show_tabs);

  /** Get the size of a tab. */
  int get_tabsize();
  /** Get the wrap type. */
  wrap_type_t get_wrap();
  /** Get tab indents with spaces. */
  bool get_tab_spaces();
  /** Get automatic indent. */
  bool get_auto_indent();
  /** Get indent aware home. */
  bool get_indent_aware_home();
  /** Get show tabs. */
  bool get_show_tabs();

  /** Save the current view parameters, to allow them to be restored later. */
  view_parameters_t *save_view_parameters();
  /** Save the current view parameters, to allow them to be restored later. */
  void save_view_parameters(view_parameters_t *params);

  /** Set the autocompleter to use. */
  void set_autocompleter(autocompleter_t *_autocompleter);
  /** Perform autocompletion, or pop-up autocompletion choice menu. */
  void autocomplete();

  /** Delete the current line, or lines if the current selection spans multiple lines. */
  void delete_line();

#define _T3_ACTION_FILE <t3widget/widgets/editwindow.actions.h>
#include <t3widget/key_binding_decl.h>
#undef _T3_ACTION_FILE
};

class T3_WIDGET_API edit_window_t::view_parameters_t {
  friend class edit_window_t;

 private:
  text_coordinate_t top_left;
  wrap_type_t wrap_type;
  int tabsize;
  bool tab_spaces;
  int ins_mode;
  text_pos_t last_set_pos;
  bool auto_indent;
  bool indent_aware_home;
  bool show_tabs;

  view_parameters_t(edit_window_t *view);
  void apply_parameters(edit_window_t *view) const;

 public:
  view_parameters_t();
  void set_tabsize(int _tabsize);
  void set_wrap(wrap_type_t _wrap_type);
  void set_tab_spaces(bool _tab_spaces);
  void set_auto_indent(bool _auto_indent);
  void set_indent_aware_home(bool _indent_aware_home);
  void set_show_tabs(bool _show_tabs);
};

class T3_WIDGET_LOCAL edit_window_t::autocomplete_panel_t : public popup_t {
 private:
  list_pane_t *list_pane;

 public:
  autocomplete_panel_t(edit_window_t *parent);
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;

  void set_completions(string_list_base_t *completions);
  size_t get_selected_idx() const;
  void connect_activate(std::function<void()> func);
};

}  // namespace t3widget
#endif
