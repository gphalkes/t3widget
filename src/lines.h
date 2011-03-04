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
#ifndef EDIT_LINES_H
#define EDIT_LINES_H

#define BUFFERSIZE 64
#define BUFFERINC 16

#include <sys/types.h>
#include <stdio.h>
#include <string>
#include <window.h>
#include <pcre.h>

#include "keys.h"
#include "casefold.h"
#include "stringmatcher.h"

class undo_t;

#define MAX_TAB 80

struct break_pos_t {
	int pos;
	int flags;
};

struct find_result_t {
	int start, end;
};

class line_t;

struct find_context_t {
	int flags, original_flags;

	string_matcher_t *matcher;

	// PCRE context and data
	pcre *regex;
	int ovector[30];
	int captures;
	int pattern_length;
	bool found;
	line_t *replacement;
};

class line_t {
	public:
		enum {
			BREAK = (1<<0),
			PARTIAL_CHAR = (1<<1),
			SPACECLEAR = (1<<2),
			TAB_AS_CONTROL = (1<<3),
			EXTEND_SELECTION = (1<<4),
			DEBUG_LINE = (1<<5)
		};

		struct paint_info_t {
			int start;	// Byte position of the start of the line (0 unless line wrapping is in effect)
			int leftcol; // First cell to draw

			int max; // Last position of the line (INT_MAX unless line wrapping is in effect)
			int size; // Maximum number of characters to draw

			int tabsize; // Length of a tab, or 0 to force TAB_AS_CONTROL flag
			int flags; // See above for valid flags
			int selection_start; // Start of selected text in bytes
			int selection_end; // End of selected text in bytes
			int cursor; // Location of cursor in bytes
			t3_attr_t normal_attr, selected_attr; // Attributes to be used for normal an selected texts
			//string highlighting;
		};

	private:
		std::string buffer, meta_buffer;
		bool starts_with_combining;

		static char spaces[MAX_TAB];
		static char dots[16];
		static const char *control_map;
		static const char *wrap_symbol;

		static char conversion_buffer[5];
		static int conversion_length;
		static char conversion_meta_data;

		static void paint_part(t3_window_t *win, const char *paint_buffer, bool is_print, int todo, t3_attr_t selection_attr);

		t3_attr_t get_draw_attrs(int i, const line_t::paint_info_t *info) const;

		void fill_line(const char *_buffer, int length);
		bool check_boundaries(int match_start, int match_end) const;

	public:
		line_t(int buffersize = BUFFERSIZE);
		line_t(const char *_buffer, int length = -1);
		line_t(const std::string *str);

		void merge(line_t *other);
		line_t *break_line(int pos);
		line_t *cut_line(int start, int end);
		line_t *clone(int start, int end);
		line_t *break_on_nl(int *start_from);

		void minimize(void);

		int calculate_screen_width(int start, int pos, int tabsize) const;
		int calculate_line_pos(int start, int max, int pos, int tabsize) const;

		void paint_line(t3_window_t *win, const paint_info_t *info) const;

		break_pos_t findNextBreakPos(int start, int length, int tabsize) const;
		int get_next_word(int start) const;
		int get_previous_word(int start) const;

		bool insert_char(int pos, key_t c, undo_t *undo);
		bool overwrite_char(int pos, key_t c, undo_t *undo);
		int delete_char(int pos, undo_t *undo);
		bool append_char(key_t c, undo_t *undo);
		int backspace_char(int pos, undo_t *undo);

		int adjust_position(int pos, int adjust) const;

		int get_length(void) const;
		int width_at(int pos) const;
		int is_print(int pos) const;
		int is_graph(int pos) const;
		int is_alnum(int pos) const;
		int is_space(int pos) const;
		int is_bad_draw(int pos) const;

		const std::string *get_data(void);
		bool find(find_context_t *context, find_result_t *result) const;

		static void init(void);

	private:
		int get_class(int pos) const;
		void insert_bytes(int pos, const char *bytes, int space, char meta_data);
		void reserve(int size);
		int byte_width_from_first(int pos) const;

		static void convert_key(key_t c);

		static int callout(pcre_callout_block *block);

		friend line_t *case_fold_t::fold(const line_t *line, int *pos);

		void check_bad_draw(int i);
};

#endif
