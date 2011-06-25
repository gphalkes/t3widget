/* Copyright (C) 2010 G.P. Halkes
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

#include "textline.h"
#include "util.h"

namespace t3_widget {

#define TEXT_START_SIZE 32

enum undo_type_t {
	UNDO_NONE,
	UNDO_DELETE,
	UNDO_DELETE_BLOCK,
	UNDO_BACKSPACE,
	UNDO_ADD,
	UNDO_ADD_BLOCK,
	UNDO_REPLACE_BLOCK,
	UNDO_OVERWRITE,
	UNDO_DELETE_NEWLINE,
	UNDO_BACKSPACE_NEWLINE,
	UNDO_ADD_NEWLINE,

	// Types only for mapping redo to undo types
	UNDO_ADD_REDO,
	UNDO_BACKSPACE_REDO,
	UNDO_REPLACE_BLOCK_REDO,
	UNDO_OVERWRITE_REDO
};

class undo_list_t {
	private:
		undo_t *head, *tail, *current, *mark;
		bool mark_is_valid;
		bool mark_beyond_current;

	public:
		undo_list_t(void) : head(NULL), tail(NULL), current(NULL), mark(NULL), mark_is_valid(true), mark_beyond_current(false) {}
		~undo_list_t(void);
		void add(undo_t *undo);
		undo_t *back(void);
		undo_t *forward(void);
		void set_mark(void);
		bool is_at_mark(void) const;

		#ifdef DEBUG
		void dump(void);
		#endif
};

class undo_t {
	private:
		static undo_type_t redo_map[];

		undo_type_t type;
		text_coordinate_t start;

		undo_t *previous, *next;
		friend class undo_list_t;
	public:

		undo_t(undo_type_t _type, int start_line, int start_pos) : type(_type), start(start_line, start_pos), previous(NULL), next(NULL) {}
		virtual ~undo_t(void);
		undo_type_t get_type(void) const;
		undo_type_t get_redo_type(void) const;
		virtual text_coordinate_t get_start(void);
	    virtual void add_newline(void) {}
		virtual text_line_t *get_text(void);
		virtual text_line_t *get_replacement(void);
		virtual text_coordinate_t get_end(void) const;
		virtual void minimize(void) {}
		virtual text_coordinate_t get_new_end(void) const;
};

class undo_single_text_t : public undo_t {
	private:
		text_line_t text;

	public:
		undo_single_text_t(undo_type_t _type, int start_line, int start_pos) : undo_t(_type, start_line, start_pos), text(TEXT_START_SIZE) {};
	    virtual void add_newline(void);
		virtual text_line_t *get_text(void);
		virtual void minimize(void);
};

class undo_single_text_double_coord_t : public undo_single_text_t {
	private:
		text_coordinate_t end;

	public:
		undo_single_text_double_coord_t(undo_type_t _type, int start_line, int start_pos, int end_line, int end_pos) :
			undo_single_text_t(_type, start_line, start_pos), end(end_line, end_pos) {}
		virtual text_coordinate_t get_end(void) const;
};

class undo_double_text_t : public undo_single_text_double_coord_t {
	private:
		text_line_t replacement;

	public:
		undo_double_text_t(undo_type_t _type, int start_line, int start_pos, int end_line, int end_pos) :
			undo_single_text_double_coord_t(_type, start_line, start_pos, end_line, end_pos), replacement(TEXT_START_SIZE) {}

		virtual text_line_t *get_replacement(void);
		virtual void minimize(void);
};

class undo_double_text_triple_coord_t : public undo_double_text_t {
	private:
		text_coordinate_t new_end;

	public:
		undo_double_text_triple_coord_t(undo_type_t _type, int start_line, int start_pos, int end_line, int end_pos) :
			undo_double_text_t(_type, start_line, start_pos, end_line, end_pos) {}

		void setNewEnd(text_coordinate_t _newEnd);
		virtual text_coordinate_t get_new_end(void) const;
};

}; // namespace
#endif
