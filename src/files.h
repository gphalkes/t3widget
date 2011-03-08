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
#ifndef T3_WIDGET_FILES_H
#define T3_WIDGET_FILES_H

#include <cstdio>
#include <vector>

#include "keys.h"
#include "lines.h"
#include "subline.h"
#include "undo.h"
#include "main.h"
#include "interfaces.h"
//#include "charactersets.h"
//#include "filestate.h"

typedef std::vector<line_t *> lines_t;
typedef std::vector<subline_t *> sublines_t;

class edit_window_t;

class text_file_t : public bullet_status_t {
	friend class edit_window_t;
	friend class OpenFiles;
	friend class RecentFiles;

	private:
		lines_t lines;
		sublines_t wraplines;
		bool wrap;
		bool file_has_bom;
		int tabsize;
		int wrap_width;
		text_coordinate_t selection_start;
		text_coordinate_t selection_end;
		undo_list_t undo_list;
		text_coordinate_t last_undo_position;
		undo_type_t last_undo_type;
		undo_t *last_undo;
		char *name;
		line_t *name_line;

		text_coordinate_t cursor, topleft;
		int ins_mode, last_set_pos;
		selection_mode_t selection_mode;
		edit_window_t *window;

		static find_context_t last_find;

		undo_t *get_undo(undo_type_t type);
		undo_t *get_undo(undo_type_t type, int line, int pos);
		bool init_wrap_lines(void);
		void locate_pos(void);
		void locate_pos(text_coordinate_t *coord) const;
		int find_line(int idx) const;
		void find_wrap_line(text_coordinate_t *coord) const;
		int get_real_line(int line) const;
		text_coordinate_t get_wrap_line(text_coordinate_t coord) const;
		int get_max(int line) const;
		bool rewrap_line(int line);
		void delete_block(text_coordinate_t start, text_coordinate_t end, undo_t *undo);
		void insert_block_internal(text_coordinate_t insertAt, line_t *block);
		int apply_undo_redo(undo_type_t type, undo_t *current);
		int merge_internal(int line);
		bool break_line_internal(void);

		/* static char *resolve_links(const char *startName);
		static char *canonicalize_path(const char *name); */

		void set_selection_from_find(int line, find_result_t *result);
	public:
		text_file_t(void);
		text_file_t(const char *_name);
		void common_init(void);
		virtual ~text_file_t(void);

/*		RWResult load(LoadState *load);
		RWResult save(SaveState *state);*/

		int get_used_lines(void) const;
		void set_tabsize(int tabsize);
		int get_tabsize(void) const;

		// API for manipulating lines through the file
		bool insert_char(key_t c);
		bool overwrite_char(key_t c);
		bool append_char(key_t c);
		int delete_char(void);
		int backspace_char(void);
		// FIXME: these functions are not used yet
		//~ int insertString(int line, int pos, char *string);
		//~ int appendString(int line, char *string);
		//~ int deleteString(int line, int pos, int n);
		//~ int searchString(int line, int pos, char *string);
		//~ int isearchString(int line, int pos, char *string);
		//~ int replaceString(int line, int pos, int n, char *string);
		int merge(bool backspace);


		bool break_line(void);
		void new_line(void);
		int calculate_screen_pos(const text_coordinate_t *where = NULL) const;
		int calculate_line_pos(int line, int pos) const;

		void paint_line(t3_window_t *win, int line, line_t::paint_info_t *info) const;
		int get_line_max(int line) const;
		void get_next_word(void);
		void get_previous_word(void);
		void get_line_info(text_coordinate_t *new_coord) const;

		void adjust_position(int adjust);
		void rewrap(void);
		bool get_wrap(void) const;
		int width_at_cursor(void) const;

		void set_selection_start(int line, int pos);
		void set_selection_end(int line, int pos);
		bool selection_empty(void) const;
		text_coordinate_t get_selection_start(void) const;
		text_coordinate_t get_selection_end(void) const;
		void delete_selection(void);
		int insert_block(line_t *block);
		void replace_selection(line_t *block);
		line_t *convert_selection(void);

		int apply_undo(void);
		int apply_redo(void);

/* 		#ifdef DEBUG
		void dump_undo(void) { undo_list.dump(); }
		#endif */

		bool is_modified(void) { return !undo_list.is_at_mark(); }

		bool find(const std::string *what, int flags, const line_t *replacement);
		void replace(void);

		virtual bool get_bullet_status(void);
		const char *get_name(void) const;
};

#endif
