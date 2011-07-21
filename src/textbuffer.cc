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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pcre.h>

#include <new>
#include <t3window/window.h>
#include <t3unicode/unicode.h>

#include "textbuffer.h"
#include "textline.h"
#include "colorscheme.h"
#include "util.h"
#include "undo.h"
#include "widgets/editwindow.h"
#include "internal.h"
#include "findcontext.h"
#include "wrapinfo.h"

using namespace std;
namespace t3_widget {
/*FIXME-REFACTOR: adjust_position in line is often called with same argument as
  where the return value is stored. Check whether this is always the case. If
  so, refactor to pass pointer to first arg.

  get_line_max may be refactorable to use the cursor, depending on use outside
  files.cc

  set_selection_start/set_selection_end need refactoring. However, there should also
  be a reset_selection call such that we can avoid any arguments.
*/

/* Free all memory used by 'text' */
text_buffer_t::~text_buffer_t(void) {
    int i;

    /* Free all the text_line_t structs */
    for (i = 0; (size_t) i < lines.size(); i++)
		delete lines[i];
	free(name);
}

text_buffer_t::text_buffer_t(const char *_name) : name(NULL) {
	if (_name != NULL) {
		if ((name = strdup(_name)) == NULL)
			throw bad_alloc();
	}
	/* Allocate a new, empty line */
	lines.push_back(new text_line_t());

	selection_start.pos = -1;
	selection_start.line = 0;
	selection_end.pos = -1;
	selection_end.line = 0;
	cursor.pos = 0;
	cursor.line = 0;
	ins_mode = 0;
	last_set_pos = 0;
	selection_mode = selection_mode_t::NONE;
	last_undo = NULL;
	last_undo_type = UNDO_NONE;
}

int text_buffer_t::size(void) const {
	return lines.size();
}
bool text_buffer_t::insert_char(key_t c) {
	if (!lines[cursor.line]->insert_char(cursor.pos, c, get_undo(UNDO_ADD)))
		return false;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
	last_undo_position = cursor;

	return true;
}

bool text_buffer_t::overwrite_char(key_t c) {
	if (!lines[cursor.line]->overwrite_char(cursor.pos, c, get_undo(UNDO_OVERWRITE)))
		return false;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
	last_undo_position = cursor;

	return true;
}

bool text_buffer_t::delete_char(void) {
	if (!lines[cursor.line]->delete_char(cursor.pos, get_undo(UNDO_DELETE)))
		return false;
	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);
	return true;
}

bool text_buffer_t::backspace_char(void) {
	int newpos;

	newpos = lines[cursor.line]->adjust_position(cursor.pos, -1);
	if (!lines[cursor.line]->backspace_char(cursor.pos, get_undo(UNDO_BACKSPACE)))
		return false;
	cursor.pos = newpos;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	last_undo_position = cursor;
	return true;
}

bool text_buffer_t::is_modified(void) const {
	return !undo_list.is_at_mark();
}

bool text_buffer_t::merge_internal(int line) {
	cursor.line = line;
	cursor.pos = lines[line]->get_length();
	lines[line]->merge(lines[line + 1]);
	lines.erase(lines.begin() + line + 1);
	rewrap_required(rewrap_type_t::DELETE_LINES, line + 1, line + 2);
	rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
	return true;
}

bool text_buffer_t::merge(bool backspace) {
	if (backspace) {
		get_undo(UNDO_BACKSPACE_NEWLINE, cursor.line - 1, lines[cursor.line - 1]->get_length());
		return merge_internal(cursor.line - 1);
	} else {
		get_undo(UNDO_DELETE_NEWLINE, cursor.line, lines[cursor.line]->get_length());
		return merge_internal(cursor.line);
	}
}

bool text_buffer_t::append_text(const char *text) {
	return append_text(text, strlen(text));
}

bool text_buffer_t::append_text(const char *text, size_t size) {
	text_coordinate_t at(lines.size() - 1, INT_MAX);
	text_line_t *append = new text_line_t(text, size);
	return insert_block_internal(at, append);
}

bool text_buffer_t::append_text(const string *text) {
	return append_text(text->data(), text->size());
}

bool text_buffer_t::break_line_internal(void) {
	text_line_t *insert;

	insert = lines[cursor.line]->break_line(cursor.pos);
	lines.insert(lines.begin() + cursor.line + 1, insert);
	rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
	rewrap_required(rewrap_type_t::INSERT_LINES, cursor.line + 1, cursor.line + 2);
	cursor.line++;
	cursor.pos = 0;
	return true;
}

bool text_buffer_t::break_line(void) {
	get_undo(UNDO_ADD_NEWLINE);
	return break_line_internal();
}

int text_buffer_t::calculate_screen_pos(const text_coordinate_t *where, int tabsize) const {
	if (where == NULL)
		where = &cursor;
	return lines[where->line]->calculate_screen_width(0, where->pos, tabsize);
}

int text_buffer_t::calculate_line_pos(int line, int pos, int tabsize) const {
	return lines[line]->calculate_line_pos(0, INT_MAX, pos, tabsize);
}

void text_buffer_t::paint_line(t3_window_t *win, int line, text_line_t::paint_info_t *info) const {
	info->start = 0;
	info->max = INT_MAX;
	info->flags = 0;
	lines[line]->paint_line(win, info);
}

int text_buffer_t::get_line_max(int line) const {
	return lines[line]->get_length();
}

void text_buffer_t::get_next_word(void) {
	text_line_t *line;

	line = lines[cursor.line];

	/* Use -1 as an indicator for end of line */
	if (cursor.pos >= line->get_length())
		cursor.pos = -1;
	else if (cursor.pos >= 0)
		cursor.pos = line->get_next_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0) {
		if ((size_t) cursor.line + 1 >= lines.size())
			break;
		line = lines[++cursor.line];
		cursor.pos = line->get_next_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = line->get_length();
}

void text_buffer_t::get_previous_word(void) {
	text_line_t *line;

	line = lines[cursor.line];

	cursor.pos = line->get_previous_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0 && cursor.line > 0) {
		line = lines[--cursor.line];
		cursor.pos = line->get_previous_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = 0;
}

void text_buffer_t::adjust_position(int adjust) {
	cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, adjust);
}

int text_buffer_t::width_at_cursor(void) const {
	return lines[cursor.line]->width_at(cursor.pos);
}

bool text_buffer_t::selection_empty(void) const {
	return selection_start.line == selection_end.line && selection_start.pos == selection_end.pos;
}

void text_buffer_t::set_selection_end(void)  {
	selection_end = cursor;
}

text_coordinate_t text_buffer_t::get_selection_start(void) const { return selection_start; }
text_coordinate_t text_buffer_t::get_selection_end(void) const { return selection_end; }

void text_buffer_t::delete_block(text_coordinate_t start, text_coordinate_t end, undo_t *undo) {
	text_line_t *start_part = NULL, *end_part = NULL;
	int i;

	/* Don't do anything on empty selection */
	if (start.line == end.line && start.pos == end.pos)
		return;

	/* Swap start and end if end is before start. This simplifies the code below. */
	if ((end.line < start.line) ||
			(end.line == start.line && end.pos < start.pos)) {
		text_coordinate_t tmp = start;
		start = end;
		end = tmp;
	}

	if (start.line == end.line) {
		text_line_t *selected_text;
		selected_text = lines[start.line]->cut_line(start.pos, end.pos);
		if (undo != NULL)
			undo->get_text()->merge(selected_text);
		else
			delete selected_text;
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

	if (start.pos == lines[start.line]->get_length()) {
		start_part = lines[start.line];
	} else if (start.pos != 0) {
		text_line_t *retval = lines[start.line]->break_line(start.pos);
		if (undo != NULL)
			undo->get_text()->merge(retval);
		else
			delete retval;
		start_part = lines[start.line];
	}

	if (end.pos == 0)
		end_part = lines[end.line];
	else if (end.pos < lines[end.line]->get_length())
		end_part = lines[end.line]->break_line(end.pos);

	if (start_part == NULL) {
		if (end_part != NULL) {
			if (undo != NULL)
				undo->get_text()->merge(lines[start.line]);
			else
				delete lines[start.line];
			lines[start.line] = end_part;
		} else {
			if (undo != NULL)
				undo->get_text()->merge(lines[start.line]);
			else
				delete lines[start.line];
			lines[start.line] = new text_line_t();
		}
	} else {
		if (end_part != NULL)
			start_part->merge(end_part);
	}

	start.line++;

	if (undo != NULL) {
		undo->add_newline();

		for (i = start.line; i < end.line; i++) {
			undo->get_text()->merge(lines[i]);
			undo->add_newline();
		}

		if (end.pos != 0)
			undo->get_text()->merge(lines[end.line]);
	} else {
		for (i = start.line; i < end.line; i++)
			delete lines[i];
	}

	end.line++;

	lines.erase(lines.begin() + start.line, lines.begin() + end.line);
	rewrap_required(rewrap_type_t::DELETE_LINES, start.line, end.line);
	rewrap_required(rewrap_type_t::REWRAP_LINE, start.line - 1, start.pos);
	if ((size_t) start.line < lines.size())
		rewrap_required(rewrap_type_t::REWRAP_LINE, start.line, 0);
}


void text_buffer_t::delete_selection(void) {
	/* Don't do anything on empty selection */
	if (selection_start.line == selection_end.line && selection_start.pos == selection_end.pos)
		return;

	last_undo = new undo_single_text_double_coord_t(UNDO_DELETE_BLOCK, selection_start.line,
			selection_start.pos, selection_end.line, selection_end.pos);
	last_undo_type = UNDO_DELETE_BLOCK;

	delete_block(selection_start, selection_end, last_undo);

	undo_list.add(last_undo);
}

bool text_buffer_t::insert_block_internal(text_coordinate_t insert_at, text_line_t *block) {
	text_line_t *second_half = NULL, *next_line;
	int next_start = 0;
//FIXME: check that everything succeeds and return false if it doesn't
	if (insert_at.pos >= 0 && insert_at.pos < lines[insert_at.line]->get_length())
		second_half = lines[insert_at.line]->break_line(insert_at.pos);

	next_line = block->break_on_nl(&next_start);

	lines[insert_at.line]->merge(next_line);
	rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, insert_at.pos);

	while (next_start > 0) {
		insert_at.line++;
		next_line = block->break_on_nl(&next_start);
		lines.insert(lines.begin() + insert_at.line, next_line);
		rewrap_required(rewrap_type_t::INSERT_LINES, insert_at.line, insert_at.line + 1);
	}

	cursor.pos = lines[insert_at.line]->get_length();

	if (second_half != NULL) {
		lines[insert_at.line]->merge(second_half);
		rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, cursor.pos);
	}

	cursor.line = insert_at.line;
	return true;
}

bool text_buffer_t::insert_block(text_line_t *block) {
	text_coordinate_t cursor_at_start = cursor;

	if (!insert_block_internal(cursor, block))
		return false;

	last_undo = new undo_single_text_double_coord_t(UNDO_ADD_BLOCK, cursor_at_start.line, cursor_at_start.pos, cursor.line, cursor.pos);
	last_undo_type = UNDO_ADD_BLOCK;
	//FIXME: clone may return NULL!
	last_undo->get_text()->merge(block->clone(0, -1));
	undo_list.add(last_undo);
	return true;
}

bool text_buffer_t::replace_selection(text_line_t *block) {
	text_coordinate_t current_start, current_end;
	undo_double_text_triple_coord_t *undo;

	current_start = get_selection_start();
	current_end = get_selection_end();

//FIXME: check that everything succeeds and return false if it doesn't
	//FIXME: make sure original state is restored on failing sub action
	/* Simply insert on empty selection */
	if (current_start.line == current_end.line && current_start.pos == current_end.pos) {
		return insert_block(block);
	}

	last_undo = undo = new undo_double_text_triple_coord_t(UNDO_REPLACE_BLOCK, current_start.line,
			current_start.pos, current_end.line, current_end.pos);
	last_undo_type = UNDO_REPLACE_BLOCK;

	delete_block(current_start, current_end, undo);

	if (current_end.line < current_start.line || (current_end.line == current_start.line && current_end.pos < current_start.pos))
		current_start = current_end;

	insert_block_internal(current_start, block);

	undo->get_replacement()->merge(block->clone(0, -1));
	undo->setNewEnd(cursor);

	undo_list.add(undo);
	return true;
}

text_line_t *text_buffer_t::convert_selection(void) {
	text_coordinate_t current_start, current_end;
	text_line_t *retval;
	int i;

	current_start = get_selection_start();
	current_end = get_selection_end();

	/* Don't do anything on empty selection */
	if (current_start.line == current_end.line && current_start.pos == current_end.pos)
		return NULL;

	/* Swap start and end if end is before start. This simplifies the code below. */
	if ((current_end.line < current_start.line) ||
			(current_end.line == current_start.line && current_end.pos < current_start.pos)) {
		text_coordinate_t tmp = current_start;
		current_start = current_end;
		current_end = tmp;
	}

	if (current_start.line == current_end.line)
		return lines[current_start.line]->clone(current_start.pos, current_end.pos);

	//FIXME: clone may return NULL!
	retval = lines[current_start.line]->clone(current_start.pos, -1);
	retval->append_char('\n', NULL);

	for (i = current_start.line + 1; i < current_end.line; i++) {
		//FIXME: clone may return NULL!
		retval->merge(lines[i]->clone(0, -1));
		retval->append_char('\n', NULL);
	}

	//FIXME: clone may return NULL!
	retval->merge(lines[current_end.line]->clone(0, current_end.pos));
	return retval;
}

undo_t *text_buffer_t::get_undo(undo_type_t type) {
	return get_undo(type, cursor.line, cursor.pos);
}

undo_t *text_buffer_t::get_undo(undo_type_t type, int line, int pos) {
	ASSERT(type != UNDO_ADD_BLOCK && type != UNDO_DELETE_BLOCK);

	if (last_undo_type == type && last_undo_position.line == line &&
			last_undo_position.pos == pos && last_undo != NULL)
		return last_undo;

	if (last_undo != NULL)
		last_undo->minimize();

	switch (type) {
		case UNDO_ADD_NEWLINE:
		case UNDO_DELETE_NEWLINE:
		case UNDO_BACKSPACE_NEWLINE:
			last_undo = new undo_t(type, line, pos);
			break;
		case UNDO_DELETE:
		case UNDO_ADD:
		case UNDO_BACKSPACE:
			last_undo = new undo_single_text_t(type, line, pos);
			break;
		case UNDO_OVERWRITE:
			//FIXME: what to do with the last arguments?
			last_undo = new undo_double_text_t(type, line, pos, -1, -1);
			break;
		default:
			ASSERT(0);
	}
	last_undo_position.line = line;
	last_undo_position.pos = pos;

	undo_list.add(last_undo);
	if (type == UNDO_ADD_NEWLINE || type == UNDO_DELETE_NEWLINE)
		last_undo_type = UNDO_NONE;
	else
		last_undo_type = type;
	return last_undo;
}


int text_buffer_t::apply_undo_redo(undo_type_t type, undo_t *current) {
	text_coordinate_t start, end;

	switch (type) {
		case UNDO_ADD:
			end = start = current->get_start();
			end.pos += current->get_text()->get_length();
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_ADD_REDO:
		case UNDO_DELETE:
			start = current->get_start();
			insert_block_internal(start, current->get_text());
			if (type == UNDO_DELETE)
				cursor = start;
			break;
		case UNDO_ADD_BLOCK:
			start = current->get_start();
			end = current->get_end();
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_DELETE_BLOCK:
			start = current->get_start();
			end = current->get_end();
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			insert_block_internal(start, current->get_text());
			cursor = end;
			break;
		case UNDO_BACKSPACE:
			start = current->get_start();
			start.pos -= current->get_text()->get_length();
			insert_block_internal(start, current->get_text());
			break;
		case UNDO_BACKSPACE_REDO:
			end = start = current->get_start();
			start.pos -= current->get_text()->get_length();
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_OVERWRITE:
			end = start = current->get_start();
			end.pos += current->get_replacement()->get_length();
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_text());
			cursor = start;
			break;
		case UNDO_OVERWRITE_REDO:
			end = start = current->get_start();
			end.pos += current->get_text()->get_length();
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_replacement());
			break;
		case UNDO_REPLACE_BLOCK:
			start = current->get_start();
			end = current->get_end();
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			end = current->get_new_end();
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_text());
			cursor = current->get_end();
			break;
		case UNDO_REPLACE_BLOCK_REDO:
			start = current->get_start();
			end = current->get_end();
			delete_block(start, end, NULL);
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			insert_block_internal(start, current->get_replacement());
			break;
		case UNDO_BACKSPACE_NEWLINE:
			cursor = current->get_start();
			break_line_internal();
			break;
		case UNDO_DELETE_NEWLINE:
			cursor = current->get_start();
			break_line_internal();
			break;
		case UNDO_ADD_NEWLINE:
			start = current->get_start();
			merge_internal(start.line);
			break;
		default:
			ASSERT(0);
			break;
	}
	return 0;
}

/*FIXME: define return values for:
	- nothing done
	- failure
	- success;
*/
int text_buffer_t::apply_undo(void) {
	undo_t *current = undo_list.back();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_type(), current);
	last_undo_type = UNDO_NONE;

	return 0;
}

int text_buffer_t::apply_redo(void) {
	undo_t *current = undo_list.forward();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_redo_type(), current);
	last_undo_type = UNDO_NONE;

	return 0;
}

void text_buffer_t::set_selection_from_find(int line, find_result_t *result) {
	selection_start.line = line;
	selection_start.pos = result->start;

	selection_end.line = line;
	selection_end.pos = result->end;

	cursor = get_selection_end();
	selection_mode = selection_mode_t::SHIFT;
}

bool text_buffer_t::find(finder_t *finder, bool reverse) {
	size_t start, idx;
	find_result_t result;

	start = idx = cursor.line;

	// Perform search
	if (((finder->get_flags() & find_flags_t::BACKWARD) != 0) ^ reverse) {
		result.start = selection_mode != selection_mode_t::NONE ? selection_start.pos : cursor.pos;
		result.end = 0;
		if (finder->match(lines[idx]->get_data(), &result, true)) {
			set_selection_from_find(idx, &result);
			return true;
		}

		result.start = INT_MAX;
		for (; idx > 0; ) {
			idx--;
			if (finder->match(lines[idx]->get_data(), &result, true)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}

		if (!(finder->get_flags() & find_flags_t::WRAP))
			return false;

		result.start = INT_MAX;
		for (idx = lines.size(); idx > start; ) {
			idx--;
			if (finder->match(lines[idx]->get_data(), &result, true)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}
	} else {
		result.start = cursor.pos;
		result.end = INT_MAX;
		if (finder->match(lines[idx]->get_data(), &result, false)) {
			set_selection_from_find(idx, &result);
			return true;
		}

		result.start = 0;
		for (idx++; idx < lines.size(); idx++) {
			if (finder->match(lines[idx]->get_data(), &result, false)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}

		if (!(finder->get_flags() & find_flags_t::WRAP))
			return false;

		for (idx = 0; idx <= start; idx++) {
			if (finder->match(lines[idx]->get_data(), &result, false)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}
	}

	return false;
}

void text_buffer_t::replace(finder_t *finder) {
	if (selection_mode == selection_mode_t::NONE)
		return;

	string *replacement_str = finder->get_replacement(lines[cursor.line]->get_data());
	text_line_t replacement(replacement_str);
	delete replacement_str;

	replace_selection(&replacement);
}

const char *text_buffer_t::get_name(void) const {
	return name;
}

void text_buffer_t::bad_draw_recheck(void) {
	for (lines_t::iterator iter = lines.begin(); iter != lines.end(); iter++)
		(*iter)->bad_draw_recheck();
	name_line.bad_draw_recheck();
}

void text_buffer_t::set_selection_mode(selection_mode_t mode) {
	switch (mode) {
		case selection_mode_t::NONE:
			selection_start = text_coordinate_t(0, -1);
			selection_end = text_coordinate_t(0, -1);
			break;
		case selection_mode_t::ALL:
			selection_start = text_coordinate_t(0, 0);
			selection_end = text_coordinate_t((lines.size() - 1), get_line_max(lines.size() - 1));
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

selection_mode_t text_buffer_t::get_selection_mode(void) const {
	return selection_mode;
}

const text_line_t *text_buffer_t::get_name_line(void) const {
	return &name_line;
}

bool text_buffer_t::indent_selection(void) {
	return true;
}

bool text_buffer_t::unindent_selection(void) {
	return true;
}

}; // namespace
