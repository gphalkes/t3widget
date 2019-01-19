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
#include <algorithm>
#include <cstring>
#include <ext/alloc_traits.h>
#include <limits>
#include <memory>
#include <string>
#include <t3window/window.h>
#include <type_traits>
#include <utility>
#include <vector>

#include "t3widget/clipboard.h"
#include "t3widget/double_string_adapter.h"
#include "t3widget/findcontext.h"
#include "t3widget/internal.h"
#include "t3widget/key.h"
#include "t3widget/signals.h"
#include "t3widget/string_view.h"
#include "t3widget/textbuffer.h"
#include "t3widget/textbuffer_impl.h"
#include "t3widget/textline.h"
#include "t3widget/tinystring.h"
#include "t3widget/undo.h"
#include "t3widget/util.h"

namespace t3widget {

text_buffer_t::text_buffer_t(text_line_factory_t *_line_factory)
    : impl(new implementation_t(_line_factory)) {}

text_buffer_t::~text_buffer_t() {}

text_pos_t text_buffer_t::size() const { return impl->size(); }

const text_line_t &text_buffer_t::get_line_data(text_pos_t idx) const { return *impl->lines[idx]; }
text_line_t *text_buffer_t::get_mutable_line_data(text_pos_t idx) { return impl->lines[idx].get(); }

text_line_factory_t *text_buffer_t::get_line_factory() { return impl->line_factory; }

bool text_buffer_t::insert_char(key_t c) { return impl->insert_char(c); }

bool text_buffer_t::overwrite_char(key_t c) { return impl->overwrite_char(c); }

bool text_buffer_t::delete_char() { return impl->delete_char(); }

bool text_buffer_t::backspace_char() { return impl->backspace_char(); }

bool text_buffer_t::is_modified() const { return !impl->undo_list.is_at_mark(); }

bool text_buffer_t::merge(bool backspace) { return impl->merge(backspace); }

bool text_buffer_t::append_text(string_view text) { return impl->append_text(text); }

bool text_buffer_t::break_line(const std::string &indent) { return impl->break_line(indent); }

text_pos_t text_buffer_t::calculate_screen_pos(int tabsize) const {
  return calculate_screen_pos(impl->cursor, tabsize);
}

text_pos_t text_buffer_t::calculate_screen_pos(const text_coordinate_t &where, int tabsize) const {
  return impl->lines[where.line]->calculate_screen_width(0, where.pos, tabsize);
}

text_pos_t text_buffer_t::calculate_line_pos(text_pos_t line, text_pos_t pos, int tabsize) const {
  return impl->calculate_line_pos(line, pos, tabsize);
}

void text_buffer_t::paint_line(t3window::window_t *win, text_pos_t line,
                               const text_line_t::paint_info_t &info) {
  prepare_paint_line(line);
  impl->lines[line]->paint_line(win, info);
}

text_pos_t text_buffer_t::get_line_size(text_pos_t line) const { return impl->get_line_size(line); }

void text_buffer_t::goto_next_word() { impl->goto_next_word(); }

void text_buffer_t::goto_previous_word() { impl->goto_previous_word(); }

void text_buffer_t::goto_next_word_boundary() { impl->goto_next_word_boundary(); }

void text_buffer_t::goto_previous_word_boundary() { impl->goto_previous_word_boundary(); }

void text_buffer_t::adjust_position(int adjust) { impl->adjust_position(adjust); }

int text_buffer_t::width_at_cursor() const { return impl->width_at_cursor(); }

bool text_buffer_t::selection_empty() const { return impl->selection_empty(); }

void text_buffer_t::set_selection_end(bool update_primary) {
  impl->set_selection_end(update_primary);
}

text_coordinate_t text_buffer_t::get_selection_start() const { return impl->selection_start; }
text_coordinate_t text_buffer_t::get_selection_end() const { return impl->selection_end; }

void text_buffer_t::delete_block(text_coordinate_t start, text_coordinate_t end) {
  /* Don't do anything on empty block */
  if (start == end) {
    return;
  }

  start_undo_block();
  impl->delete_block_internal(start, end, impl->get_undo(UNDO_DELETE, std::min(start, end)));
  end_undo_block();
}

bool text_buffer_t::insert_block(const std::string &block) { return impl->insert_block(block); }

bool text_buffer_t::replace_block(text_coordinate_t start, text_coordinate_t end,
                                  const std::string &block) {
  return impl->replace_block(start, end, block);
}

std::unique_ptr<std::string> text_buffer_t::convert_block(text_coordinate_t start,
                                                          text_coordinate_t end) {
  return impl->convert_block(start, end);
}

void text_buffer_t::set_undo_mark() { impl->set_undo_mark(); }

/*FIXME: define return values for:
        - nothing done
        - failure
        - success;
*/
int text_buffer_t::apply_undo() {
  undo_t *current = impl->undo_list.back();

  if (current == nullptr) {
    return -1;
  }

  impl->apply_undo_redo(current->get_type(), current);
  return 0;
}

int text_buffer_t::apply_redo() {
  undo_t *current = impl->undo_list.forward();

  if (current == nullptr) {
    return -1;
  }

  impl->apply_undo_redo(current->get_redo_type(), current);
  return 0;
}

void text_buffer_t::set_selection_from_find(const find_result_t &result) {
  impl->set_selection_from_find(result);
}

bool text_buffer_t::find(finder_t *finder, find_result_t *result, bool reverse) const {
  return impl->find(finder, result, reverse);
}

bool text_buffer_t::find_limited(finder_t *finder, text_coordinate_t start, text_coordinate_t end,
                                 find_result_t *result) const {
  return impl->find_limited(finder, start, end, result);
}

void text_buffer_t::replace(const finder_t &finder, const find_result_t &result) {
  std::string replacement_str = finder.get_replacement(impl->lines[result.start.line]->get_data());
  replace_block(result.start, result.end, replacement_str);
}

void text_buffer_t::set_selection_mode(selection_mode_t mode) {
  return impl->set_selection_mode(mode);
}

selection_mode_t text_buffer_t::get_selection_mode() const { return impl->selection_mode; }

bool text_buffer_t::indent_selection(int tabsize, bool tab_spaces) {
  return impl->indent_selection(tabsize, tab_spaces);
}

bool text_buffer_t::indent_block(text_coordinate_t &start, text_coordinate_t &end, int tabsize,
                                 bool tab_spaces) {
  return impl->indent_block(start, end, tabsize, tab_spaces);
}

bool text_buffer_t::unindent_selection(int tabsize) { return impl->unindent_selection(tabsize); }

bool text_buffer_t::unindent_block(text_coordinate_t &start, text_coordinate_t &end, int tabsize) {
  return impl->unindent_block(start, end, tabsize);
}

bool text_buffer_t::unindent_line(int tabsize) { return impl->unindent_line(tabsize); }

void text_buffer_t::prepare_paint_line(text_pos_t line) { (void)line; }

void text_buffer_t::start_undo_block() { impl->start_undo_block(); }

void text_buffer_t::end_undo_block() { impl->end_undo_block(); }

void text_buffer_t::goto_pos(text_pos_t line, text_pos_t pos) { impl->goto_pos(line, pos); }

text_coordinate_t text_buffer_t::get_cursor() const { return impl->cursor; }

void text_buffer_t::set_cursor(text_coordinate_t _cursor) { impl->cursor = _cursor; }

void text_buffer_t::set_cursor_pos(text_pos_t pos) { impl->cursor.pos = pos; }

_T3_WIDGET_IMPL_SIGNAL(text_buffer_t, rewrap_required, rewrap_type_t, text_pos_t, text_pos_t)

//==================================== implementation_t ============================================

text_pos_t text_buffer_t::implementation_t::calculate_line_pos(text_pos_t line, text_pos_t pos,
                                                               int tabsize) const {
  return lines[line]->calculate_line_pos(0, std::numeric_limits<text_pos_t>::max(), pos, tabsize);
}

bool text_buffer_t::implementation_t::insert_char(key_t c) {
  if (!lines[cursor.line]->insert_char(cursor.pos, c, get_undo(UNDO_ADD))) {
    return false;
  }

  rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
  last_undo_position = cursor;

  return true;
}

bool text_buffer_t::implementation_t::overwrite_char(key_t c) {
  if (!lines[cursor.line]->overwrite_char(cursor.pos, c, get_undo(UNDO_OVERWRITE))) {
    return false;
  }
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);
  rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
  last_undo_position = cursor;

  return true;
}

bool text_buffer_t::implementation_t::delete_char() {
  if (!lines[cursor.line]->delete_char(cursor.pos, get_undo(UNDO_DELETE))) {
    return false;
  }
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);
  rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);
  return true;
}

bool text_buffer_t::implementation_t::backspace_char() {
  text_pos_t newpos;

  newpos = lines[cursor.line]->adjust_position(cursor.pos, -1);
  if (!lines[cursor.line]->backspace_char(cursor.pos, get_undo(UNDO_BACKSPACE))) {
    return false;
  }
  cursor.pos = newpos;
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);

  rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

  last_undo_position = cursor;
  return true;
}

bool text_buffer_t::implementation_t::merge_internal(text_pos_t line) {
  cursor.line = line;
  cursor.pos = lines[line]->size();
  lines[line]->merge(std::move(lines[line + 1]));
  lines.erase(lines.begin() + line + 1);
  rewrap_required(rewrap_type_t::DELETE_LINES, line + 1, line + 2);
  rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
  return true;
}

bool text_buffer_t::implementation_t::insert_block_internal(text_coordinate_t insert_at,
                                                            std::unique_ptr<text_line_t> block) {
  std::unique_ptr<text_line_t> second_half, next_line;
  text_pos_t next_start = 0;
  // FIXME: check that everything succeeds and return false if it doesn't
  if (insert_at.pos >= 0 && insert_at.pos < lines[insert_at.line]->size()) {
    second_half = lines[insert_at.line]->break_line(insert_at.pos);
  }

  lines[insert_at.line]->merge(block->break_on_nl(&next_start));
  rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, insert_at.pos);

  while (next_start > 0) {
    insert_at.line++;
    lines.insert(lines.begin() + insert_at.line, block->break_on_nl(&next_start));
    rewrap_required(rewrap_type_t::INSERT_LINES, insert_at.line, insert_at.line + 1);
  }

  cursor.pos = lines[insert_at.line]->size();

  if (second_half != nullptr) {
    lines[insert_at.line]->merge(std::move(second_half));
    rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, cursor.pos);
  }

  cursor.line = insert_at.line;
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);
  return true;
}

void text_buffer_t::implementation_t::delete_block_internal(text_coordinate_t start,
                                                            text_coordinate_t end, undo_t *undo) {
  text_line_t *start_part = nullptr;
  std::unique_ptr<text_line_t> end_part;

  /* Don't do anything on empty block */
  if (start == end) {
    return;
  }

  /* Swap start and end if end is before start. This simplifies the code below. */
  if ((end.line < start.line) || (end.line == start.line && end.pos < start.pos)) {
    text_coordinate_t tmp = start;
    start = end;
    end = tmp;
  }
  cursor = start;

  if (start.line == end.line) {
    std::unique_ptr<text_line_t> selected_text = lines[start.line]->cut_line(start.pos, end.pos);
    if (undo != nullptr) {
      undo->get_text()->append(selected_text->get_data());
    }
    cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);
    rewrap_required(rewrap_type_t::REWRAP_LINE, start.line, start.pos);
    return;
  }

  /* Cases:
          - first line is completely removed
          - first/last line is broken halfway
          - eol at first line is removed
          - last line is emptied
          - last line is unmodified

          principle: remainder of first line is merged with remainder of last line
          Optimisations
          - first line is completely removed (start.pos == 0)
                  then the merge is unnecessary and the remainder of the last line
                  can replace the first line
          - first line eol is removed
                  then no break is necessary for the first line
          - last line is completely emptied (end.pos == Line_getLineLength(lines[end.line]))
                  then no merge is necessary
          - last line is unmodified (end.pos == 0)
                  then no break is necessary for the last line
  */

  if (start.pos == lines[start.line]->size()) {
    start_part = lines[start.line].get();
  } else if (start.pos != 0) {
    std::unique_ptr<text_line_t> retval = lines[start.line]->break_line(start.pos);
    if (undo != nullptr) {
      undo->get_text()->append(retval->get_data());
    }
    start_part = lines[start.line].get();
  }

  if (end.pos == 0) {
    end_part = std::move(lines[end.line]);
  } else if (end.pos < lines[end.line]->size()) {
    end_part = lines[end.line]->break_line(end.pos);
  }

  if (start_part == nullptr) {
    if (undo != nullptr) {
      undo->get_text()->append(lines[start.line]->get_data());
    }
    if (end_part != nullptr) {
      lines[start.line] = std::move(end_part);
    } else {
      lines[start.line] = line_factory->new_text_line_t();
    }
  } else {
    if (end_part != nullptr) {
      start_part->merge(std::move(end_part));
    }
  }

  start.line++;

  if (undo != nullptr) {
    undo->add_newline();

    for (text_pos_t i = start.line; i < end.line; i++) {
      undo->get_text()->append(lines[i]->get_data());
      undo->add_newline();
    }

    if (end.pos != 0) {
      undo->get_text()->append(lines[end.line]->get_data());
    }
  }
  end.line++;
  lines.erase(lines.begin() + start.line, lines.begin() + end.line);
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);

  rewrap_required(rewrap_type_t::DELETE_LINES, start.line, end.line);
  rewrap_required(rewrap_type_t::REWRAP_LINE, start.line - 1, start.pos);
  if (static_cast<size_t>(start.line) < lines.size()) {
    rewrap_required(rewrap_type_t::REWRAP_LINE, start.line, 0);
  }
}

bool text_buffer_t::implementation_t::break_line_internal(const std::string &indent) {
  std::unique_ptr<text_line_t> insert = lines[cursor.line]->break_line(cursor.pos);
  lines.insert(lines.begin() + cursor.line + 1, std::move(insert));
  rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
  rewrap_required(rewrap_type_t::INSERT_LINES, cursor.line + 1, cursor.line + 2);
  cursor.line++;
  if (indent.empty()) {
    cursor.pos = 0;
  } else {
    std::unique_ptr<text_line_t> new_line = line_factory->new_text_line_t(indent);
    new_line->merge(std::move(lines[cursor.line]));
    lines[cursor.line] = std::move(new_line);
    cursor.pos = indent.size();
  }
  return true;
}

bool text_buffer_t::implementation_t::merge(bool backspace) {
  start_undo_block();
  text_pos_t delete_line = backspace ? cursor.line - 1 : cursor.line;
  undo_t *undo = get_undo(UNDO_DELETE, text_coordinate_t(delete_line, lines[delete_line]->size()));
  undo->get_text()->append(1, '\n');
  merge_internal(delete_line);
  cursor.line = delete_line;
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 0);
  end_undo_block();
  return true;
}

bool text_buffer_t::implementation_t::append_text(string_view text) {
  bool result;
  text_coordinate_t at(lines.size() - 1, std::numeric_limits<text_pos_t>::max());
  result = insert_block_internal(at, line_factory->new_text_line_t(text));
  return result;
}

bool text_buffer_t::implementation_t::break_line(const std::string &indent) {
  start_undo_block();
  undo_t *undo = get_undo(UNDO_ADD);
  undo->get_text()->append(1, '\n');
  undo->get_text()->append(indent);
  bool result = break_line_internal(indent);
  end_undo_block();
  return result;
}

bool text_buffer_t::implementation_t::insert_block(const std::string &block) {
  text_coordinate_t cursor_at_start = cursor;
  std::unique_ptr<text_line_t> converted_block = line_factory->new_text_line_t(block);
  std::string sanitized_block(converted_block->get_data());
  undo_t *undo;

  if (!insert_block_internal(cursor, std::move(converted_block))) {
    return false;
  }

  get_undo(UNDO_BLOCK_START, cursor_at_start);
  undo = get_undo(UNDO_ADD, cursor_at_start);
  undo->get_text()->append(sanitized_block);
  get_undo(UNDO_BLOCK_END, cursor);

  return true;
}

bool text_buffer_t::implementation_t::replace_block(text_coordinate_t start, text_coordinate_t end,
                                                    const std::string &block) {
  std::unique_ptr<text_line_t> converted_block;

  // FIXME: check that everything succeeds and return false if it doesn't
  // FIXME: make sure original state is restored on failing sub action

  start_undo_block();
  text_coordinate_t loc = std::min(start, end);
  if (start != end) {
    delete_block_internal(start, end, get_undo(UNDO_DELETE, loc));
  }

  converted_block = line_factory->new_text_line_t(block);
  *get_undo(UNDO_ADD, loc)->get_text() = converted_block->get_data();
  // FIXME: insert_block_internal may fail!!!
  insert_block_internal(loc, std::move(converted_block));
  end_undo_block();
  return true;
}

std::unique_ptr<std::string> text_buffer_t::implementation_t::convert_block(text_coordinate_t start,
                                                                            text_coordinate_t end) {
  text_coordinate_t current_start, current_end;

  current_start = start;
  current_end = end;

  /* Don't do anything on empty selection */
  if (current_start.line == current_end.line && current_start.pos == current_end.pos) {
    return nullptr;
  }

  /* Swap start and end if end is before start. This simplifies the code below. */
  if ((current_end.line < current_start.line) ||
      (current_end.line == current_start.line && current_end.pos < current_start.pos)) {
    text_coordinate_t tmp = current_start;
    current_start = current_end;
    current_end = tmp;
  }

  if (current_start.line == current_end.line) {
    return t3widget::make_unique<std::string>(lines[current_start.line]->get_data(),
                                              current_start.pos,
                                              current_end.pos - current_start.pos);
  }

  // FIXME: new and append may fail!
  std::unique_ptr<std::string> retval(
      new std::string(lines[current_start.line]->get_data(), current_start.pos));
  retval->append(1, '\n');

  for (text_pos_t i = current_start.line + 1; i < current_end.line; i++) {
    // FIXME: append may fail!
    retval->append(lines[i]->get_data());
    retval->append(1, '\n');
  }

  // FIXME: append may fail!
  retval->append(lines[current_end.line]->get_data(), 0, current_end.pos);
  return retval;
}

void text_buffer_t::implementation_t::goto_next_word() {
  text_line_t *line = lines[cursor.line].get();

  /* Use -1 as an indicator for end of line */
  if (cursor.pos >= line->size()) {
    cursor.pos = -1;
    /* Keep skipping to next line if no word can be found */
    while (cursor.pos < 0) {
      if (static_cast<size_t>(cursor.line) + 1 >= lines.size()) {
        break;
      }
      line = lines[++cursor.line].get();
      cursor.pos = line->get_next_word(-1);
    }
  } else if (cursor.pos >= 0) {
    cursor.pos = line->get_next_word(cursor.pos);
  }

  /* Convert cursor.line and cursor.pos to text coordinate again. */
  if (cursor.pos < 0) {
    cursor.pos = line->size();
  }
}

void text_buffer_t::implementation_t::goto_previous_word() {
  text_line_t *line = lines[cursor.line].get();

  cursor.pos = line->get_previous_word(cursor.pos);

  /* Keep skipping to next line if no word can be found */
  while (cursor.pos < 0 && cursor.line > 0) {
    line = lines[--cursor.line].get();
    cursor.pos = line->get_previous_word(-1);
  }

  /* Convert cursor.line and cursor.pos to text coordinate again. */
  if (cursor.pos < 0) {
    cursor.pos = 0;
  }
}

void text_buffer_t::implementation_t::goto_next_word_boundary() {
  cursor.pos = lines[cursor.line]->get_next_word_boundary(cursor.pos);
}

void text_buffer_t::implementation_t::goto_previous_word_boundary() {
  cursor.pos = lines[cursor.line]->get_previous_word_boundary(cursor.pos);
}

void text_buffer_t::implementation_t::adjust_position(int adjust) {
  cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, adjust);
}

int text_buffer_t::implementation_t::width_at_cursor() const {
  return lines[cursor.line]->width_at(cursor.pos);
}

void text_buffer_t::implementation_t::set_selection_mode(selection_mode_t mode) {
  switch (mode) {
    case selection_mode_t::NONE:
      selection_start = text_coordinate_t(0, -1);
      selection_end = text_coordinate_t(0, -1);
      break;
    case selection_mode_t::ALL:
      selection_start = text_coordinate_t(0, 0);
      selection_end = text_coordinate_t(lines.size() - 1, get_line_size(lines.size() - 1));
      break;
    default:
      if (selection_mode == selection_mode_t::ALL || selection_mode == selection_mode_t::NONE) {
        selection_start = cursor;
        selection_end = cursor;
      }
      break;
  }
  selection_mode = mode;
}

void text_buffer_t::implementation_t::set_selection_end(bool update_primary) {
  selection_end = cursor;
  if (update_primary) {
    set_primary(convert_block(selection_start, selection_end));
  }
}

undo_t *text_buffer_t::implementation_t::get_undo(undo_type_t type) {
  return get_undo(type, cursor);
}

undo_t *text_buffer_t::implementation_t::get_undo(undo_type_t type, text_coordinate_t coord) {
  if (last_undo_type == type && last_undo_position.line == coord.line &&
      last_undo_position.pos == coord.pos && last_undo != nullptr) {
    return last_undo;
  }

  if (last_undo != nullptr) {
    last_undo->minimize();
  }

  ASSERT(type != UNDO_NONE && type <= UNDO_BLOCK_END);
  last_undo_position = coord;

  last_undo = undo_list.add(type, coord);
  switch (type) {
    case UNDO_BLOCK_START:
    case UNDO_BLOCK_END:
    case UNDO_INDENT:
    case UNDO_UNINDENT:
      last_undo_type = UNDO_NONE;
      break;
    default:
      last_undo_type = type;
      break;
  }
  return last_undo;
}

void text_buffer_t::implementation_t::set_undo_mark() {
  undo_list.set_mark();
  if (last_undo != nullptr) {
    last_undo->minimize();
  }
  last_undo_type = UNDO_NONE;
}

// FIXME: re-implement the complex block operations in terms of UNDO_BLOCK_START/END and the simple
// operations.

void text_buffer_t::implementation_t::apply_undo_redo(undo_type_t type, undo_t *current) {
  text_coordinate_t start, end;

  set_selection_mode(selection_mode_t::NONE);
  switch (type) {
    case UNDO_ADD: {
      end = start = current->get_start();
      size_t newline = current->get_text()->find_last_of('\n');
      if (newline == std::string::npos) {
        end.pos += current->get_text()->size();
      } else {
        end.pos = current->get_text()->size() - newline - 1;
        tiny_string_t::iterator begin = current->get_text()->begin();
        end.line += std::count(begin, begin + newline, '\n') + 1;
      }
      delete_block_internal(start, end, nullptr);
      break;
    }
    case UNDO_ADD_REDO:
    case UNDO_DELETE:
      start = current->get_start();
      insert_block_internal(start, line_factory->new_text_line_t(*current->get_text()));
      if (type == UNDO_DELETE) {
        cursor = start;
      }
      break;
    case UNDO_BACKSPACE:
      start = current->get_start();
      start.pos -= current->get_text()->size();
      insert_block_internal(start, line_factory->new_text_line_t(*current->get_text()));
      break;
    case UNDO_BACKSPACE_REDO:
      end = start = current->get_start();
      start.pos -= current->get_text()->size();
      delete_block_internal(start, end, nullptr);
      break;
    case UNDO_OVERWRITE: {
      double_string_adapter_t undo_adapter(current->get_text());
      end = start = current->get_start();
      end.pos += undo_adapter.second().size();
      delete_block_internal(start, end, nullptr);
      insert_block_internal(start, line_factory->new_text_line_t(undo_adapter.first()));
      cursor = start;
      break;
    }
    case UNDO_OVERWRITE_REDO: {
      double_string_adapter_t undo_adapter(current->get_text());
      end = start = current->get_start();
      end.pos += undo_adapter.first().size();
      delete_block_internal(start, end, nullptr);
      insert_block_internal(start, line_factory->new_text_line_t(undo_adapter.second()));
      break;
    }
    case UNDO_INDENT:
    case UNDO_UNINDENT:
      undo_indent_selection(current, type);
      break;
    case UNDO_BLOCK_START:
    case UNDO_BLOCK_END_REDO:
      cursor = current->get_start();
      break;
    case UNDO_BLOCK_END:
      do {
        current = undo_list.back();
        apply_undo_redo(current->get_type(), current);
      } while (current != nullptr && current->get_type() != UNDO_BLOCK_START);
      ASSERT(current != nullptr);
      break;
    case UNDO_BLOCK_START_REDO:
      do {
        current = undo_list.forward();
        apply_undo_redo(current->get_redo_type(), current);
      } while (current != nullptr && current->get_redo_type() != UNDO_BLOCK_END_REDO);
      ASSERT(current != nullptr);
      break;
    default:
      ASSERT(false);
      break;
  }
  last_undo_type = UNDO_NONE;
}

void text_buffer_t::implementation_t::set_selection_from_find(const find_result_t &result) {
  selection_start = result.start;

  selection_end = result.end;

  cursor = selection_end;
  selection_mode = selection_mode_t::SHIFT;
  set_primary(convert_block(selection_start, selection_end));
}

bool text_buffer_t::implementation_t::find(finder_t *finder, find_result_t *result,
                                           bool reverse) const {
  text_pos_t start, idx;

  /* Note: the value of result->start.line and result->end.line are ignored after the
     search has started. The finder->match function does not take those values into
     account. */

  // Perform search
  if (((finder->get_flags() & find_flags_t::BACKWARD) != 0) ^ reverse) {
    start = idx = result->start.line;
    result->end = result->start;
    result->start.pos = -1;
    if (finder->match(lines[idx]->get_data(), result, true)) {
      result->start.line = result->end.line = idx;
      return true;
    }

    result->end.pos = -1;
    for (; idx > 0;) {
      idx--;
      if (finder->match(lines[idx]->get_data(), result, true)) {
        result->start.line = result->end.line = idx;
        return true;
      }
    }

    if (!(finder->get_flags() & find_flags_t::WRAP)) {
      return false;
    }

    for (idx = lines.size(); idx > start;) {
      idx--;
      if (finder->match(lines[idx]->get_data(), result, true)) {
        result->start.line = result->end.line = idx;
        return true;
      }
    }
  } else {
    start = idx = cursor.line;
    result->start = cursor;
    result->end.pos = -1;
    if (finder->match(lines[idx]->get_data(), result, false)) {
      result->start.line = result->end.line = idx;
      return true;
    }

    result->start.pos = -1;
    const size_t lines_size = lines.size();
    for (idx++; idx < static_cast<text_pos_t>(lines_size); idx++) {
      if (finder->match(lines[idx]->get_data(), result, false)) {
        result->start.line = result->end.line = idx;
        return true;
      }
    }

    if (!(finder->get_flags() & find_flags_t::WRAP)) {
      return false;
    }

    for (idx = 0; idx <= start; idx++) {
      if (finder->match(lines[idx]->get_data(), result, false)) {
        result->start.line = result->end.line = idx;
        return true;
      }
    }
  }

  return false;
}

bool text_buffer_t::implementation_t::find_limited(finder_t *finder, text_coordinate_t start,
                                                   text_coordinate_t end,
                                                   find_result_t *result) const {
  text_pos_t idx;

  /* Note: the finder->match function does not take value of result->start.line
     and result->end.line into account. */
  result->start = start;
  result->end.pos = -1;

  for (idx = start.line; static_cast<size_t>(idx) < lines.size() && idx < end.line; idx++) {
    if (finder->match(lines[idx]->get_data(), result, false)) {
      result->start.line = result->end.line = idx;
      return true;
    }
    result->start.pos = -1;
  }

  result->end = end;
  if (static_cast<size_t>(idx) < lines.size() &&
      finder->match(lines[idx]->get_data(), result, false)) {
    result->start.line = result->end.line = idx;
    return true;
  }

  return false;
}

bool text_buffer_t::implementation_t::indent_block(text_coordinate_t &start, text_coordinate_t &end,
                                                   int tabsize, bool tab_spaces) {
  text_pos_t end_line;
  text_coordinate_t insert_at;
  std::string str;
  undo_t *undo;

  start_undo_block();
  undo = get_undo(UNDO_INDENT, std::min(start, end));

  if (tab_spaces) {
    str.append(tabsize, ' ');
  } else {
    str.append(1, '\t');
  }

  insert_at.pos = 0;
  if (end.line < start.line) {
    insert_at.line = end.line;
    end_line = start.line;
    if (start.pos == 0) {
      end_line--;
    }
  } else {
    insert_at.line = start.line;
    end_line = end.line;
    if (end.pos == 0) {
      end_line--;
    }
  }
  tiny_string_t *undo_text = undo->get_text();

  for (; insert_at.line <= end_line; insert_at.line++) {
    undo_text->append(str);
    undo_text->append(1, 'X');  // Simply add a non-space/tab as marker
    insert_block_internal(insert_at, line_factory->new_text_line_t(str));
  }
  start.pos = 0;
  if (end.pos == 0) {
    undo_text->append(1, 'X');  // Simply add a non-space/tab as marker
    cursor.line = end.line;
  } else {
    end.pos += tab_spaces ? tabsize : 1;
  }
  cursor.pos = end.pos;
  end_undo_block();

  return true;
}

bool text_buffer_t::implementation_t::indent_selection(int tabsize, bool tab_spaces) {
  // FIXME: move this check to calls of indent_selection
  if (selection_mode == selection_mode_t::NONE && selection_start.line != selection_end.line) {
    return true;
  }
  return indent_block(selection_start, selection_end, tabsize, tab_spaces);
}

bool text_buffer_t::implementation_t::undo_indent_selection(undo_t *undo, undo_type_t type) {
  text_pos_t first_line;
  size_t pos = 0, next_pos = 0;

  first_line = undo->get_start().line;

  tiny_string_t *undo_text = undo->get_text();
  bool last = false;
  for (; !last; first_line++) {
    next_pos = undo_text->find('X', pos);

    if (next_pos == std::string::npos) {
      next_pos = undo_text->size();
      last = true;
    }

    if (type == UNDO_INDENT) {
      text_coordinate_t delete_start, delete_end;
      delete_start.line = delete_end.line = first_line;
      delete_start.pos = 0;
      delete_end.pos = next_pos - pos;
      delete_block_internal(delete_start, delete_end, nullptr);
    } else {
      text_coordinate_t insert_at(first_line, 0);
      if (next_pos != pos) {
        insert_block_internal(insert_at, line_factory->new_text_line_t(
                                             string_view(undo_text->data() + pos, next_pos - pos)));
      }
    }
    pos = next_pos + 1;
  }
  return true;
}

bool text_buffer_t::implementation_t::unindent_selection(int tabsize) {
  // FIXME: move this check to calls of unindent_selection
  if (selection_mode == selection_mode_t::NONE && selection_start.line != selection_end.line) {
    return true;
  }
  return unindent_block(selection_start, selection_end, tabsize);
}

bool text_buffer_t::implementation_t::unindent_block(text_coordinate_t &start,
                                                     text_coordinate_t &end, int tabsize) {
  text_pos_t end_line;
  text_coordinate_t delete_start, delete_end;
  std::string undo_text;
  bool text_changed = false;

  start_undo_block();
  if (end.line < start.line) {
    delete_start.line = end.line;
    end_line = start.line;
    if (start.pos == 0) {
      end_line--;
    }
  } else {
    delete_start.line = start.line;
    end_line = end.line;
    if (end.pos == 0) {
      end_line--;
    }
  }

  delete_start.pos = 0;
  for (; delete_start.line <= end_line; delete_start.line++) {
    const std::string &data = lines[delete_start.line]->get_data();
    for (delete_end.pos = 0; delete_end.pos < tabsize; delete_end.pos++) {
      if (data[delete_end.pos] == '\t') {
        delete_end.pos++;
        break;
      } else if (data[delete_end.pos] != ' ') {
        break;
      }
    }

    undo_text.append(data, 0, delete_end.pos);
    undo_text.append(1, 'X');  // Simply add a non-space/tab as marker
    if (delete_end.pos == 0) {
      continue;
    }
    text_changed = true;
    delete_end.line = delete_start.line;
    delete_block_internal(delete_start, delete_end, nullptr);

    if (delete_end.line == start.line) {
      if (start.pos > delete_end.pos) {
        start.pos -= delete_end.pos;
      } else {
        start.pos = 0;
      }
    } else if (delete_end.line == end.line) {
      if (end.pos > delete_end.pos) {
        cursor.pos = end.pos - delete_end.pos;
      } else {
        cursor.pos = 0;
      }
    }
  }
  if (end.line > start.line && end_line != end.line) {
    undo_text.append(1, 'X');  // Simply add a non-space/tab as marker
  }

  if (text_changed) {
    undo_t *undo = get_undo(UNDO_UNINDENT, std::min(start, end));
    undo->get_text()->append(undo_text);
  }
  cursor.line = end.line;
  end_undo_block();
  return true;
}

bool text_buffer_t::implementation_t::unindent_line(int tabsize) {
  text_coordinate_t delete_start(cursor.line, 0), delete_end(cursor.line, 0);
  text_coordinate_t saved_cursor;
  undo_t *undo;

  start_undo_block();
  // FIXME: move this check to calls of unindent_line
  if (selection_mode != selection_mode_t::NONE) {
    set_selection_mode(selection_mode_t::NONE);
  }

  const std::string &data = lines[delete_start.line]->get_data();
  for (; delete_end.pos < tabsize; delete_end.pos++) {
    if (data[delete_end.pos] == '\t') {
      delete_end.pos++;
      break;
    } else if (data[delete_end.pos] != ' ') {
      break;
    }
  }

  if (delete_end.pos == 0) {
    return true;
  }

  undo = get_undo(UNDO_UNINDENT, std::min(delete_start, cursor));
  undo->get_text()->append(string_view(data).substr(0, delete_end.pos));
  undo->get_text()->append("X");

  /* delete_block_interal sets the cursor position to the position where the
     block was removed. In this particular case, that is not desired. */
  saved_cursor = cursor;
  delete_block_internal(delete_start, delete_end, nullptr);
  cursor = saved_cursor;
  if (cursor.pos > delete_end.pos) {
    cursor.pos -= delete_end.pos;
  } else {
    cursor.pos = 0;
  }
  end_undo_block();
  return true;
}

void text_buffer_t::implementation_t::goto_pos(text_pos_t line, text_pos_t pos) {
  if (line < 1 && pos < 1) {
    return;
  }

  if (line >= 1) {
    cursor.line = (line > size() ? size() : line) - 1;
  }
  if (pos >= 1) {
    text_pos_t screen_pos = lines[cursor.line]->calculate_screen_width(0, pos - 1, 1);
    cursor.pos = calculate_line_pos(cursor.line, screen_pos, 1);
  }
}

}  // namespace t3widget
