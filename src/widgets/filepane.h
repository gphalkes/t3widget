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
#ifndef T3_WIDGET_FILEPANE_H
#define T3_WIDGET_FILEPANE_H

#include <string>

#include <t3widget/contentlist.h>
#include <t3widget/dialogs/popup.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/widgets/textfield.h>
#include <t3widget/widgets/widget.h>

#define _T3_WDIGET_FP_MAX_COLUMNS 8

namespace t3_widget {

/** A widget displaying the contents of a directory. */
class T3_WIDGET_API file_pane_t : public widget_t, public container_t {
 private:
  class search_panel_t;

  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  /** Ensure that the updated value of #current does not put the highlighted entry outside the
   * visible range. */
  void ensure_cursor_on_screen();
  /** Draw a single item. */
  void draw_line(int idx, bool selected);
  /** Update the width of a single column, based on the items to draw in it. */
  void update_column_width(int column, int start);
  /** Update the widths of all columns. */
  void update_column_widths();
  /** Handle a change of contents of #file_list. */
  void content_changed();

  void scrollbar_clicked(scrollbar_t::step_t step);
  void scrollbar_dragged(int start);

  void search(const std::string *text);

 public:
  file_pane_t();
  ~file_pane_t() override;
  /** Associate a text_field_t with this file_pane_t.
      The text_field_t will be updated when the selection in this file_pane_t
      changes.
  */
  void set_text_field(text_field_t *_field);
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t _focus) override;

  void set_child_focus(window_component_t *target) override;
  bool is_child(window_component_t *component) override;

  bool process_mouse_event(mouse_event_t event) override;

  /** Set the list to its initial position, i.e. the selected item is the first item. */
  void reset();
  /** Set the file_list_base_t that this file_pane_t displays. */
  void set_file_list(file_list_base_t *_file_list);
  /** Set the current selected item to the named item. */
  void set_file(const std::string *name);

  connection_t connect_activate(std::function<void(const std::string &)> cb);
};

class T3_WIDGET_LOCAL file_pane_t::search_panel_t : public popup_t {
 private:
  file_pane_t *parent;
  bool redraw;
  text_line_t text;

 public:
  search_panel_t(file_pane_t *_parent);
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void show() override;
  bool process_mouse_event(mouse_event_t event) override;
};

}  // namespace

#endif
