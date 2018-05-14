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
#include <t3widget/util.h>

namespace t3_widget {

/* FIXME: the undo/redo functionality should be reimplemented, composing operations from:
   DELETE, ADD, ADD_NEWLINE, DELETE_NEWLINE, BLOCK_START, BLOCK_END
   The last two should contain the cursor positions before and after the action, the others
   should only encode the minimal location information to execute the operation at the
   correct point. If the location information does not leave the cursor at the correct
   point, it should be enclosed by the BLOCK_START/BLOCK_END combination.
*/

enum undo_type_t {
  UNDO_NONE,
  UNDO_DELETE,
  UNDO_BACKSPACE,
  UNDO_ADD,
  UNDO_REPLACE_BLOCK,
  UNDO_OVERWRITE,
  UNDO_DELETE_NEWLINE,
  UNDO_BACKSPACE_NEWLINE,
  UNDO_ADD_NEWLINE,
  UNDO_INDENT,
  UNDO_UNINDENT,
  UNDO_ADD_NEWLINE_INDENT,
  /* Markers for blocks of undo operations. All operations between a UNDO_BLOCK_START and
     UNDO_BLOCK_END
     are to be applied as a single operation. */
  UNDO_BLOCK_START,
  UNDO_BLOCK_END,

  // Types only for mapping redo to undo types
  UNDO_ADD_REDO,
  UNDO_BACKSPACE_REDO,
  UNDO_REPLACE_BLOCK_REDO,
  UNDO_OVERWRITE_REDO,
  UNDO_ADD_NEWLINE_INDENT_REDO,
  UNDO_BLOCK_START_REDO,
  UNDO_BLOCK_END_REDO,
};

// FIXME: reimplement using std::list

class T3_WIDGET_API undo_list_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  pimpl_t<implementation_t> impl;

 public:
  undo_list_t();
  ~undo_list_t();
  void add(undo_t *undo);
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

  undo_type_t type;
  text_coordinate_t start;

  undo_t *previous_ = nullptr, *next_ = nullptr;
  undo_t *&next() { return next_; }
  undo_t *&prev() { return previous_; }
  template <typename T>
  friend class subclass_list_t;

 public:
  undo_t(undo_type_t _type, text_coordinate_t _start) : type(_type), start(_start) {}
  virtual ~undo_t();
  undo_type_t get_type() const;
  undo_type_t get_redo_type() const;
  virtual text_coordinate_t get_start();
  virtual void add_newline() {}
  virtual std::string *get_text();
  virtual std::string *get_replacement();
  virtual text_coordinate_t get_end() const;
  virtual void minimize() {}
  virtual text_coordinate_t get_new_end() const;
};

class T3_WIDGET_API undo_single_text_t : public undo_t {
 private:
  std::string text;

 public:
  undo_single_text_t(undo_type_t _type, text_coordinate_t _start) : undo_t(_type, _start){};
  void add_newline() override;
  std::string *get_text() override;
  void minimize() override;
};

class T3_WIDGET_API undo_single_text_double_coord_t : public undo_single_text_t {
 private:
  text_coordinate_t end;

 public:
  undo_single_text_double_coord_t(undo_type_t _type, text_coordinate_t _start,
                                  text_coordinate_t _end)
      : undo_single_text_t(_type, _start), end(_end) {}
  text_coordinate_t get_end() const override;
};

class T3_WIDGET_API undo_double_text_t : public undo_single_text_double_coord_t {
 private:
  std::string replacement;

 public:
  undo_double_text_t(undo_type_t _type, text_coordinate_t _start, text_coordinate_t _end)
      : undo_single_text_double_coord_t(_type, _start, _end) {}

  std::string *get_replacement() override;
  void minimize() override;
};

class T3_WIDGET_API undo_double_text_triple_coord_t : public undo_double_text_t {
 private:
  text_coordinate_t new_end;

 public:
  undo_double_text_triple_coord_t(undo_type_t _type, text_coordinate_t _start,
                                  text_coordinate_t _end)
      : undo_double_text_t(_type, _start, _end) {}

  void set_new_end(text_coordinate_t _new_end);
  text_coordinate_t get_new_end() const override;
};

}  // namespace
#endif
