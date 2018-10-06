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
/* _XOPEN_SOURCE is defined to enable wcswidth. */
#define _XOPEN_SOURCE

#include <cstring>
#include <t3window/utf8.h>

#include "t3widget/colorscheme.h"
#include "t3widget/double_string_adapter.h"
#include "t3widget/findcontext.h"
#include "t3widget/internal.h"
#include "t3widget/stringmatcher.h"
#include "t3widget/textline.h"
#include "t3widget/undo.h"
#include "t3widget/util.h"

namespace t3widget {

char text_line_t::spaces[_T3_MAX_TAB];
char text_line_t::dashes[_T3_MAX_TAB];
char text_line_t::dots[16];
const char *text_line_t::control_map = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]_^";
const char *text_line_t::wrap_symbol = "\xE2\x86\xB5";

text_line_factory_t default_text_line_factory;

// Returns whether a codepoint is one of the conjoining Jamo L codepoints.
static bool is_conjoining_jamo_l(uint32_t c) { return c >= 0x1100 && c <= 0x1112; }

// Returns whether a codepoint is one of the conjoining Jamo V codepoints.
static bool is_conjoining_jamo_v(uint32_t c) { return c >= 0x1161 && c <= 0x1175; }

// Returns whether a codepoint is one of the conjoining Jamo T codepoints.
static bool is_conjoining_jamo_t(uint32_t c) { return c >= 0x11A7 && c <= 0x11C2; }

// Returns whether a codepoint is one of the conjoining Jamo LV codepoints.
static bool is_conjoining_jamo_lv(uint32_t c) {
  if (c >= 0xAC00 && c <= 0xD788) {
    return (c - 0xAC00) % 28 == 0;
  }
  return false;
}

text_line_t::text_line_t(int buffersize, text_line_factory_t *_factory)
    : starts_with_combining(false),
      factory(_factory == nullptr ? &default_text_line_factory : _factory) {
  reserve(buffersize);
}

text_line_t::~text_line_t() {}

void text_line_t::fill_line(string_view _buffer) {
  size_t char_bytes, round_trip_bytes;
  key_t next;
  char byte_buffer[5];

  /* If _buffer is valid UTF-8, we will end up with a buffer of size length.
     So just tell the buffer that, such that it can allocate an appropriately
     sized buffer. */
  reserve(_buffer.size());

  while (!_buffer.empty()) {
    char_bytes = _buffer.size();
    next = t3_utf8_get(_buffer.data(), &char_bytes);
    round_trip_bytes = t3_utf8_put(next, byte_buffer);
    buffer.append(byte_buffer, round_trip_bytes);
    _buffer.remove_prefix(char_bytes);
  }
  starts_with_combining = buffer.size() > 0 && width_at(0) == 0;
}

text_line_t::text_line_t(string_view _buffer, text_line_factory_t *_factory)
    : starts_with_combining(false),
      factory(_factory == nullptr ? &default_text_line_factory : _factory) {
  fill_line(_buffer);
}

void text_line_t::set_text(string_view _buffer) {
  buffer.clear();
  fill_line(_buffer);
}

/* Merge line2 into line1, freeing line2 */
void text_line_t::merge(std::unique_ptr<text_line_t> other) {
  int buffer_len = buffer.size();

  if (buffer_len == 0 && other->starts_with_combining) {
    starts_with_combining = true;
  }

  reserve(buffer.size() + other->buffer.size());

  buffer += other->buffer;
}

/* Break up 'line' at position 'pos'. This means that the character at 'pos'
   is the first character in the new line. The right part of the line is
   returned the left part remains in 'line' */
std::unique_ptr<text_line_t> text_line_t::break_line(t3widget::text_pos_t pos) {
  std::unique_ptr<text_line_t> newline;

  // FIXME: cut_line and break_line are very similar. Maybe we should combine them!
  if (static_cast<size_t>(pos) == buffer.size()) {
    return factory->new_text_line_t();
  }

  /* Only allow line breaks at non-combining marks. This doesn't use width_at, because
     conjoining Jamo will make it return 0, but we need to allow them to be split. */
  ASSERT(t3_utf8_wcwidth(t3_utf8_get(buffer.data() + pos, nullptr)));

  /* copy the right part of the string into the new buffer */
  newline = factory->new_text_line_t(buffer.size() - pos);
  newline->buffer.assign(buffer.data() + pos, buffer.size() - pos);

  buffer.resize(pos);
  return newline;
}

std::unique_ptr<text_line_t> text_line_t::cut_line(text_pos_t start, text_pos_t end) {
  std::unique_ptr<text_line_t> retval;

  ASSERT(static_cast<size_t>(end) == buffer.size() ||
         t3_utf8_wcwidth(t3_utf8_get(buffer.data() + end, nullptr)) != 0);
  // FIXME: special case: if the selection cover a whole text_line_t (note: not wrapped) we
  // shouldn't copy

  retval = clone(start, end);

  buffer.erase(start, (end - start));
  starts_with_combining = buffer.size() > 0 && width_at(0) == 0;

  return retval;
}

std::unique_ptr<text_line_t> text_line_t::clone(text_pos_t start, text_pos_t end) {
  if (end == -1) {
    end = buffer.size();
  }

  ASSERT(static_cast<size_t>(end) <= buffer.size());
  ASSERT(start >= 0);
  ASSERT(start <= end);

  if (start == end) {
    return factory->new_text_line_t(0);
  }

  std::unique_ptr<text_line_t> retval = factory->new_text_line_t((end - start));

  retval->buffer.assign(buffer.data() + start, (end - start));
  retval->starts_with_combining = width_at(start) == 0;

  return retval;
}

std::unique_ptr<text_line_t> text_line_t::break_on_nl(text_pos_t *startFrom) {
  text_pos_t i;

  for (i = *startFrom; static_cast<size_t>(i) < buffer.size(); i++) {
    if (buffer[i] == '\n') {
      break;
    }
  }

  std::unique_ptr<text_line_t> retval = clone(*startFrom, i);

  *startFrom = static_cast<size_t>(i) == buffer.size() ? -1 : i + 1;
  return retval;
}

void text_line_t::insert(std::unique_ptr<text_line_t> other, t3widget::text_pos_t pos) {
  ASSERT(pos >= 0 && static_cast<size_t>(pos) <= buffer.size());

  reserve(buffer.size() + other->buffer.size());
  buffer.insert(pos, other->buffer);
  if (pos == 0) {
    starts_with_combining = other->starts_with_combining;
  }
}

void text_line_t::minimize() {
#ifdef HAS_STRING_SHRINK_TO_FIT
  buffer.shrink_to_fit();
#else
  reserve(0);
#endif
}

/* Calculate the screen width of the characters from 'start' to 'pos' with tabsize 'tabsize' */
/* tabsize == 0 -> tab as control */
text_pos_t text_line_t::calculate_screen_width(text_pos_t start, text_pos_t pos,
                                               int tabsize) const {
  text_pos_t i, total = 0;

  if (starts_with_combining && start == 0 && pos > 0) {
    total++;
  }

  for (i = start; static_cast<size_t>(i) < buffer.size() && i < pos;
       i += byte_width_from_first(i)) {
    if (buffer[i] == '\t') {
      total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
    } else {
      total += width_at(i);
    }
  }

  return total;
}

/* Return the line position in text_line_t associated with screen position 'pos' or
   length if it is outside of 'line'. */
text_pos_t text_line_t::calculate_line_pos(text_pos_t start, text_pos_t max, text_pos_t pos,
                                           int tabsize) const {
  text_pos_t i, total = 0;

  if (pos == 0) {
    return start;
  }

  if (start == 0 && starts_with_combining) {
    pos--;
  }

  for (i = start; static_cast<size_t>(i) < buffer.size() && i < max;
       i += byte_width_from_first(i)) {
    if (buffer[i] == '\t') {
      total += tabsize - (total % tabsize);
    } else {
      total += width_at(i);
    }

    if (total > pos) {
      return i;
    }
  }

  return std::min(max, get_length());
}

void text_line_t::paint_part(t3window::window_t *win, const char *paint_buffer, text_pos_t todo,
                             bool is_print, t3_attr_t selection_attr) {
  if (todo <= 0) {
    return;
  }

  if (is_print) {
    win->addnstr(paint_buffer, todo, selection_attr);
  } else {
    selection_attr = t3_term_combine_attrs(attributes.non_print, selection_attr);
    for (; static_cast<size_t>(todo) > sizeof(dots); todo -= sizeof(dots)) {
      win->addnstr(dots, sizeof(dots), selection_attr);
    }
    win->addnstr(dots, todo, selection_attr);
  }
}

t3_attr_t text_line_t::get_base_attr(text_pos_t i, const text_line_t::paint_info_t &info) {
  (void)i;
  return info.normal_attr;
}

t3_attr_t text_line_t::get_draw_attrs(text_pos_t i, const text_line_t::paint_info_t &info) {
  t3_attr_t retval = get_base_attr(i, info);

  if (i >= info.selection_start && i < info.selection_end) {
    retval = i == info.cursor ? t3_term_combine_attrs(attributes.text_selection_cursor2, retval)
                              : info.selected_attr;
  } else if (i == info.cursor) {
    retval = t3_term_combine_attrs(
        i == info.selection_end ? attributes.text_selection_cursor : attributes.text_cursor,
        retval);
  }

  if (is_bad_draw(i)) {
    retval = t3_term_combine_attrs(attributes.bad_draw, retval);
  }

  return retval;
}

void text_line_t::paint_line(t3window::window_t *win, const text_line_t::paint_info_t &info) {
  int tabspaces, endchars = 0;
  bool _is_print, new_is_print;
  t3_attr_t selection_attr = 0, new_selection_attr;
  int flags = info.flags;
  text_pos_t total = 0, size = info.size;

  if (info.tabsize == 0) {
    flags |= text_line_t::TAB_AS_CONTROL;
  }

  size += info.leftcol;
  if (flags & text_line_t::BREAK) {
    size--;
  }

  if (size < 0) {
    return;
  }

  if (starts_with_combining && info.leftcol > 0 && info.start == 0) {
    total++;
  }

  text_pos_t i;
  for (i = info.start;
       static_cast<size_t>(i) < buffer.size() && i < info.max && total < info.leftcol;
       i += byte_width_from_first(i)) {
    if (width_at(i) != 0) {
      selection_attr = get_draw_attrs(i, info);
    }

    if (buffer[i] == '\t' && !(flags & text_line_t::TAB_AS_CONTROL)) {
      tabspaces = info.tabsize - (total % info.tabsize);
      total += tabspaces;
      if (total >= size) {
        total = size;
      }
      if (total > info.leftcol) {
        if (flags & text_line_t::SHOW_TABS) {
          selection_attr = t3_term_combine_attrs(selection_attr, attributes.meta_text);
          if (total - info.leftcol > 1) {
            win->addnstr(dashes, (total - info.leftcol - 1), selection_attr);
          }
          win->addch('>', selection_attr);
        } else {
          win->addnstr(spaces, (total - info.leftcol), selection_attr);
        }
      }
    } else if (static_cast<unsigned char>(buffer[i]) < 32) {
      total += 2;
      // If total > info.leftcol than only the right side character is visible
      if (total > info.leftcol) {
        win->addch(control_map[static_cast<int>(buffer[i])],
                   t3_term_combine_attrs(attributes.non_print, selection_attr));
      }
    } else if (width_at(i) > 1) {
      total += width_at(i);
      if (total > info.leftcol) {
        for (text_pos_t j = info.leftcol; j < total; j++) {
          win->addch('<', t3_term_combine_attrs(attributes.non_print, selection_attr));
        }
      }
    } else {
      total += width_at(i);
    }
  }

  text_pos_t print_from, accumulated = 0;
  if (starts_with_combining && info.leftcol == 0 && info.start == 0) {
    selection_attr = get_draw_attrs(0, info);
    paint_part(win, " ", 1, true, t3_term_combine_attrs(attributes.non_print, selection_attr));

    print_from = i;

    /* Find the first non-zero-width char, and paint all zero-width chars now. */
    while (static_cast<size_t>(i) < buffer.size() && i < info.max && width_at(i) == 0) {
      i += byte_width_from_first(i);
    }

    /* Note that non-printable characters will be discarded by libt3window. Thus
       we don't have to filter for them here. */
    paint_part(win, buffer.data() + print_from, i - print_from, true,
               t3_term_combine_attrs(attributes.non_print, selection_attr));
    total++;
  } else {
    /* Skip to first non-zero-width char */
    while (static_cast<size_t>(i) < buffer.size() && i < info.max && width_at(i) == 0) {
      i += byte_width_from_first(i);
    }
  }

  _is_print = is_print(i);
  print_from = i;
  new_selection_attr = selection_attr;
  for (; static_cast<size_t>(i) < buffer.size() && i < info.max && total + accumulated < size;
       i += byte_width_from_first(i)) {
    if (width_at(i) != 0) {
      new_selection_attr = get_draw_attrs(i, info);
    }

    /* If selection changed between this char and the previous, print what
       we had so far. */
    if (new_selection_attr != selection_attr) {
      paint_part(win, buffer.data() + print_from, _is_print ? i - print_from : accumulated,
                 _is_print, selection_attr);
      total += accumulated;
      accumulated = 0;
      print_from = i;
    }
    selection_attr = new_selection_attr;

    new_is_print = is_print(i);
    if (buffer[i] == '\t' && !(flags & text_line_t::TAB_AS_CONTROL)) {
      /* Calculate the correct number of spaces for a tab character. */
      paint_part(win, buffer.data() + print_from, _is_print ? i - print_from : accumulated,
                 _is_print, selection_attr);
      total += accumulated;
      accumulated = 0;
      tabspaces = info.tabsize - (total % info.tabsize);
      /*if (total + tabspaces >= size)
              tabspaces = size - total;*/
      if (i == info.cursor) {
        char cursor_char = (flags & text_line_t::SHOW_TABS) ? (tabspaces > 1 ? '<' : '>') : ' ';
        win->addch(cursor_char, selection_attr);
        tabspaces--;
        total++;
        selection_attr = i >= info.selection_start && i < info.selection_end ? info.selected_attr
                                                                             : info.normal_attr;
      }
      if (tabspaces > 0) {
        if (flags & text_line_t::SHOW_TABS) {
          selection_attr = t3_term_combine_attrs(selection_attr, attributes.meta_text);
          if (tabspaces > 1) {
            win->addch(i == info.cursor ? '-' : '<', selection_attr);
          }
          if (tabspaces > 2) {
            win->addnstr(dashes, tabspaces - 2, selection_attr);
          }
          win->addch('>', selection_attr);
        } else {
          win->addnstr(spaces, tabspaces, selection_attr);
        }
      }
      total += tabspaces;
      print_from = i + 1;
    } else if (static_cast<unsigned char>(buffer[i]) < 32) {
      /* Print control characters as ^ followed by a letter indicating the control char. */
      paint_part(win, buffer.data() + print_from, _is_print ? i - print_from : accumulated,
                 _is_print, selection_attr);
      total += accumulated;
      accumulated = 0;
      win->addch('^', t3_term_combine_attrs(attributes.non_print, selection_attr));
      total += 2;
      if (total <= size) {
        win->addch(control_map[static_cast<int>(buffer[i])],
                   t3_term_combine_attrs(attributes.non_print, selection_attr));
      }
      print_from = i + 1;
    } else if (_is_print != new_is_print) {
      /* Print part of the buffer as either printable or control characters, because
         the next character is in the other category. */
      paint_part(win, buffer.data() + print_from, _is_print ? i - print_from : accumulated,
                 _is_print, selection_attr);
      total += accumulated;
      accumulated = width_at(i);
      print_from = i;
    } else {
      /* Take care of double width characters that cross the right screen edge. */
      if (total + accumulated + width_at(i) > size) {
        endchars = (size - total + accumulated);
        break;
      }
      accumulated += width_at(i);
    }
    _is_print = new_is_print;
  }
  while (static_cast<size_t>(i) < buffer.size() && i < info.max && width_at(i) == 0) {
    i += byte_width_from_first(i);
  }

  paint_part(win, buffer.data() + print_from, _is_print ? i - print_from : accumulated, _is_print,
             selection_attr);
  total += accumulated;

  if ((flags & text_line_t::PARTIAL_CHAR) && i >= info.max) {
    endchars = 1;
  }

  for (int j = 0; j < endchars; j++) {
    win->addch('>', t3_term_combine_attrs(attributes.non_print, selection_attr));
  }
  total += endchars;

  /* Add a selected space when the selection crosses the end of this line
     and this line is not merely a part of a broken line */
  if (total < size && !(flags & text_line_t::BREAK)) {
    if (i <= info.selection_end || i == info.cursor) {
      win->addch(' ', get_draw_attrs(i, info));
      total++;
    }
  }

  if (flags & text_line_t::BREAK) {
    for (; total < size; total++) {
      win->addch(' ', info.normal_attr);
    }
    win->addstr(wrap_symbol, t3_term_combine_attrs(attributes.meta_text, info.normal_attr));
  } else if (flags & text_line_t::SPACECLEAR) {
    for (; total + sizeof(spaces) < static_cast<size_t>(size); total += sizeof(spaces)) {
      win->addnstr(spaces, sizeof(spaces),
                   (flags & text_line_t::EXTEND_SELECTION) ? info.selected_attr : info.normal_attr);
    }
    win->addnstr(spaces, (size - total),
                 (flags & text_line_t::EXTEND_SELECTION) ? info.selected_attr : info.normal_attr);
  } else {
    win->clrtoeol();
  }
}

/* FIXME:
Several things need to be done:
  - check what to do with no-break space etc. (especially zero-width no-break
        space and zero-width space)
  - should control characters break both before and after?
*/
/* tabsize == 0 -> tab as control */
text_line_t::break_pos_t text_line_t::find_next_break_pos(text_pos_t start, text_pos_t length,
                                                          int tabsize) const {
  text_pos_t i, total = 0;
  break_pos_t possible_break = {start, 0};
  bool graph_seen = false, last_was_graph = false;

  if (starts_with_combining && start == 0) {
    total++;
  }

  for (i = start; static_cast<size_t>(i) < buffer.size() && total < length;
       i = adjust_position(i, 1)) {
    if (buffer[i] == '\t') {
      total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
    } else {
      total += width_at(i);
    }

    if (total > length) {
      if (possible_break.pos == start) {
        possible_break.flags = text_line_t::PARTIAL_CHAR;
      }
      break;
    }

    int cclass = get_class(buffer, i);
    if (buffer[i] < 32 && (buffer[i] != '\t' || tabsize == 0)) {
      cclass = CLASS_GRAPH;
    }

    if (!graph_seen) {
      if (cclass == CLASS_ALNUM || cclass == CLASS_GRAPH) {
        graph_seen = true;
        last_was_graph = true;
      }
      possible_break.pos = i;
    } else if (cclass == CLASS_WHITESPACE && last_was_graph) {
      possible_break.pos = adjust_position(i, 1);
      last_was_graph = false;
    } else if (cclass == CLASS_ALNUM || cclass == CLASS_GRAPH) {
      last_was_graph = true;
    }
  }

  if (static_cast<size_t>(i) < buffer.size()) {
    if (possible_break.pos == start) {
      possible_break.pos = i;
    }
    possible_break.flags |= text_line_t::BREAK;
    return possible_break;
  }
  possible_break.flags = 0;
  possible_break.pos = -1;
  return possible_break;
}

text_pos_t text_line_t::get_next_word(text_pos_t start) const {
  text_pos_t i;
  int cclass, newCclass;

  if (start < 0) {
    start = 0;
    cclass = CLASS_WHITESPACE;
  } else {
    cclass = get_class(buffer, start);
    start = adjust_position(start, 1);
  }

  for (i = start; static_cast<size_t>(i) < buffer.size() &&
                  ((newCclass = get_class(buffer, i)) == cclass || newCclass == CLASS_WHITESPACE);
       i = adjust_position(i, 1)) {
    cclass = newCclass;
  }

  return static_cast<size_t>(i) >= buffer.size() ? -1 : i;
}

text_pos_t text_line_t::get_previous_word(text_pos_t start) const {
  if (start == 0) {
    return -1;
  }

  if (start < 0) {
    start = buffer.size();
  }

  text_pos_t i;
  int cclass = CLASS_WHITESPACE;
  for (i = adjust_position(start, -1); i > 0 && (cclass = get_class(buffer, i)) == CLASS_WHITESPACE;
       i = adjust_position(i, -1)) {
  }

  if (i == 0 && cclass == CLASS_WHITESPACE) {
    return -1;
  }

  text_pos_t savePos = i;

  for (i = adjust_position(i, -1); i > 0 && get_class(buffer, i) == cclass;
       i = adjust_position(i, -1)) {
    savePos = i;
  }

  if (i == 0 && get_class(buffer, i) == cclass) {
    savePos = i;
  }

  return cclass != CLASS_WHITESPACE ? savePos : -1;
}

text_pos_t text_line_t::get_next_word_boundary(text_pos_t start) const {
  int cclass = get_class(buffer, start);

  text_pos_t i;
  for (i = adjust_position(start, 1);
       static_cast<size_t>(i) < buffer.size() && get_class(buffer, i) == cclass;
       i = adjust_position(i, 1)) {
  }

  return i;
}

text_pos_t text_line_t::get_previous_word_boundary(text_pos_t start) const {
  if (start <= 0) {
    return 0;
  }

  int cclass = get_class(buffer, start);
  text_pos_t savePos = start;

  text_pos_t i;
  for (i = adjust_position(start, -1); i > 0 && get_class(buffer, i) == cclass;
       i = adjust_position(i, -1)) {
    savePos = i;
  }

  if (i == 0 && get_class(buffer, i) == cclass) {
    return 0;
  }

  return savePos;
}

/* Insert character 'c' into 'line' at position 'pos' */
bool text_line_t::insert_char(text_pos_t pos, key_t c, undo_t *undo) {
  char conversion_buffer[5];
  int conversion_length;

  conversion_length = t3_utf8_put(c, conversion_buffer);

  reserve(buffer.size() + conversion_length + 1);

  if (undo != nullptr) {
    tiny_string_t *undo_text = undo->get_text();
    undo_text->reserve(undo_text->size() + conversion_length);

    ASSERT(undo->get_type() == UNDO_ADD);
    undo_text->append(string_view(conversion_buffer, conversion_length));
  }

  if (pos == 0) {
    starts_with_combining = key_width(c) == 0;
  }

  buffer.insert(pos, conversion_buffer, conversion_length);
  return true;
}

/* Overwrite a character with 'c' in 'line' at position 'pos' */
bool text_line_t::overwrite_char(text_pos_t pos, key_t c, undo_t *undo) {
  text_pos_t oldspace;
  char conversion_buffer[5];
  size_t conversion_length;

  conversion_length = t3_utf8_put(c, conversion_buffer);

  /* Zero-width characters don't overwrite, only insert. */
  if (key_width(c) == 0) {
    // FIXME: shouldn't this simply insert and set starts_with_combining to true?
    if (pos == 0) {
      return false;
    }

    if (undo != nullptr) {
      ASSERT(undo->get_type() == UNDO_OVERWRITE);
      double_string_adapter_t undo_adapter(undo->get_text());
      undo_adapter.append_second(string_view(conversion_buffer, conversion_length));
    }
    return insert_char(pos, c, nullptr);
  }

  if (starts_with_combining && pos == 0) {
    starts_with_combining = false;
  }

  oldspace = adjust_position(pos, 1) - pos;
  if (static_cast<size_t>(oldspace) < conversion_length) {
    reserve(buffer.size() + conversion_length - oldspace);
  }

  if (undo != nullptr) {
    ASSERT(undo->get_type() == UNDO_OVERWRITE);
    double_string_adapter_t undo_adapter(undo->get_text());
    undo_adapter.append_first(string_view(buffer.data() + pos, oldspace));
    undo_adapter.append_second(string_view(conversion_buffer, conversion_length));
  }

  buffer.replace(pos, oldspace, conversion_buffer, conversion_length);
  return true;
}

/* Delete the character at position 'pos' */
bool text_line_t::delete_char(text_pos_t pos, undo_t *undo) {
  text_pos_t oldspace;

  if (pos < 0 || static_cast<size_t>(pos) >= buffer.size()) {
    return false;
  }

  if (starts_with_combining && pos == 0) {
    starts_with_combining = false;
  }

  oldspace = adjust_position(pos, 1) - pos;

  if (undo != nullptr) {
    tiny_string_t *undo_text = undo->get_text();
    undo_text->reserve(oldspace);

    ASSERT(undo->get_type() == UNDO_DELETE || undo->get_type() == UNDO_BACKSPACE);
    undo_text->insert(undo->get_type() == UNDO_DELETE ? undo_text->size() : 0,
                      string_view(buffer.data() + pos, oldspace));
  }

  buffer.erase(pos, oldspace);
  return true;
}

/* Append character 'c' to 'line' */
bool text_line_t::append_char(key_t c, undo_t *undo) { return insert_char(buffer.size(), c, undo); }

/* Delete char at 'pos - 1' */
bool text_line_t::backspace_char(text_pos_t pos, undo_t *undo) {
  if (pos == 0) {
    return false;
  }
  return delete_char(adjust_position(pos, -1), undo);
}

/** Adjust the line position @a adjust non-zero-width characters.
    @param pos The starting position.
    @param adjust How many characters to adjust.

    This function finds the next (previous) point in the line at which the cursor could be. This
    means skipping all zero-width characters between the current position and the next
    non-zero-width character, and repeating for @a adjust times. */
text_pos_t text_line_t::adjust_position(text_pos_t pos, int adjust) const {
  if (adjust > 0) {
    for (; adjust > 0 && static_cast<size_t>(pos) < buffer.size();
         adjust -= (width_at(pos) ? 1 : 0)) {
      pos += byte_width_from_first(pos);
    }
  } else if (adjust < 0) {
    for (; adjust < 0 && pos > 0; adjust += (width_at(pos) ? 1 : 0)) {
      do {
        pos--;
      } while (pos > 0 && (buffer[pos] & 0xc0) == 0x80);
    }
  } else {
    while (pos > 0 && width_at(pos) == 0) {
      do {
        pos--;
      } while (pos > 0 && (buffer[pos] & 0xc0) == 0x80);
    }
  }
  return pos;
}

text_pos_t text_line_t::get_length() const { return buffer.size(); }

int text_line_t::byte_width_from_first(text_pos_t pos) const {
  switch (buffer[pos] & 0xF0) {
    case 0xF0:
      return 4;
    case 0xE0:
      return 3;
    case 0xC0:
    case 0xD0:
      return 2;
    default:
      return 1;
  }
}

int text_line_t::key_width(key_t key) {
  int width = t3_utf8_wcwidth(static_cast<uint32_t>(key));
  if (width < 0) {
    width = key < 32 && key != '\t' ? 2 : 1;
  }
  return width;
}

int text_line_t::width_at(text_pos_t pos) const {
  uint32_t c = t3_utf8_get(buffer.data() + pos, nullptr);
  if (is_conjoining_jamo_t(c) && pos > 0) {
    do {
      pos--;
    } while (pos > 0 && (buffer[pos] & 0xc0) == 0x80);
    c = t3_utf8_get(buffer.data() + pos, nullptr);
    if (is_conjoining_jamo_lv(c)) {
      return 0;
    }

    if (is_conjoining_jamo_v(c) && pos > 0) {
      do {
        pos--;
      } while (pos > 0 && (buffer[pos] & 0xc0) == 0x80);
      c = t3_utf8_get(buffer.data() + pos, nullptr);
      if (is_conjoining_jamo_l(c)) {
        return 0;
      }
    }
    return 1;
  } else if (is_conjoining_jamo_v(c) && pos > 0) {
    do {
      pos--;
    } while (pos > 0 && (buffer[pos] & 0xc0) == 0x80);
    c = t3_utf8_get(buffer.data() + pos, nullptr);
    return is_conjoining_jamo_l(c) ? 0 : 1;
  }
  return key_width(c);
}
bool text_line_t::is_print(text_pos_t pos) const {
  return buffer[pos] == '\t' ||
         !uc_is_general_category_withtable(t3_utf8_get(buffer.data() + pos, nullptr),
                                           T3_UTF8_CONTROL_MASK);
}
bool text_line_t::is_alnum(text_pos_t pos) const { return get_class(buffer, pos) == CLASS_ALNUM; }
bool text_line_t::is_space(text_pos_t pos) const {
  return get_class(buffer, pos) == CLASS_WHITESPACE;
}
bool text_line_t::is_bad_draw(text_pos_t pos) const {
  return !t3_term_can_draw(buffer.data() + pos, (adjust_position(pos, 1) - pos));
}

const std::string &text_line_t::get_data() const { return buffer; }

void text_line_t::init() {
  memset(spaces, ' ', sizeof(spaces));
  memset(dashes, '-', sizeof(dashes));
  memset(dots, '.', sizeof(dots));
  if (!t3_term_can_draw(wrap_symbol, strlen(wrap_symbol))) {
    wrap_symbol = "$";
  }
}

void text_line_t::reserve(text_pos_t size) { buffer.reserve(size); }

bool text_line_t::check_boundaries(text_pos_t match_start, text_pos_t match_end) const {
  return (match_start == 0 ||
          get_class(buffer, match_start) != get_class(buffer, adjust_position(match_start, -1))) &&
         (match_end == get_length() ||
          get_class(buffer, match_end) != get_class(buffer, adjust_position(match_end, 1)));
}

//============================= text_line_factory_t ========================

text_line_factory_t::text_line_factory_t() {}
text_line_factory_t::~text_line_factory_t() {}
std::unique_ptr<text_line_t> text_line_factory_t::new_text_line_t(int buffersize) {
  return make_unique<text_line_t>(buffersize, this);
}
std::unique_ptr<text_line_t> text_line_factory_t::new_text_line_t(string_view _buffer) {
  return make_unique<text_line_t>(_buffer, this);
}

}  // namespace t3widget
