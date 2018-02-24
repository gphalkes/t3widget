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

namespace t3_widget {
class edit_window_t;
};  // namespace

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

namespace t3_widget {

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
  static signals::connection goto_connection;
  static find_dialog_t *global_find_dialog;
  static signals::connection global_find_dialog_connection;
  static finder_t global_finder;
  static replace_buttons_dialog_t *replace_buttons;
  static signals::connection replace_buttons_connection;
  static menu_panel_t *right_click_menu;
  static signals::connection right_click_menu_connection;
  static signals::connection init_connected;

  struct T3_WIDGET_LOCAL implementation_t {
    unique_t3_window_ptr edit_window, /**< Window containing the text. */
        indicator_window; /**< Window holding the line, column, modified, etc. information line at
                             the bottom. */
    std::unique_ptr<scrollbar_t> scrollbar; /**< Scrollbar on the right of the text. */
    int screen_pos;                         /**< Cached position of cursor in screen coordinates. */
    int tabsize;                            /**< Width of a tab, in cells. */
    bool focus;      /**< Boolean indicating whether this edit_window_t has the input focus. */
    bool tab_spaces; /**< Boolean indicating whether to use spaces for tab. */
    /** Associated find dialog.
            By default this is the shared dialog, but can be set to a different one.
    */
    find_dialog_t *find_dialog;
    finder_t *finder;      /**< Object used for find actions in the text. */
    wrap_type_t wrap_type; /**< The wrap_type_t used for display. */
    wrap_info_t
        *wrap_info; /**< Required information for wrapped display, or @c nullptr if not in use. */
    /** The top-left coordinate in the text.
            This is either a proper text_coordinate_t when wrapping is disabled, or
            a line and sub-line (pos @c member) coordinate when wrapping is enabled.
    */
    text_coordinate_t top_left;
    int ins_mode,     /**< Current insert/overwrite mode. */
        last_set_pos; /**< Last horiziontal position set by user action. */
    bool auto_indent; /**< Boolean indicating whether automatic indentation should be enabled. */
    /** Boolean indicating whether the current text is part of a paste operation.
        Set automatically as a response to bracketed paste. Disables auto-indent. */
    bool pasting_text;
    bool indent_aware_home; /**< Boolean indicating whether home key should handle indentation
                               specially. */
    bool show_tabs;         /**< Boolean indicating whether to explicitly show tabs. */

    std::unique_ptr<autocompleter_t> autocompleter; /**< Object used for autocompletion. */
    std::unique_ptr<autocomplete_panel_t>
        autocomplete_panel; /**< Panel for showing autocomplete options. */

    int repaint_min, /**< First line to repaint. */
        repaint_max; /**< Last line to repaint. */

    implementation_t()
        : screen_pos(0),
          tabsize(8),
          focus(false),
          tab_spaces(false),
          find_dialog(nullptr),
          finder(nullptr),
          wrap_type(wrap_type_t::NONE),
          wrap_info(nullptr),
          ins_mode(0),
          last_set_pos(0),
          auto_indent(true),
          pasting_text(false),
          indent_aware_home(true),
          show_tabs(false),
          repaint_min(0),
          repaint_max(INT_MAX) {}
  };
  pimpl_ptr<implementation_t>::t impl;

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
  void find_activated(find_action_t action, finder_t *finder);
  /** Handle setting of the wrap mode. */
  void set_wrap_internal(wrap_type_t wrap);

  void scroll(int lines);
  void scrollbar_clicked(scrollbar_t::step_t step);
  void scrollbar_dragged(int start);
  void autocomplete_activated();
  void mark_selection();
  /** Pastes either the selection, or the clipboard. */
  void paste(bool clipboard);

  void right_click_menu_activated(int action);

 protected:
  text_buffer_t *text;               /**< Buffer holding the text currently displayed. */
  cleanup_t3_window_ptr info_window; /**< Window for other information, such as buffer name. */

  /** Draw the information in the #info_window.

      This function is called whenever the #info_window changes size and
      therefore needs to be redrawn.
  */
  virtual void draw_info_window();

  /** Activate the autocomplete panel.
      @param autocomplete_single Should the autocomplete be automatic if
          there is only one option.

      The autocomplete panel activate, only if there is a possible autocompletion.
  */
  void activate_autocomplete(bool autocomplete_single);

  /** Convert coordinates relative to the edit window to a text_coordinate_t. */
  text_coordinate_t xy_to_text_coordinate(int x, int y);
  /** Ensure that the cursor is visible. */
  void ensure_cursor_on_screen();
  /** Change the lines to start and end repainting.

      It is acceptable to pass @p start and @p end in reverse order.
  */
  void update_repaint_lines(int start, int end);

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
  bool is_child(window_component_t *widget) override;
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
  void goto_line(int line);

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
  /** Set the finder_t to use.
      Because the finder_t stores the search information, such as the text searched
      for, it is sometimes useful to use different finder_t instances for different windows.
      The finder_t is used for example for the find-next action.
  */
  void set_finder(finder_t *_finder);

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
  int ins_mode, last_set_pos;
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
  void connect_activate(const signals::slot<void> &slot);
};

};  // namespace
#endif
