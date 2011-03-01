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

using namespace std;

#include "keys.h"
#include "casefold.h"
#include "stringmatcher.h"

class Undo;

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
		string buffer, meta_buffer;
		bool starts_with_combining;

		static char spaces[MAX_TAB];
		static char dots[16];
		static const char *control_map;

		static char conversion_buffer[5];
		static int conversion_length;
		static char conversion_meta_data;

		static void paintpart(t3_window_t *win, const char *buffer, bool isPrint, int todo, t3_attr_t selectionAttr);

		t3_attr_t getDrawAttrs(int i, const line_t::paint_info_t *info) const;

		void fillLine(const char *_buffer, int length);
		bool checkBoundaries(int matchStart, int matchEnd) const;

	public:
		line_t(int buffersize = BUFFERSIZE);
		line_t(const char *_buffer, int length = -1); //UTF8Mode switch
		line_t(const string *str);

		void merge(line_t *other);
		line_t *break_line(int pos);
		line_t *cutLine(int start, int end);
		line_t *clone(int start, int end);
		line_t *breakOnNL(int *startFrom);

		void minimize(void);

		int calculateScreenWidth(int start, int pos, int tabsize) const;
		int calculate_line_pos(int start, int max, int pos, int tabsize) const;

		void paint_line(t3_window_t *win, const paint_info_t *info) const;

		break_pos_t findNextBreakPos(int start, int length, int tabsize) const;
		int get_next_word(int start) const;
		int get_previous_word(int start) const;

		bool insert_char(int pos, key_t c, Undo *undo);
		bool overwrite_char(int pos, key_t c, Undo *undo);
		int delete_char(int pos, Undo *undo);
		bool append_char(key_t c, Undo *undo);
		int backspace_char(int pos, Undo *undo);

		int adjust_position(int pos, int adjust) const;

		int get_length(void) const;
		int widthAt(int pos) const;
		int isPrint(int pos) const;
		int isGraph(int pos) const;
		int isAlnum(int pos) const;
		int isSpace(int pos) const;
		int isBadDraw(int pos) const;

		const string *getData(void);
		bool find(find_context_t *context, find_result_t *result) const;

		static void init(void);

	private:
		int getClass(int pos) const;
		void insertBytes(int pos, const char *bytes, int space, char meta_data);
		void reserve(int size);
		int byteWidthFromFirst(int pos) const; //UTF8Mode switch

		static void convertKey(key_t c); //UTF8Mode switch
		static void paintWrapSymbol(t3_window_t *win); //UTF8Mode switch
		static char get_s_b_c_meta_data(key_t c);

		static int callout(pcre_callout_block *block);

		friend line_t *case_fold_t::fold(const line_t *line, int *pos);

		void checkBadDraw(int i);
};

#endif
