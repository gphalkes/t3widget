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

#include "wrapinfo.h"
#include "internal.h"
#include "log.h"

namespace t3_widget {

wrap_info_t::wrap_info_t(int width, int _tabsize)
    : text(nullptr), size(0), tabsize(_tabsize), wrap_width(width) {}

wrap_info_t::~wrap_info_t() {
  rewrap_connection.disconnect();
  for (wrap_points_t *iter : wrap_data) {
    delete iter;
  }
}

int wrap_info_t::get_size() const { return wrap_data.size(); }
int wrap_info_t::get_text_size() const { return size; }

void wrap_info_t::delete_lines(int first, int last) {
  for (wrap_data_t::iterator iter = wrap_data.begin() + first; iter != wrap_data.begin() + last;
       iter++) {
    size -= (*iter)->size();
    delete *iter;
  }
  wrap_data.erase(wrap_data.begin() + first, wrap_data.begin() + last);
}

void wrap_info_t::insert_lines(int first, int last) {
  int i;
  for (i = first; i < last; i++) {
    wrap_data.insert(wrap_data.begin() + i, new wrap_points_t());
    // Ensure that the list of break positions contains at least the start position.
    wrap_data[i]->push_back(0);
    size++;
    rewrap_line(i, 0, true);
  }
}

void wrap_info_t::rewrap_line(int line, int pos, bool local) {
  text_line_t::break_pos_t break_pos;
  size_t i;

  /* The list of break positions always contains the start position (0). */

  for (i = wrap_data[line]->size() - 1; i > 0 && (*wrap_data[line])[i] > pos; i--) {
  }

  if (local) {
    break_pos = text->impl->lines[line]->find_next_break_pos((*wrap_data[line])[i], wrap_width - 1,
                                                             tabsize);
    if (i < wrap_data[line]->size() - 1 && break_pos.pos == (*wrap_data[line])[i + 1]) {
      return;
    }
  }

  /* Keep it simple: subtract the full size here, and add the full size again
     when we are done rewrapping. */
  size -= wrap_data[line]->size();
  wrap_data[line]->erase(wrap_data[line]->begin() + i + 1, wrap_data[line]->end());

  while (true) {
    break_pos = text->impl->lines[line]->find_next_break_pos(wrap_data[line]->back(),
                                                             wrap_width - 1, tabsize);
    if (break_pos.pos > 0) {
      wrap_data[line]->push_back(break_pos.pos);
    } else {
      break;
    }
  }
  size += wrap_data[line]->size();
}

void wrap_info_t::rewrap_all() {
  for (size_t i = 0; i < wrap_data.size(); i++) {
    rewrap_line(i, 0, false);
  }
}

void wrap_info_t::set_wrap_width(int width) {
  lprintf("Setting wrap width: %d\n", width);
  if (width == wrap_width) {
    return;
  }
  wrap_width = width;
  if (text != nullptr) {
    rewrap_all();
  }
}

void wrap_info_t::set_tabsize(int _tabsize) {
  if (_tabsize == tabsize) {
    return;
  }
  tabsize = _tabsize;
  if (text != nullptr) {
    rewrap_all();
  }
}

void wrap_info_t::set_text_buffer(text_buffer_t *_text) {
  rewrap_connection.disconnect();

  text = _text;
  if (_text == nullptr) {
    return;
  }

  rewrap_connection = text->connect_rewrap_required(bind_front(&wrap_info_t::rewrap, this));

  if (wrap_data.size() > text->impl->lines.size()) {
    delete_lines(text->impl->lines.size(), wrap_data.size());
  }

  for (size_t i = 0; i < wrap_data.size(); i++) {
    rewrap_line(i, 0, false);
  }

  if (wrap_data.size() < text->impl->lines.size()) {
    insert_lines(wrap_data.size(), text->impl->lines.size());
  }
}

void wrap_info_t::rewrap(rewrap_type_t type, int a, int b) {
  switch (type) {
    case rewrap_type_t::REWRAP_ALL:
      rewrap_all();
      break;
    case rewrap_type_t::REWRAP_LINE:
      rewrap_line(a, b, false);
      break;
    case rewrap_type_t::REWRAP_LINE_LOCAL:
      rewrap_line(a, b, true);
      break;
    case rewrap_type_t::INSERT_LINES:
      insert_lines(a, b);
      break;
    case rewrap_type_t::DELETE_LINES:
      delete_lines(a, b);
      break;
    default:
      ASSERT(false);
  }
}

bool wrap_info_t::add_lines(text_coordinate_t &coord, int count) const {
  ASSERT(count > 0);
  while (coord.line < static_cast<int>(wrap_data.size()) &&
         static_cast<int>(wrap_data[coord.line]->size()) <= coord.pos + count) {
    count -= wrap_data[coord.line]->size() - coord.pos;
    coord.line++;
    coord.pos = 0;
  }
  if (coord.line == static_cast<int>(wrap_data.size())) {
    coord.line = wrap_data.size() - 1;
    coord.pos = wrap_data[coord.line]->size() - 1;
    return true;
  } else {
    coord.pos += count;
    return false;
  }
}

bool wrap_info_t::sub_lines(text_coordinate_t &coord, int count) const {
  ASSERT(count > 0);
  if (coord.pos > count) {
    coord.pos -= count;
    return false;
  }
  count -= coord.pos;
  coord.pos = 0;
  while (coord.line > 0 && count >= static_cast<int>(wrap_data[coord.line - 1]->size())) {
    coord.line--;
    count -= wrap_data[coord.line]->size();
  }
  if (count == 0) {
    return false;
  }
  if (coord.line == 0) {
    return true;
  }
  coord.line--;
  coord.pos = wrap_data[coord.line]->size() - count;
  return false;
}

int wrap_info_t::get_line_count(int line) const {
  return static_cast<int>(wrap_data[line]->size());
}

text_coordinate_t wrap_info_t::get_end() const {
  text_coordinate_t result(static_cast<int>(wrap_data.size()) - 1,
                           static_cast<int>(wrap_data[wrap_data.size() - 1]->size()) - 1);
  return result;
}

int wrap_info_t::find_line(text_coordinate_t coord) const {
  size_t i;
  for (i = 1; i < wrap_data[coord.line]->size() && coord.pos >= (*wrap_data[coord.line])[i]; i++) {
  }
  return i - 1;
}

int wrap_info_t::calculate_screen_pos() const { return calculate_screen_pos(&text->cursor); }

int wrap_info_t::calculate_screen_pos(const text_coordinate_t *where) const {
  int sub_line;
  sub_line = find_line(text->cursor);
  return text->impl->lines[where->line]->calculate_screen_width((*wrap_data[where->line])[sub_line],
                                                                where->pos, tabsize);
}

int wrap_info_t::calculate_line_pos(int line, int pos, int sub_line) const {
  return text->impl->lines[line]->calculate_line_pos(
      (*wrap_data[line])[sub_line],
      sub_line + 1 < static_cast<int>(wrap_data[line]->size())
          ? (*wrap_data[line])[sub_line + 1] - 1
          : INT_MAX,
      pos, tabsize);
}

void wrap_info_t::paint_line(t3_window::window_t *win, text_coordinate_t line,
                             text_line_t::paint_info_t *info) const {
  info->start = (*wrap_data[line.line])[line.pos];
  info->flags &= ~text_line_t::BREAK;
  if (line.pos + 1 < static_cast<int>(wrap_data[line.line]->size())) {
    info->max = (*wrap_data[line.line])[line.pos + 1];
    info->flags |= text_line_t::BREAK;
  } else {
    info->max = INT_MAX;
  }
  if (tabsize <= 0) {
    info->flags |= text_line_t::TAB_AS_CONTROL;
  } else {
    info->flags &= ~text_line_t::TAB_AS_CONTROL;
  }
  info->tabsize = tabsize;
  text->paint_line(win, line.line, info);
}

}  // namespace
