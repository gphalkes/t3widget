/* Copyright (C) 2011 G.P. Halkes
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
#include <cstring>
#include <climits>
#include <new>
#include <t3window/window.h>

#include "textbuffer.h"
#include "textline.h"
#include "colorscheme.h"
#include "util.h"
#include "undo.h"
#include "internal.h"
#include "findcontext.h"
#include "wrapinfo.h"
#include "clipboard.h"

using namespace std;
namespace t3_widget {
/*FIXME-REFACTOR: adjust_position in line is often called with same argument as
  where the return value is stored. Check whether this is always the case. If
  so, refactor to pass pointer to first arg.

  get_line_max may be refactorable to use the cursor, depending on use outside
  files.cc
*/

text_buffer_t::text_buffer_t(text_line_factory_t *_line_factory) : impl(new implementation_t(_line_factory)), cursor(0, 0)
{
	/* Allocate a new, empty line */
	impl->lines.push_back(impl->line_factory->new_text_line_t());
}

text_buffer_t::~text_buffer_t(void) {
    int i;

    /* Free all the text_line_t structs */
    for (i = 0; (size_t) i < impl->lines.size(); i++)
		delete impl->lines[i];
}

int text_buffer_t::size(void) const {
	return impl->lines.size();
}

const text_line_t *text_buffer_t::get_line_data(int idx) const {
	return impl->lines[idx];
}

text_line_t *text_buffer_t::get_line_data_nonconst(int idx) {
	return impl->lines[idx];
}

text_line_factory_t *text_buffer_t::get_line_factory(void) {
	return impl->line_factory;
}

bool text_buffer_t::insert_char(key_t c) {
	if (!impl->lines[cursor.line]->insert_char(cursor.pos, c, get_undo(UNDO_ADD)))
		return false;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	cursor.pos = impl->lines[cursor.line]->adjust_position(cursor.pos, 1);
	impl->last_undo_position = cursor;

	return true;
}

bool text_buffer_t::overwrite_char(key_t c) {
	if (!impl->lines[cursor.line]->overwrite_char(cursor.pos, c, get_undo(UNDO_OVERWRITE)))
		return false;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	cursor.pos = impl->lines[cursor.line]->adjust_position(cursor.pos, 1);
	impl->last_undo_position = cursor;

	return true;
}

bool text_buffer_t::delete_char(void) {
	if (!impl->lines[cursor.line]->delete_char(cursor.pos, get_undo(UNDO_DELETE)))
		return false;
	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);
	return true;
}

bool text_buffer_t::backspace_char(void) {
	int newpos;

	newpos = impl->lines[cursor.line]->adjust_position(cursor.pos, -1);
	if (!impl->lines[cursor.line]->backspace_char(cursor.pos, get_undo(UNDO_BACKSPACE)))
		return false;
	cursor.pos = newpos;

	rewrap_required(rewrap_type_t::REWRAP_LINE_LOCAL, cursor.line, cursor.pos);

	impl->last_undo_position = cursor;
	return true;
}

bool text_buffer_t::is_modified(void) const {
	return !impl->undo_list.is_at_mark();
}

bool text_buffer_t::merge_internal(int line) {
	cursor.line = line;
	cursor.pos = impl->lines[line]->get_length();
	impl->lines[line]->merge(impl->lines[line + 1]);
	impl->lines.erase(impl->lines.begin() + line + 1);
	rewrap_required(rewrap_type_t::DELETE_LINES, line + 1, line + 2);
	rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
	return true;
}

bool text_buffer_t::merge(bool backspace) {
	if (backspace) {
		get_undo(UNDO_BACKSPACE_NEWLINE, text_coordinate_t(cursor.line - 1, impl->lines[cursor.line - 1]->get_length()));
		return merge_internal(cursor.line - 1);
	} else {
		get_undo(UNDO_DELETE_NEWLINE, text_coordinate_t(cursor.line, impl->lines[cursor.line]->get_length()));
		return merge_internal(cursor.line);
	}
}

bool text_buffer_t::append_text(const char *text) {
	return append_text(text, strlen(text));
}

bool text_buffer_t::append_text(const char *text, size_t _size) {
	bool result;
	text_coordinate_t at(impl->lines.size() - 1, INT_MAX);
	text_line_t *append = impl->line_factory->new_text_line_t(text, _size);
	result = insert_block_internal(at, append);
	delete append;
	return result;
}

bool text_buffer_t::append_text(const string *text) {
	return append_text(text->data(), text->size());
}

bool text_buffer_t::break_line_internal(const string *indent) {
	text_line_t *insert;

	insert = impl->lines[cursor.line]->break_line(cursor.pos);
	impl->lines.insert(impl->lines.begin() + cursor.line + 1, insert);
	rewrap_required(rewrap_type_t::REWRAP_LINE, cursor.line, cursor.pos);
	rewrap_required(rewrap_type_t::INSERT_LINES, cursor.line + 1, cursor.line + 2);
	cursor.line++;
	if (indent == NULL) {
		cursor.pos = 0;
	} else {
		text_line_t *new_line = impl->line_factory->new_text_line_t(indent);
		new_line->merge(impl->lines[cursor.line]);
		impl->lines[cursor.line] = new_line;
		cursor.pos = indent->size();
	}
	return true;
}

bool text_buffer_t::break_line(const string *indent) {
	if (indent == NULL) {
		get_undo(UNDO_ADD_NEWLINE);
	} else {
		undo_t *undo = get_undo(UNDO_ADD_NEWLINE_INDENT);
		undo->get_text()->append(1, '\n');
		undo->get_text()->append(*indent);
	}
	return break_line_internal(indent);
}

int text_buffer_t::calculate_screen_pos(const text_coordinate_t *where, int tabsize) const {
	if (where == NULL)
		where = &cursor;
	return impl->lines[where->line]->calculate_screen_width(0, where->pos, tabsize);
}

int text_buffer_t::calculate_line_pos(int line, int pos, int tabsize) const {
	return impl->lines[line]->calculate_line_pos(0, INT_MAX, pos, tabsize);
}

void text_buffer_t::paint_line(t3_window_t *win, int line, text_line_t::paint_info_t *info) {
	prepare_paint_line(line);
	info->start = 0;
	info->max = INT_MAX;
	info->flags &= ~text_line_t::BREAK;
	impl->lines[line]->paint_line(win, info);
}

int text_buffer_t::get_line_max(int line) const {
	return impl->lines[line]->get_length();
}

void text_buffer_t::goto_next_word(void) {
	text_line_t *line;

	line = impl->lines[cursor.line];

	/* Use -1 as an indicator for end of line */
	if (cursor.pos >= line->get_length())
		cursor.pos = -1;
	else if (cursor.pos >= 0)
		cursor.pos = line->get_next_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0) {
		if ((size_t) cursor.line + 1 >= impl->lines.size())
			break;
		line = impl->lines[++cursor.line];
		cursor.pos = line->get_next_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = line->get_length();
}

void text_buffer_t::goto_previous_word(void) {
	text_line_t *line;

	line = impl->lines[cursor.line];

	cursor.pos = line->get_previous_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0 && cursor.line > 0) {
		line = impl->lines[--cursor.line];
		cursor.pos = line->get_previous_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = 0;
}

void text_buffer_t::adjust_position(int adjust) {
	cursor.pos = impl->lines[cursor.line]->adjust_position(cursor.pos, adjust);
}

int text_buffer_t::width_at_cursor(void) const {
	return impl->lines[cursor.line]->width_at(cursor.pos);
}

bool text_buffer_t::selection_empty(void) const {
	return impl->selection_start == impl->selection_end;
}

void text_buffer_t::set_selection_end(bool update_primary)  {
	impl->selection_end = cursor;
	if (update_primary)
		set_primary(convert_selection());
}

text_coordinate_t text_buffer_t::get_selection_start(void) const { return impl->selection_start; }
text_coordinate_t text_buffer_t::get_selection_end(void) const { return impl->selection_end; }

void text_buffer_t::delete_block_internal(text_coordinate_t start, text_coordinate_t end, undo_t *undo) {
	text_line_t *start_part = NULL, *end_part = NULL;
	int i;

	/* Don't do anything on empty block */
	if (start == end)
		return;

	/* Swap start and end if end is before start. This simplifies the code below. */
	if ((end.line < start.line) ||
			(end.line == start.line && end.pos < start.pos)) {
		text_coordinate_t tmp = start;
		start = end;
		end = tmp;
	}
	cursor = start;

	if (start.line == end.line) {
		text_line_t *selected_text;
		selected_text = impl->lines[start.line]->cut_line(start.pos, end.pos);
		if (undo != NULL)
			undo->get_text()->append(*selected_text->get_data());
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
		- last line is completely emptied (end.pos == Line_getLineLength(impl->lines[end.line]))
			then no merge is necessary
		- last line is unmodified (end.pos == 0)
			then no break is necessary for the last line
	*/

	if (start.pos == impl->lines[start.line]->get_length()) {
		start_part = impl->lines[start.line];
	} else if (start.pos != 0) {
		text_line_t *retval = impl->lines[start.line]->break_line(start.pos);
		if (undo != NULL)
			undo->get_text()->append(*retval->get_data());
		delete retval;
		start_part = impl->lines[start.line];
	}

	if (end.pos == 0)
		end_part = impl->lines[end.line];
	else if (end.pos < impl->lines[end.line]->get_length())
		end_part = impl->lines[end.line]->break_line(end.pos);

	if (start_part == NULL) {
		if (undo != NULL)
			undo->get_text()->append(*impl->lines[start.line]->get_data());
		delete impl->lines[start.line];
		if (end_part != NULL)
			impl->lines[start.line] = end_part;
		else
			impl->lines[start.line] = impl->line_factory->new_text_line_t();
	} else {
		if (end_part != NULL)
			start_part->merge(end_part);
	}

	start.line++;

	if (undo != NULL) {
		undo->add_newline();

		for (i = start.line; i < end.line; i++) {
			undo->get_text()->append(*impl->lines[i]->get_data());
			delete impl->lines[i];
			undo->add_newline();
		}

		if (end.pos != 0) {
			undo->get_text()->append(*impl->lines[end.line]->get_data());
			delete impl->lines[end.line];
		}
	}
	end.line++;
	impl->lines.erase(impl->lines.begin() + start.line, impl->lines.begin() + end.line);

	rewrap_required(rewrap_type_t::DELETE_LINES, start.line, end.line);
	rewrap_required(rewrap_type_t::REWRAP_LINE, start.line - 1, start.pos);
	if ((size_t) start.line < impl->lines.size())
		rewrap_required(rewrap_type_t::REWRAP_LINE, start.line, 0);
}


void text_buffer_t::delete_block(text_coordinate_t start, text_coordinate_t end) {
	/* Don't do anything on empty block */
	if (start == end)
		return;

	delete_block_internal(start, end, get_undo(UNDO_DELETE_BLOCK, start, end));
}

bool text_buffer_t::insert_block_internal(text_coordinate_t insert_at, text_line_t *block) {
	text_line_t *second_half = NULL, *next_line;
	int next_start = 0;
//FIXME: check that everything succeeds and return false if it doesn't
	if (insert_at.pos >= 0 && insert_at.pos < impl->lines[insert_at.line]->get_length())
		second_half = impl->lines[insert_at.line]->break_line(insert_at.pos);

	next_line = block->break_on_nl(&next_start);

	impl->lines[insert_at.line]->merge(next_line);
	rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, insert_at.pos);

	while (next_start > 0) {
		insert_at.line++;
		next_line = block->break_on_nl(&next_start);
		impl->lines.insert(impl->lines.begin() + insert_at.line, next_line);
		rewrap_required(rewrap_type_t::INSERT_LINES, insert_at.line, insert_at.line + 1);
	}

	cursor.pos = impl->lines[insert_at.line]->get_length();

	if (second_half != NULL) {
		impl->lines[insert_at.line]->merge(second_half);
		rewrap_required(rewrap_type_t::REWRAP_LINE, insert_at.line, cursor.pos);
	}

	cursor.line = insert_at.line;
	return true;
}

bool text_buffer_t::insert_block(const string *block) {
	text_coordinate_t cursor_at_start = cursor;
	text_line_t *converted_block = impl->line_factory->new_text_line_t(block);
	undo_t *undo;

	if (!insert_block_internal(cursor, converted_block)) {
		delete converted_block;
		return false;
	}

	undo = get_undo(UNDO_ADD_BLOCK, cursor_at_start, cursor);
	//FIXME: clone may return NULL!
	undo->get_text()->append(*converted_block->get_data());
	delete converted_block;

	return true;
}

bool text_buffer_t::replace_block(text_coordinate_t start, text_coordinate_t end, const string *block) {
	undo_double_text_triple_coord_t *undo;
	text_line_t *converted_block;

//FIXME: check that everything succeeds and return false if it doesn't
	//FIXME: make sure original state is restored on failing sub action
	/* Simply insert on empty block */
	if (start == end)
		return insert_block(block);

	impl->last_undo = undo = new undo_double_text_triple_coord_t(UNDO_REPLACE_BLOCK, start, end);
	impl->last_undo_type = UNDO_NONE;

	delete_block_internal(start, end, undo);

	if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
		start = end;

	converted_block = impl->line_factory->new_text_line_t(block);
	//FIXME: insert_block_internal may fail!!!
	insert_block_internal(start, converted_block);

	undo->get_replacement()->append(*converted_block->get_data());
	delete converted_block;
	undo->set_new_end(cursor);

	impl->undo_list.add(undo);
	return true;
}

string *text_buffer_t::convert_selection(void) {
	text_coordinate_t current_start, current_end;
	string *retval;
	int i;

	current_start = impl->selection_start;
	current_end = impl->selection_end;

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
		return new string(*impl->lines[current_start.line]->get_data(), current_start.pos, current_end.pos - current_start.pos);

	//FIXME: new and append may fail!
	retval = new string(*impl->lines[current_start.line]->get_data(), current_start.pos);
	retval->append(1, '\n');

	for (i = current_start.line + 1; i < current_end.line; i++) {
		//FIXME: append may fail!
		retval->append(*impl->lines[i]->get_data());
		retval->append(1, '\n');
	}

	//FIXME: append may fail!
	retval->append(*impl->lines[current_end.line]->get_data(), 0, current_end.pos);
	return retval;
}

undo_t *text_buffer_t::get_undo(undo_type_t type) {
	return get_undo(type, cursor);
}

undo_t *text_buffer_t::get_undo(undo_type_t type, text_coordinate_t coord) {
	ASSERT(type != UNDO_ADD_BLOCK && type != UNDO_DELETE_BLOCK && type != UNDO_INDENT && type != UNDO_UNINDENT);

	if (impl->last_undo_type == type && impl->last_undo_position.line == coord.line &&
			impl->last_undo_position.pos == coord.pos && impl->last_undo != NULL)
		return impl->last_undo;

	if (impl->last_undo != NULL)
		impl->last_undo->minimize();

	switch (type) {
		case UNDO_ADD_NEWLINE:
		case UNDO_DELETE_NEWLINE:
		case UNDO_BACKSPACE_NEWLINE:
		case UNDO_BLOCK_START:
		case UNDO_BLOCK_END:
			impl->last_undo = new undo_t(type, coord);
			break;
		case UNDO_DELETE:
		case UNDO_ADD:
		case UNDO_BACKSPACE:
		case UNDO_ADD_NEWLINE_INDENT:
			impl->last_undo = new undo_single_text_t(type, coord);
			break;
		case UNDO_OVERWRITE:
			//FIXME: what to do with the last arguments?
			impl->last_undo = new undo_double_text_t(type, coord, text_coordinate_t(-1, -1));
			break;
			break;
		default:
			ASSERT(0);
	}
	impl->last_undo_position = coord;

	impl->undo_list.add(impl->last_undo);
	switch (type) {
		case UNDO_ADD_NEWLINE:
		case UNDO_DELETE_NEWLINE:
		case UNDO_BLOCK_START:
		case UNDO_BLOCK_END:
			impl->last_undo_type = UNDO_NONE;
			break;
		default:
			impl->last_undo_type = type;
			break;
	}
	return impl->last_undo;
}

undo_t *text_buffer_t::get_undo(undo_type_t type, text_coordinate_t start, text_coordinate_t end) {
	ASSERT(type == UNDO_ADD_BLOCK || type == UNDO_DELETE_BLOCK || type == UNDO_INDENT || type == UNDO_UNINDENT);
	if (impl->last_undo != NULL)
		impl->last_undo->minimize();

	impl->last_undo = new undo_single_text_double_coord_t(type, start, end);
	impl->last_undo_type = UNDO_NONE;
	impl->undo_list.add(impl->last_undo);
	return impl->last_undo;
}

void text_buffer_t::set_undo_mark(void) {
	impl->undo_list.set_mark();
	if (impl->last_undo != NULL)
		impl->last_undo->minimize();
	impl->last_undo_type = UNDO_NONE;
}

int text_buffer_t::apply_undo_redo(undo_type_t type, undo_t *current) {
	text_coordinate_t start, end;

	set_selection_mode(selection_mode_t::NONE);
	switch (type) {
		case UNDO_ADD:
			end = start = current->get_start();
			end.pos += current->get_text()->size();
			delete_block_internal(start, end, NULL);
			break;
		case UNDO_ADD_REDO:
		case UNDO_DELETE:
			start = current->get_start();
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_text()));
			if (type == UNDO_DELETE)
				cursor = start;
			break;
		case UNDO_ADD_BLOCK:
			start = current->get_start();
			end = current->get_end();
			delete_block_internal(start, end, NULL);
			break;
		case UNDO_DELETE_BLOCK:
			start = current->get_start();
			end = current->get_end();
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_text()));
			cursor = end;
			break;
		case UNDO_BACKSPACE:
			start = current->get_start();
			start.pos -= current->get_text()->size();
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_text()));
			break;
		case UNDO_BACKSPACE_REDO:
			end = start = current->get_start();
			start.pos -= current->get_text()->size();
			delete_block_internal(start, end, NULL);
			break;
		case UNDO_OVERWRITE:
			end = start = current->get_start();
			end.pos += current->get_replacement()->size();
			delete_block_internal(start, end, NULL);
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_text()));
			cursor = start;
			break;
		case UNDO_OVERWRITE_REDO:
			end = start = current->get_start();
			end.pos += current->get_text()->size();
			delete_block_internal(start, end, NULL);
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_replacement()));
			break;
		case UNDO_REPLACE_BLOCK:
			start = current->get_start();
			end = current->get_end();
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			end = current->get_new_end();
			delete_block_internal(start, end, NULL);
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_text()));
			cursor = current->get_end();
			break;
		case UNDO_REPLACE_BLOCK_REDO:
			start = current->get_start();
			end = current->get_end();
			delete_block_internal(start, end, NULL);
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			//FIXME: insert_block_internal may fail, in which case we must clean up the block
			insert_block_internal(start, impl->line_factory->new_text_line_t(current->get_replacement()));
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
		case UNDO_INDENT:
		case UNDO_UNINDENT:
			undo_indent_selection(current, type);
			break;
		case UNDO_ADD_NEWLINE_INDENT:
			cursor = current->get_start();
			delete_block_internal(cursor, text_coordinate_t(cursor.line + 1, current->get_text()->size() - 1), NULL);
			break;
		case UNDO_ADD_NEWLINE_INDENT_REDO:
			insert_block_internal(current->get_start(), impl->line_factory->new_text_line_t(current->get_text()));
			break;
		case UNDO_BLOCK_START:
		case UNDO_BLOCK_END_REDO:
			cursor = current->get_start();
			break;
		case UNDO_BLOCK_END:
			do {
				current = impl->undo_list.back();
				apply_undo_redo(current->get_type(), current);
			} while (current != NULL && current->get_type() != UNDO_BLOCK_START);
			ASSERT(current != NULL);
			break;
		case UNDO_BLOCK_START_REDO:
			do {
				current = impl->undo_list.forward();
				apply_undo_redo(current->get_redo_type(), current);
			} while (current != NULL && current->get_redo_type() != UNDO_BLOCK_END_REDO);
			ASSERT(current != NULL);
			break;
		default:
			ASSERT(0);
			break;
	}
	impl->last_undo_type = UNDO_NONE;
	return 0;
}

/*FIXME: define return values for:
	- nothing done
	- failure
	- success;
*/
int text_buffer_t::apply_undo(void) {
	undo_t *current = impl->undo_list.back();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_type(), current);
	return 0;
}

int text_buffer_t::apply_redo(void) {
	undo_t *current = impl->undo_list.forward();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_redo_type(), current);
	return 0;
}

void text_buffer_t::set_selection_from_find(find_result_t *result) {
	impl->selection_start.line = result->line;
	impl->selection_start.pos = result->start;

	impl->selection_end.line = result->line;
	impl->selection_end.pos = result->end;

	cursor = get_selection_end();
	impl->selection_mode = selection_mode_t::SHIFT;
	set_primary(convert_selection());
}

bool text_buffer_t::find(finder_t *finder, find_result_t *result, bool reverse) const {
	size_t start, idx;

	start = idx = cursor.line;

	// Perform search
	if (((finder->get_flags() & find_flags_t::BACKWARD) != 0) ^ reverse) {
		result->end = impl->selection_mode != selection_mode_t::NONE ? impl->selection_start.pos : cursor.pos;
		result->start = 0;
		if (finder->match(impl->lines[idx]->get_data(), result, true)) {
			result->line = idx;
			return true;
		}

		result->end = INT_MAX;
		for (; idx > 0; ) {
			idx--;
			if (finder->match(impl->lines[idx]->get_data(), result, true)) {
				result->line = idx;
				return true;
			}
		}

		if (!(finder->get_flags() & find_flags_t::WRAP))
			return false;

		for (idx = impl->lines.size(); idx > start; ) {
			idx--;
			if (finder->match(impl->lines[idx]->get_data(), result, true)) {
				result->line = idx;
				return true;
			}
		}
	} else {
		result->start = cursor.pos;
		result->end = INT_MAX;
		if (finder->match(impl->lines[idx]->get_data(), result, false)) {
			result->line = idx;
			return true;
		}

		result->start = 0;
		for (idx++; idx < impl->lines.size(); idx++) {
			if (finder->match(impl->lines[idx]->get_data(), result, false)) {
				result->line = idx;
				return true;
			}
		}

		if (!(finder->get_flags() & find_flags_t::WRAP))
			return false;

		for (idx = 0; idx <= start; idx++) {
			if (finder->match(impl->lines[idx]->get_data(), result, false)) {
				result->line = idx;
				return true;
			}
		}
	}

	return false;
}

bool text_buffer_t::find_limited(finder_t *finder, text_coordinate_t start, text_coordinate_t end, find_result_t *result) const {
	size_t idx;

	result->start = start.pos;
	result->end = INT_MAX;

	for (idx = start.line; idx < impl->lines.size() && idx < (size_t) end.line; idx++) {
		if (finder->match(impl->lines[idx]->get_data(), result, false)) {
			result->line = idx;
			return true;
		}
		result->start = 0;
	}

	result->end = end.pos;
	if (idx < impl->lines.size() && finder->match(impl->lines[idx]->get_data(), result, false)) {
		result->line = idx;
		return true;
	}

	return false;
}

void text_buffer_t::replace(finder_t *finder, find_result_t *result) {
	text_coordinate_t start(result->line, result->start), end (result->line, result->end);
	string *replacement_str;

	if (result->start == result->end)
		return;

	replacement_str = finder->get_replacement(impl->lines[result->line]->get_data());

	replace_block(start, end, replacement_str);
	delete replacement_str;
}

void text_buffer_t::set_selection_mode(selection_mode_t mode) {
	switch (mode) {
		case selection_mode_t::NONE:
			impl->selection_start = text_coordinate_t(0, -1);
			impl->selection_end = text_coordinate_t(0, -1);
			break;
		case selection_mode_t::ALL:
			impl->selection_start = text_coordinate_t(0, 0);
			impl->selection_end = text_coordinate_t((impl->lines.size() - 1), get_line_max(impl->lines.size() - 1));
			break;
		default:
			if (impl->selection_mode == selection_mode_t::ALL || impl->selection_mode == selection_mode_t::NONE) {
				impl->selection_start = cursor;
				impl->selection_end = cursor;
			}
			break;
	}
	impl->selection_mode = mode;
}

selection_mode_t text_buffer_t::get_selection_mode(void) const {
	return impl->selection_mode;
}

bool text_buffer_t::indent_selection(int tabsize, bool tab_spaces) {
	int end_line;
	text_coordinate_t insert_at;
	string str, *undo_text;
	undo_t *undo;
	text_line_t *indent;

	if (impl->selection_mode == selection_mode_t::NONE && impl->selection_start.line != impl->selection_end.line)
		return true;

	undo = get_undo(UNDO_INDENT, impl->selection_start, impl->selection_end);

	if (tab_spaces)
		str.append(tabsize, ' ');
	else
		str.append(1, '\t');

	insert_at.pos = 0;
	if (impl->selection_end.line < impl->selection_start.line) {
		insert_at.line = impl->selection_end.line;
		end_line = impl->selection_start.line;
	} else {
		insert_at.line = impl->selection_start.line;
		end_line = impl->selection_end.line;
		if (impl->selection_end.pos == 0)
			end_line--;
	}
	undo_text = undo->get_text();

	for (; insert_at.line <= end_line; insert_at.line++) {
		undo_text->append(str);
		undo_text->append(1, 'X'); // Simply add a non-space/tab as marker
		indent = impl->line_factory->new_text_line_t(&str);
		insert_block_internal(insert_at, indent);
	}
	impl->selection_start.pos = 0;
	if (impl->selection_end.pos == 0) {
		undo_text->append(1, 'X'); // Simply add a non-space/tab as marker
		cursor.line = impl->selection_end.line;
	} else {
		impl->selection_end.pos += tab_spaces ? tabsize : 1;
	}
	cursor.pos = impl->selection_end.pos;

	return true;
}

bool text_buffer_t::undo_indent_selection(undo_t *undo, undo_type_t type) {
	int first_line, last_line;
	size_t pos = 0, next_pos;
	string *undo_text;

	if (undo->get_start().line < undo->get_end().line) {
		first_line = undo->get_start().line;
		last_line = undo->get_end().line;
	} else {
		first_line = undo->get_end().line;
		last_line = undo->get_start().line;
	}

	undo_text = undo->get_text();
	for (; first_line <= last_line; first_line++) {
		next_pos = undo_text->find('X', pos);
		if (type == UNDO_INDENT) {
			text_coordinate_t delete_start, delete_end;
			delete_start.line = delete_end.line = first_line;
			delete_start.pos = 0;
			delete_end.pos = next_pos - pos;
			delete_block_internal(delete_start, delete_end, NULL);
		} else {
			text_coordinate_t insert_at(first_line, 0);
			if (next_pos != pos) {
				text_line_t *indent = impl->line_factory->new_text_line_t(undo_text->data() + pos, next_pos - pos);
				insert_block_internal(insert_at, indent);
			}
		}
		pos = next_pos + 1;
	}
	cursor = undo->get_end();
	if (type == UNDO_UNINDENT) {
		pos = undo_text->rfind('X', pos - 2);
		if (pos == string::npos)
		    cursor.pos += undo_text->size() - 1;
		else
			cursor.pos += next_pos - pos - 1;
	}
	return true;
}

bool text_buffer_t::unindent_selection(int tabsize) {
	int end_line;
	text_coordinate_t delete_start, delete_end;
	string undo_text;
	bool text_changed = false;

	if (impl->selection_mode == selection_mode_t::NONE && impl->selection_start.line != impl->selection_end.line)
		return true;

	if (impl->selection_end.line < impl->selection_start.line) {
		delete_start.line = impl->selection_end.line;
		end_line = impl->selection_start.line;
	} else {
		delete_start.line = impl->selection_start.line;
		end_line = impl->selection_end.line;
		if (impl->selection_end.pos == 0)
			end_line--;
	}

	delete_start.pos = 0;
	for (; delete_start.line <= end_line; delete_start.line++) {
		const string *data = impl->lines[delete_start.line]->get_data();
		for (delete_end.pos = 0; delete_end.pos < tabsize; delete_end.pos++) {
			if ((*data)[delete_end.pos] == '\t') {
				delete_end.pos++;
				break;
			} else if ((*data)[delete_end.pos] != ' ') {
				break;
			}
		}

		undo_text.append(*data, 0, delete_end.pos);
		undo_text.append(1, 'X'); // Simply add a non-space/tab as marker
		if (delete_end.pos == 0)
			continue;
		text_changed = true;
		delete_end.line = delete_start.line;
		delete_block_internal(delete_start, delete_end, NULL);

		if (delete_end.line == impl->selection_start.line) {
			if (impl->selection_start.pos > delete_end.pos)
				impl->selection_start.pos -= delete_end.pos;
			else
				impl->selection_start.pos = 0;
		} else if (delete_end.line == impl->selection_end.line) {
			if (impl->selection_end.pos > delete_end.pos)
				cursor.pos = impl->selection_end.pos -= delete_end.pos;
			else
				cursor.pos = impl->selection_end.pos = 0;
		}
	}
	if (impl->selection_end.line > impl->selection_start.line && end_line != impl->selection_end.line)
		undo_text.append(1, 'X'); // Simply add a non-space/tab as marker

	if (text_changed) {
		undo_t *undo = get_undo(UNDO_UNINDENT, impl->selection_start, impl->selection_end);
		undo->get_text()->append(undo_text);
	}
	cursor.line = impl->selection_end.line;
	return true;
}

bool text_buffer_t::unindent_line(int tabsize) {
	text_coordinate_t delete_start(cursor.line, 0), delete_end(cursor.line, 0);

	if (impl->selection_mode != selection_mode_t::NONE)
		set_selection_mode(selection_mode_t::NONE);

	const string *data = impl->lines[delete_start.line]->get_data();
	for (; delete_end.pos < tabsize; delete_end.pos++) {
		if ((*data)[delete_end.pos] == '\t') {
			delete_end.pos++;
			break;
		} else if ((*data)[delete_end.pos] != ' ') {
			break;
		}
	}

	if (delete_end.pos == 0)
		return true;

	delete_block_internal(delete_start, delete_end, get_undo(UNDO_DELETE_BLOCK, delete_start, delete_end));
	if (cursor.pos > delete_end.pos)
		cursor.pos -= delete_end.pos;
	else
		cursor.pos = 0;
	return true;
}

void text_buffer_t::prepare_paint_line(int line) {
	(void) line;
}

void text_buffer_t::start_undo_block(void) {
	get_undo(UNDO_BLOCK_START);
}

void text_buffer_t::end_undo_block(void) {
	get_undo(UNDO_BLOCK_END);
}

}; // namespace
