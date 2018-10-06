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
#ifndef T3_WIDGET_WRAPINFO_H
#define T3_WIDGET_WRAPINFO_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

#include <vector>

#include <t3widget/textbuffer.h>
#include <t3widget/util.h>

namespace t3widget {

typedef std::vector<text_pos_t> wrap_points_t;
typedef std::vector<wrap_points_t *> wrap_data_t;

/** Class holding information about wrapping a text_buffer_t.

    This class is required by edit_window_t and text_buffer_t to present the
    user with a wrapped text. Except for in the function find_line, it uses the
    text_coordinate_t class in a special way: the @c pos field is used to store
    the index in the array of wrap points for the line indicated by the @c line
    field.
*/
class T3_WIDGET_LOCAL wrap_info_t {
 private:
  wrap_data_t wrap_data;
  text_buffer_t *text;
  int tabsize;
  int wrap_width;
  text_pos_t size;
  connection_t rewrap_connection;

  void delete_lines(text_pos_t first, text_pos_t last);
  void insert_lines(text_pos_t first, text_pos_t last);
  void rewrap_line(text_pos_t line, text_pos_t pos, bool force);
  void rewrap_all();
  void rewrap(rewrap_type_t type, text_pos_t a, text_pos_t b);

 public:
  wrap_info_t(int width, int tabsize = 8);
  ~wrap_info_t();
  text_pos_t unwrapped_size() const;
  text_pos_t wrapped_size() const;
  text_pos_t get_line_count(text_pos_t line) const;

  void set_wrap_width(int width);
  void set_tabsize(int _tabsize);
  void set_text_buffer(text_buffer_t *_text);

  bool add_lines(text_coordinate_t &coord, text_pos_t count) const;
  bool sub_lines(text_coordinate_t &coord, text_pos_t count) const;
  text_coordinate_t get_end() const;
  text_pos_t find_line(text_coordinate_t coord) const;
  text_pos_t calculate_screen_pos() const;
  text_pos_t calculate_screen_pos(const text_coordinate_t &where) const;
  text_pos_t calculate_line_pos(text_pos_t line, text_pos_t pos, text_pos_t subline) const;
  void paint_line(t3window::window_t *win, text_coordinate_t line,
                  text_line_t::paint_info_t &info) const;
};

}  // namespace t3widget
#endif
