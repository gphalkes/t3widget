/* Copyright (C) 2011,2018 G.P. Halkes
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
#ifndef T3_WIDGET_UNDO_H
#define T3_WIDGET_UNDO_H

#include <string>
#include <t3widget/textline.h>
#include <t3widget/tinystring.h>
#include <t3widget/util.h>

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

namespace t3widget {

enum undo_type_t {
  UNDO_NONE,
  UNDO_DELETE,
  UNDO_BACKSPACE,
  UNDO_ADD,
  UNDO_OVERWRITE,
  UNDO_INDENT,
  UNDO_UNINDENT,
  /* Markers for blocks of undo operations. All operations between a UNDO_BLOCK_START and
     UNDO_BLOCK_END
     are to be applied as a single operation. */
  UNDO_BLOCK_START,
  UNDO_BLOCK_END,

  // Types only for mapping redo to undo types
  UNDO_ADD_REDO,
  UNDO_BACKSPACE_REDO,
  UNDO_OVERWRITE_REDO,
  UNDO_BLOCK_START_REDO,
  UNDO_BLOCK_END_REDO,
};

class T3_WIDGET_API undo_list_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  pimpl_t<implementation_t> impl;

 public:
  undo_list_t();
  ~undo_list_t();
  undo_t *add(undo_type_t type, text_coordinate_t coord);
  undo_t *back();
  undo_t *forward();
  void set_mark();
  bool is_at_mark() const;

#ifdef DEBUG
  void dump();
#endif
};

class T3_WIDGET_API undo_t {
 private:
  static undo_type_t redo_map[];

  tiny_string_t text;
  text_coordinate_t start;
  undo_type_t type;

 public:
  undo_t(undo_type_t _type, text_coordinate_t _start) : start(_start), type(_type) {}
  undo_type_t get_type() const;
  undo_type_t get_redo_type() const;
  text_coordinate_t get_start();
  void add_newline();
  tiny_string_t *get_text();
  void minimize();
};

}  // namespace t3widget
#endif
