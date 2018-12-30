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
#ifndef T3_WIDGET_TEXTLINE_H
#define T3_WIDGET_TEXTLINE_H

#define BUFFERSIZE 64
#define BUFFERINC 16

#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <t3widget/interfaces.h>
#include <t3widget/key.h>
#include <t3widget/string_view.h>
#include <t3widget/widget_api.h>
#include <t3window/window.h>

namespace t3widget {

class undo_t;

#define _T3_MAX_TAB 80

class text_line_factory_t;

class T3_WIDGET_API text_line_t {
 public:
  enum {
    BREAK = (1 << 0),
    PARTIAL_CHAR = (1 << 1),
    SPACECLEAR = (1 << 2),
    TAB_AS_CONTROL = (1 << 3),
    EXTEND_SELECTION = (1 << 4),
    DEBUG_LINE = (1 << 5),
    SHOW_TABS = (1 << 6)
  };

  struct T3_WIDGET_API paint_info_t {
    // Byte position of the start of the line (0 unless line wrapping is in effect)
    text_pos_t start;
    text_pos_t leftcol;  // First cell to draw

    // Last position of the line (std::numeric_limits<text_pos_t>::max()) unless line wrapping is in
    // effect)
    text_pos_t max;
    text_pos_t size;  // Maximum number of characters to draw

    int tabsize;                           // Length of a tab, or 0 to force TAB_AS_CONTROL flag
    int flags;                             // See above for valid flags
    text_pos_t selection_start;            // Start of selected text in bytes
    text_pos_t selection_end;              // End of selected text in bytes
    text_pos_t cursor;                     // Location of cursor in bytes
    t3_attr_t normal_attr, selected_attr;  // Attributes to be used for normal an selected texts
                                           // string highlighting;
  };

  struct T3_WIDGET_API break_pos_t {
    text_pos_t pos;
    int flags;
  };

 private:
  static char spaces[_T3_MAX_TAB];
  static char dashes[_T3_MAX_TAB];
  static char dots[16];
  static const char *control_map;
  static const char *wrap_symbol;

  struct implementation_t;
  pimpl_t<implementation_t> impl;

  static void paint_part(t3window::window_t *win, const char *paint_buffer, text_pos_t todo,
                         bool is_print, t3_attr_t selection_attr);
  static int key_width(key_t key);
  static text_pos_t adjust_position(string_view str, text_pos_t pos, int adjust);
  static int byte_width_from_first(string_view str, text_pos_t pos);
  static int width_at(string_view str, text_pos_t pos);

  t3_attr_t get_draw_attrs(text_pos_t i, const paint_info_t &info) const;

  void fill_line(string_view _buffer);
  bool check_boundaries(text_pos_t match_start, text_pos_t match_end) const;

  void reserve(text_pos_t size);
  int byte_width_from_first(text_pos_t pos) const;

  friend class regex_finder_t;

 protected:
  text_line_factory_t *get_line_factory() const;
  virtual t3_attr_t get_base_attr(text_pos_t i, const paint_info_t &info) const;

 public:
  text_line_t(int buffersize = BUFFERSIZE, text_line_factory_t *factory = nullptr);
  text_line_t(string_view buffer, text_line_factory_t *factory = nullptr);
  virtual ~text_line_t();

  void set_text(string_view _buffer);

  void merge(std::unique_ptr<text_line_t> other);
  std::unique_ptr<text_line_t> break_line(text_pos_t pos);
  std::unique_ptr<text_line_t> cut_line(text_pos_t start, text_pos_t end);
  std::unique_ptr<text_line_t> clone(text_pos_t start, text_pos_t end);
  std::unique_ptr<text_line_t> break_on_nl(text_pos_t *start_from);
  void insert(std::unique_ptr<text_line_t> other, text_pos_t pos);

  void minimize();

  text_pos_t calculate_screen_width(text_pos_t start, text_pos_t pos, int tabsize) const;
  text_pos_t calculate_line_pos(text_pos_t start, text_pos_t max, text_pos_t pos,
                                int tabsize) const;

  void paint_line(t3window::window_t *win, const paint_info_t &info) const;

  break_pos_t find_next_break_pos(text_pos_t start, text_pos_t length, int tabsize) const;
  text_pos_t get_next_word(text_pos_t start) const;
  text_pos_t get_previous_word(text_pos_t start) const;

  bool insert_char(text_pos_t pos, key_t c, undo_t *undo);
  bool overwrite_char(text_pos_t pos, key_t c, undo_t *undo);
  bool delete_char(text_pos_t pos, undo_t *undo);
  bool append_char(key_t c, undo_t *undo);
  bool backspace_char(text_pos_t pos, undo_t *undo);

  text_pos_t adjust_position(text_pos_t pos, int adjust) const;

  text_pos_t size() const;
  int width_at(text_pos_t pos) const;
  bool is_print(text_pos_t pos) const;
  bool is_space(text_pos_t pos) const;
  bool is_alnum(text_pos_t pos) const;
  bool is_bad_draw(text_pos_t pos) const;

  const std::string &get_data() const;

  text_pos_t get_next_word_boundary(text_pos_t start) const;
  text_pos_t get_previous_word_boundary(text_pos_t start) const;

  static void init();
};

class T3_WIDGET_API text_line_factory_t {
 public:
  text_line_factory_t();
  virtual ~text_line_factory_t();
  virtual std::unique_ptr<text_line_t> new_text_line_t(int buffersize = BUFFERSIZE);
  virtual std::unique_ptr<text_line_t> new_text_line_t(string_view _buffer);
};

T3_WIDGET_API extern text_line_factory_t default_text_line_factory;

}  // namespace t3widget
#endif
