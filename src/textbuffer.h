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
#ifndef T3_WIDGET_TEXTBUFFER_H
#define T3_WIDGET_TEXTBUFFER_H

#include <cstdio>
#include <string>
#include <vector>

#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/textline.h>
#include <t3widget/undo.h>

namespace t3_widget {

typedef std::vector<text_line_t *> lines_t;

struct find_result_t;
class finder_t;
class wrap_info_t;

class T3_WIDGET_API text_buffer_t {
  friend class wrap_info_t;

 private:
  struct T3_WIDGET_LOCAL implementation_t {
    lines_t lines;
    text_coordinate_t selection_start;
    text_coordinate_t selection_end;
    selection_mode_t selection_mode;

    undo_list_t undo_list;
    text_coordinate_t last_undo_position;
    undo_type_t last_undo_type;
    undo_t *last_undo;

    text_line_factory_t *line_factory;

    implementation_t(text_line_factory_t *_line_factory)
        : selection_start(-1, 0),
          selection_end(-1, 0),
          selection_mode(selection_mode_t::NONE),
          last_undo_type(UNDO_NONE),
          last_undo(nullptr),
          line_factory(_line_factory == nullptr ? &default_text_line_factory : _line_factory) {}
  };
  std::unique_ptr<implementation_t> impl;

 protected:
  undo_t *get_undo(undo_type_t type);
  undo_t *get_undo(undo_type_t type, text_coordinate_t coord);
  undo_t *get_undo(undo_type_t type, text_coordinate_t start, text_coordinate_t end);

  void set_undo_mark();

  void delete_block_internal(text_coordinate_t start, text_coordinate_t end, undo_t *undo);
  bool insert_block_internal(text_coordinate_t insert_at, text_line_t *block);
  int apply_undo_redo(undo_type_t type, undo_t *current);
  bool merge_internal(int line);
  bool break_line_internal(const std::string *indent = nullptr);

  bool undo_indent_selection(undo_t *undo, undo_type_t type);

  text_line_t *get_line_data_nonconst(int idx);
  text_line_factory_t *get_line_factory();
  text_line_t *get_mutable_line_data(int idx);

  virtual void prepare_paint_line(int line);

 public:
  text_buffer_t(text_line_factory_t *_line_factory = nullptr);
  virtual ~text_buffer_t();

  int size() const;
  const text_line_t *get_line_data(int idx) const;

  bool insert_char(key_t c);
  bool overwrite_char(key_t c);
  bool delete_char();
  bool backspace_char();
  bool merge(bool backspace);
  bool break_line(const std::string *indent = nullptr);
  bool insert_block(const std::string *block);

  bool append_text(const char *text);
  bool append_text(const char *text, size_t _size);
  bool append_text(const std::string *text);

  int get_line_max(int line) const;
  void adjust_position(int adjust);
  int width_at_cursor() const;

  void paint_line(window_wrapper_t *win, int line, const text_line_t::paint_info_t *info);
  void goto_next_word();
  void goto_previous_word();

  int calculate_screen_pos(const text_coordinate_t *where, int tabsize) const;
  int calculate_line_pos(int line, int pos, int tabsize) const;

  text_coordinate_t get_selection_start() const;
  text_coordinate_t get_selection_end() const;
  void set_selection_end(bool update_primary = true);
  void set_selection_mode(selection_mode_t mode);
  selection_mode_t get_selection_mode() const;
  bool selection_empty() const;
  void delete_block(text_coordinate_t start, text_coordinate_t end);
  bool replace_block(text_coordinate_t start, text_coordinate_t end, const std::string *block);
  bool indent_selection(int tabsize, bool tab_spaces);
  bool indent_block(text_coordinate_t &start, text_coordinate_t &end, int tabsize, bool tab_spaces);
  bool unindent_selection(int tabsize);
  bool unindent_block(text_coordinate_t &start, text_coordinate_t &end, int tabsize);
  bool unindent_line(int tabsize);
  void set_selection_from_find(find_result_t *result);

  /** Find a substring in the text.
      @param finder The ::finder_t used to locate the substring.
      @param result The previous find result and the location in which the result will be returned.
      @param reverse Reverse the direction of the find action.

      The @p result parameter is both an input and an output parameter. On input
      it should contain in its @c start member the location of the start of
      the previously found string, or @c cursor if no previous string was
      (yet) found.
  */
  bool find(finder_t *finder, find_result_t *result, bool reverse = false) const;
  bool find_limited(finder_t *finder, text_coordinate_t start, text_coordinate_t end,
                    find_result_t *result) const;
  void replace(finder_t *finder, find_result_t *result);

  bool is_modified() const;
  std::string *convert_block(text_coordinate_t start, text_coordinate_t end);
  int apply_undo();
  int apply_redo();
  void start_undo_block();
  void end_undo_block();

  void goto_next_word_boundary();
  void goto_previous_word_boundary();

  /** Sets the position in the text file.

      Values smaller than 1 are ignored, while values that are out of range are reduced
      to the maximum line and position respectively.

      Note that this does not update the @c edit_window_t viewport, so that has to
      be triggered separately.
  */
  void goto_pos(int line, int pos);

  // FIXME: make these members private again
  text_coordinate_t cursor;

  T3_WIDGET_SIGNAL(rewrap_required, void, rewrap_type_t, int, int);
};

}  // namespace
#endif
