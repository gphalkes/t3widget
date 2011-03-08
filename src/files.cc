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
#include <sys/stat.h>
#include <pcre.h>

#include <new>
#include <window.h>
#include <unicode.h>

#include "files.h"
#include "lines.h"
#include "subline.h"
#include "colorscheme.h"
#include "util.h"
#include "undo.h"
#include "main.h"
#include "editwindow.h"

using namespace std;
/* FIXME: TODO
	- check rewrap_line return values!
*/

#define LINESINC 64
#define BOM_STRING "\xEF\xBB\xBF"

/*FIXME-REFACTOR: adjust_position in line is often called with same argument as
  where the return value is stored. Check whether this is always the case. If
  so, refactor to pass pointer to first arg.

  find_wrap_line is only called from get_wrap_line. So it should be removed.

  get_line_max may be refactorable to use the cursor, depending on use outside
  files.cc

  check rewrap_line for refactoring.

  set_selection_start/set_selection_end need refactoring. However, there should also
  be a reset_selection call such that we can avoid any arguments.
*/

find_context_t text_file_t::last_find;

/* Free all memory used by 'text' */
text_file_t::~text_file_t(void) {
    int i;

	//FIXME: this doesn't take into account sublines etc. Needs checking of what should be deleted
    /* Free all the line_t structs */
    for (i = 0; (size_t) i < lines.size(); i++)
		delete lines[i];
	//free(name);
}

#warning FIXME: make constructors with edit_window_t parameter such that rewrapping will not be necessary
/* Read 'file' into memory. Returns NULL on failure or an initialized
   text_file_t struct on succes */
text_file_t::text_file_t(const char *_name) : wrap_width(79), name_line(NULL) {
	if ((name = strdup(_name)) == NULL)
		throw bad_alloc();
}

#if 0
RWResult text_file_t::load(LoadState *state) {
	string *line;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case LoadState::INITIAL: {
			char *_name;

			if ((_name = canonicalize_path(name)) == NULL)
				return RWResult(RWResult::ERRNO_ERROR, errno);

			free(name);
			name = _name;

			if ((state->fd = open(name, O_RDONLY)) < 0)
				return RWResult(RWResult::ERRNO_ERROR, errno);

			try {
				name_line = new line_t(name);
				state->wrapper = new FileReadWrapper(state->fd, encoding->getHandle());
			} catch (bad_alloc &ba) {
				return RWResult(RWResult::ERRNO_ERROR, ENOMEM);
			}
			state->state = LoadState::READING;
		}
		case LoadState::READING:
			try {
				while ((line = state->wrapper->readLine()) != NULL) {
					if (lines.size() == 0 && line->size() >= 3 && encoding->get_index() == utf8CharacterSetIndex &&
							memcmp(line->c_str(), BOM_STRING, 3) == 0)
					{
						file_has_bom = true;
						line->erase(0, 3);
					}

					/* Allocate a new line_t struct and initialize it with the newly read line */
					try {
						lines.push_back(new line_t(line));
					} catch (...) {
						delete line;
						return RWResult(RWResult::ERRNO_ERROR, ENOMEM);
					}
					delete line;
				}
			} catch (RWResult &result) {
				return result;
			}
			break;
		default:
			PANIC();
	}
	common_init();
	return RWResult(RWResult::SUCCESS);
}
#endif

/* Create a new, 'empty' text_file_t structure */
text_file_t::text_file_t(void) : wrap_width(79), name(NULL) {
	/* Allocate a new, empty line */
	lines.push_back(new line_t());
	common_init();
	name_line = new line_t("");
}

void text_file_t::common_init(void) {
	#warning FIXME: there is no way to set wrap or tabsize now!
	tabsize = 8;
	wrap = false;

	if (wrap)
		init_wrap_lines();

	selection_start.pos = -1;
	selection_start.line = 0;
	selection_end.pos = -1;
	selection_end.line = 0;
	cursor.pos = 0;
	cursor.line = 0;
	topleft.pos = 0;
	topleft.line = 0;
	ins_mode = 0;
	last_set_pos = 0;
	selection_mode = selection_mode_t::NONE;
	last_undo = NULL;
	last_undo_type = UNDO_NONE;
	window = NULL;
	file_has_bom = false;
	//~ openFiles.push_back(this);
}
#if 0
char *text_file_t::resolve_links(const char *startName) {
	long buffer_max = pathconf("/", _PC_PATH_MAX);

	if (buffer_max < PATH_MAX)
		buffer_max = PATH_MAX;

	char buffer[buffer_max + 1];
	ssize_t retval;

	while ((retval = readlink(startName, buffer, buffer_max)) > 0) {
		if (retval == buffer_max)
			return NULL;
		buffer[retval] = 0;
		startName = buffer;
	}
	return strdup(startName);
}

char *text_file_t::canonicalize_path(const char *path) {
	string result;

	if (path[0] != '/') {
		result = get_working_directory();
		result += "/";
	}

	result += path;

	/* TODO:
		- remove all occurences of // and /./
	    - remove all occurences of ^/..
		- remove all occurences of XXX/../
	*/
	return strdup(result.c_str());
}

RWResult text_file_t::save(SaveState *state) {
	size_t idx;
	const char *save_name;
	charconv_t *handle;

	if (state->file != this)
		PANIC();

	switch (state->state) {
		case SaveState::INITIAL:
			if (state->name == NULL) {
				if (name == NULL)
					PANIC();
				save_name = name;
			} else {
				save_name = state->name;
			}

			if ((state->realName = resolve_links(save_name)) == NULL)
				return RWResult(RWResult::ERRNO_ERROR, ENAMETOOLONG);

			if (stat(state->realName, &state->file_info) < 0) {
				if (errno == ENOENT) {
					if ((state->fd = creat(state->realName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
						return RWResult(RWResult::ERRNO_ERROR, errno);
				} else {
					return RWResult(RWResult::ERRNO_ERROR, errno);
				}
			} else {
				state->state = SaveState::ALLOW_OVERWRITE;
				if (state->name != NULL)
					return RWResult(RWResult::FILE_EXISTS);
		case SaveState::ALLOW_OVERWRITE:
				string temp_name_str = state->realName;

				if ((idx = temp_name_str.rfind('/')) == string::npos)
					idx = 0;
				else
					idx++;

				temp_name_str.erase(idx);
				try {
					temp_name_str.append(".tildeXXXXXX");
				} catch (bad_alloc &ba) {
					return RWResult(RWResult::ERRNO_ERROR, ENOMEM);
				}

				/* Unfortunately, we can't pass the c_str result to mkstemp as we are not allowed
				   to change that string. So we'll just have to strdup it :-( */
				if ((state->tempName = strdup(temp_name_str.c_str())) == NULL)
					return RWResult(RWResult::ERRNO_ERROR, errno);

				if ((state->fd = mkstemp(state->tempName)) < 0)
					return RWResult(RWResult::ERRNO_ERROR, errno);

				// Preserve ownership and attributes
				fchmod(state->fd, state->file_info.st_mode);
				fchown(state->fd, -1, state->file_info.st_gid);
				fchown(state->fd, state->file_info.st_uid, -1);
			}

			if (state->encoding != NULL)
				handle = state->encoding->getHandle();
			else if (encoding != NULL)
				handle = encoding->getHandle();
			else
				handle = NULL;
			state->wrapper = new FileWriteWrapper(state->fd, handle);
			state->i = 0;
			state->state = SaveState::WRITING;
		case SaveState::WRITING:
			try {
				if (file_has_bom && state->i == 0)
					state->wrapper->write(BOM_STRING, 3);

				if (state->i != 0)
					state->wrapper->write("\n", 1);

				for (; state->i < lines.size(); state->i++)
					lines[state->i]->writeLineData(state->wrapper, state->i < lines.size() - 1);
			} catch (RWResult error) {
				return error;
			}

			fsync(state->fd);
			close(state->fd);
			state->fd = -1;
			if (state->tempName != NULL) {
				if (rename(state->tempName, state->realName) < 0)
					return RWResult(RWResult::ERRNO_ERROR, errno);
			}

			if (state->newName != NULL) {
				free(name);
				delete name_line;
				name = state->newName;
				name_line = state->newNameLine;
				state->newName = NULL;
				state->newNameLine = NULL;
			}
			undo_list.setMark();
			last_undo_type = UNDO_NONE;
			break;
		default:
			PANIC();
	}
	return RWResult(RWResult::SUCCESS);
}
#endif
int text_file_t::get_used_lines(void) const {
	return wrap ? wraplines.size() : lines.size();
}

void text_file_t::locate_pos(void) {
	locate_pos(&cursor);
}

void text_file_t::locate_pos(text_coordinate_t *coord) const {
	if (coord->pos < wraplines[coord->line]->get_start()) {
		do {
			coord->line--;
			ASSERT(coord->line >= 0);
			ASSERT(wraplines[coord->line]->get_line() == wraplines[coord->line + 1]->get_line());
		} while (coord->pos < wraplines[coord->line]->get_start());
		return;
	}

	while ((size_t) coord->line + 1 < wraplines.size() &&
			wraplines[coord->line]->get_line() == wraplines[coord->line + 1]->get_line() &&
			wraplines[coord->line + 1]->get_start() <= coord->pos)
		coord->line++;
}

bool text_file_t::insert_char(key_t c) {
	bool retval;

	if (wrap) {
		//FIXME: maybe we should undo the insert action if the wrap fails
		retval = wraplines[cursor.line]->get_line()->insert_char(cursor.pos, c, get_undo(UNDO_ADD));
		if (retval) {
			if (cursor.line > 0 && wraplines[cursor.line - 1]->get_line() == wraplines[cursor.line]->get_line())
				rewrap_line(cursor.line - 1);
			else
				rewrap_line(cursor.line);
			cursor.pos = wraplines[cursor.line]->get_line()->adjust_position(cursor.pos, 1);
			locate_pos();
		}
	} else {
		retval = lines[cursor.line]->insert_char(cursor.pos, c, get_undo(UNDO_ADD));
		if (retval)
			cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
	}

	last_undo_position = cursor;

	return retval;
}

bool text_file_t::overwrite_char(key_t c) {
	bool retval;

	if (wrap) {
		//FIXME: maybe we should undo the overwrite action if the wrap fails
		retval = wraplines[cursor.line]->get_line()->overwrite_char(cursor.pos, c, get_undo(UNDO_OVERWRITE));
		if (retval) {
			if (cursor.line > 0 && wraplines[cursor.line - 1]->get_line() == wraplines[cursor.line]->get_line())
				rewrap_line(cursor.line - 1);
			else
				rewrap_line(cursor.line);
			cursor.pos = wraplines[cursor.line]->get_line()->adjust_position(cursor.pos, 1);
			locate_pos(&cursor);
		}
	} else {
		retval = lines[cursor.line]->overwrite_char(cursor.pos, c, get_undo(UNDO_OVERWRITE));
		if (retval)
			cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, 1);
	}

	last_undo_position = cursor;

	return retval;
}

bool text_file_t::append_char(key_t c) {
	bool retval;

	ASSERT(cursor.pos == get_line_max(cursor.line));

	if (wrap) {
		//FIXME: maybe we should undo the append/insert action if the wrap fails
		if ((size_t) cursor.line == wraplines.size() - 1 || wraplines[cursor.line]->get_line() != wraplines[cursor.line + 1]->get_line()) {
			cursor.pos = wraplines[cursor.line]->get_line()->get_length();
			retval = wraplines[cursor.line]->get_line()->append_char(c, get_undo(UNDO_ADD));
			if (retval)
				cursor.pos = wraplines[cursor.line]->get_line()->get_length();
		} else {
			cursor.pos = wraplines[cursor.line]->get_line()->adjust_position(wraplines[cursor.line + 1]->get_start(), -1);
			retval = wraplines[cursor.line]->get_line()->insert_char(cursor.pos, c, get_undo(UNDO_ADD));
			if (retval)
				cursor.pos = wraplines[cursor.line]->get_line()->adjust_position(cursor.pos, 1);
		}

		if (retval) {
			rewrap_line(cursor.line);
			locate_pos();
		}
	} else {
		cursor.pos = lines[cursor.line]->get_length();
		retval = lines[cursor.line]->append_char(c, get_undo(UNDO_ADD));
		if (retval)
			cursor.pos = lines[cursor.line]->get_length();
	}

	last_undo_position = cursor;

	return retval;
}

int text_file_t::delete_char(void) {
	if (wrap) {
		int retval = wraplines[cursor.line]->get_line()->delete_char(cursor.pos, get_undo(UNDO_DELETE));
		if (cursor.line > 0 && wraplines[cursor.line - 1]->get_line() == wraplines[cursor.line]->get_line())
			rewrap_line(cursor.line - 1);
		else
			rewrap_line(cursor.line);
		locate_pos();
		return retval;
	} else {
		return lines[cursor.line]->delete_char(cursor.pos, get_undo(UNDO_DELETE));
	}
}

int text_file_t::backspace_char(void) {
	int retval;
	int newpos;

	if (wrap) {
		newpos = wraplines[cursor.line]->get_line()->adjust_position(cursor.pos, -1);
		retval = wraplines[cursor.line]->get_line()->backspace_char(cursor.pos, get_undo(UNDO_BACKSPACE));
		if (cursor.line > 0 && wraplines[cursor.line - 1]->get_line() == wraplines[cursor.line]->get_line())
			cursor.line--;

		cursor.pos = newpos;
		rewrap_line(cursor.line);
		locate_pos();
	} else {
		newpos = lines[cursor.line]->adjust_position(cursor.pos, -1);
		retval = lines[cursor.line]->backspace_char(cursor.pos, get_undo(UNDO_BACKSPACE));
		cursor.pos = newpos;
	}

	last_undo_position = cursor;
	return retval;
}

// FIXME: these functions are not used yet
//~ int text_file_t::insertString(int line, int pos, char *string);
//~ int text_file_t::appendString(int line, char *string);
//~ int text_file_t::deleteString(int line, int pos, int n);
//~ int text_file_t::searchString(int line, int pos, char *string);
//~ int text_file_t::isearchString(int line, int pos, char *string);
//~ int text_file_t::replaceString(int line, int pos, int n, char *string);

int text_file_t::find_line(int idx) const {
	line_t *line = wraplines[idx]->get_line();

	if ((size_t) idx >= lines.size())
		idx = lines.size() - 1;

	while (idx > 0 && lines[idx] != line)
		idx--;

	ASSERT(lines[idx] == line);
	return idx;
}

void text_file_t::find_wrap_line(text_coordinate_t *coord) const {
	line_t *line = lines[coord->line];
	while (wraplines[coord->line]->get_line() != line && (size_t) coord->line < wraplines.size())
		coord->line++;

	ASSERT(wraplines[coord->line]->get_line() == line);
	locate_pos(coord);
}

int text_file_t::get_real_line(int line) const {
	return wrap ? find_line(line) : line;
}

text_coordinate_t text_file_t::get_wrap_line(text_coordinate_t coord) const {
	if (wrap)
		find_wrap_line(&coord);
	return coord;
}

int text_file_t::merge_internal(int line) {
	if (wrap) {
		int otherline;
		line_t *replace_line, *replace_with;

		cursor.line = line;
		cursor.pos = wraplines[line]->get_line()->get_length();

		replace_with = wraplines[line]->get_line();
		replace_line = wraplines[line + 1]->get_line();

		wraplines[line]->get_line()->merge(wraplines[line + 1]->get_line());

		lines.erase(lines.begin() + find_line(line + 1));

		for (otherline = line + 1; (size_t) otherline < wraplines.size() &&
				wraplines[otherline]->get_line() == replace_line; otherline++)
		{
			wraplines[otherline]->set_line(replace_with);
			wraplines[otherline]->set_start(-1);
			wraplines[otherline]->set_flags(0);
		}

		if (line > 0 && wraplines[line - 1]->get_line() == wraplines[line]->get_line())
			rewrap_line(line - 1);
		else
			rewrap_line(line);
		locate_pos();
	} else {
		cursor.line = line;
		cursor.pos = lines[line]->get_length();
		lines[line]->merge(lines[line + 1]);
		lines.erase(lines.begin() + line + 1);
	}
	return 0;
}

int text_file_t::merge(bool backspace) {
	if (backspace) {
		if (wrap && wraplines[cursor.line]->get_line() == wraplines[cursor.line - 1]->get_line())
			return backspace_char();
		get_undo(UNDO_BACKSPACE_NEWLINE, cursor.line - 1, (wrap ? wraplines[cursor.line - 1]->get_line() : lines[cursor.line - 1])->get_length());
		return merge_internal(cursor.line - 1);
	} else {
		if (wrap && wraplines[cursor.line]->get_line() == wraplines[cursor.line + 1]->get_line())
			return delete_char();
		get_undo(UNDO_DELETE_NEWLINE, cursor.line, (wrap ? wraplines[cursor.line]->get_line() : lines[cursor.line])->get_length());
		return merge_internal(cursor.line);
	}
}

bool text_file_t::break_line_internal(void) {
	int lineindex;
	line_t *insert;

	lineindex = wrap ? find_line(cursor.line) : cursor.line;

	insert = lines[lineindex]->break_line(cursor.pos);
	lines.insert(lines.begin() + lineindex + 1, insert);
	if (wrap) {
		int nextline;

		wraplines[cursor.line]->set_flags(0);

		for (nextline = cursor.line + 1; (size_t) nextline < wraplines.size() &&
				wraplines[cursor.line]->get_line() == wraplines[nextline]->get_line();
				nextline++) {}

		if (nextline > cursor.line + 1) {
			wraplines[cursor.line + 1]->set_line(insert);
			wraplines[cursor.line + 1]->set_start(0);
			wraplines[cursor.line + 1]->set_flags(0);
			if (nextline > cursor.line + 2)
				wraplines.erase(wraplines.begin() + cursor.line + 2, wraplines.begin() + nextline);
		} else {
			subline_t *next = new subline_t(insert, 0);
			if (next == NULL)
				return false;

			wraplines.insert(wraplines.begin() + cursor.line + 1, next);
		}

		rewrap_line(cursor.line + 1);
		rewrap_line(cursor.line);
	}
	return true;
}

bool text_file_t::break_line(void) {
	get_undo(UNDO_ADD_NEWLINE);
	return break_line_internal();
}

void text_file_t::new_line(void) {
	int lineindex;
	line_t *insert = new line_t();

	get_undo(UNDO_ADD_NEWLINE);

	lineindex = get_real_line(cursor.line + 1);
	lines.insert(lines.begin() + lineindex, insert);
	if (wrap) {
		subline_t *next = new subline_t(insert, 0);
		wraplines.insert(wraplines.begin() + cursor.line + 1, next);
	}
}

int text_file_t::calculate_screen_pos(const text_coordinate_t *where) const {
	if (where == NULL)
		where = &cursor;
	return wrap ?
		wraplines[where->line]->get_line()->calculate_screen_width(wraplines[where->line]->get_start(), where->pos, tabsize)
		: lines[where->line]->calculate_screen_width(0, where->pos, tabsize);
}

int text_file_t::get_max(int line) const {
	return (size_t) line == wraplines.size() - 1 ||
		wraplines[line]->get_line() != wraplines[line + 1]->get_line() ?
		INT_MAX : wraplines[line + 1]->get_start();
}

int text_file_t::calculate_line_pos(int line, int pos) const {
	if (wrap) {
		int retval;
		int max = get_max(line);

		/* For wrapped text, the position after the last character on a line is
		   not permissable because that is the same as the first position on the
		   next line. */
		if (max < INT_MAX)
			max--;

		retval = wraplines[line]->get_line()->calculate_line_pos(wraplines[line]->get_start(),
			max, pos, tabsize);

		return retval;
	} else {
		return lines[line]->calculate_line_pos(0, INT_MAX, pos, tabsize);
	}
}

void text_file_t::paint_line(t3_window_t *win, int line, line_t::paint_info_t *info) const {
	info->tabsize = tabsize;
	if (wrap) {
		info->start = wraplines[line]->get_start();
		info->max = get_max(line);
		info->flags = wraplines[line]->get_flags();
		wraplines[line]->get_line()->paint_line(win, info);
	} else {
		info->start = 0;
		info->max = INT_MAX;
		info->flags = 0;
		lines[line]->paint_line(win, info);
	}
}

int text_file_t::get_line_max(int line) const {
	if (wrap) {
		if ((size_t) line == wraplines.size() - 1 || wraplines[line]->get_line() != wraplines[line + 1]->get_line())
			return wraplines[line]->get_line()->get_length();
		return wraplines[line + 1]->get_line()->adjust_position(wraplines[line + 1]->get_start(), - 1);
	} else {
		return lines[line]->get_length();
	}
}

void text_file_t::get_next_word(void) {
	line_t *line;

	if (wrap)
		line = wraplines[cursor.line]->get_line();
	else
		line = lines[cursor.line];

	cursor.pos = cursor.pos;

	/* Use -1 as an indicator for end of line */
	if (cursor.pos >= line->get_length())
		cursor.pos = -1;
	else if (cursor.pos >= 0)
		cursor.pos = line->get_next_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0) {
		if (wrap) {
			while ((size_t) cursor.line + 1 < wraplines.size() && line == wraplines[cursor.line + 1]->get_line())
				cursor.line++;
			if ((size_t) cursor.line + 1 >= wraplines.size())
				break;
			cursor.line++;
			line = wraplines[cursor.line]->get_line();
		} else {
			if ((size_t) cursor.line + 1 >= lines.size())
				break;
			line = lines[++cursor.line];
		}
		cursor.pos = line->get_next_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = line->get_length();

	if (wrap) {
		while ((size_t) cursor.line + 1 < wraplines.size() &&
				wraplines[cursor.line + 1]->get_line() == line &&
				wraplines[cursor.line + 1]->get_start() <= cursor.pos)
		{
			cursor.line++;
		}
	}
}

void text_file_t::get_previous_word(void) {
	line_t *line;

	if (wrap)
		line = wraplines[cursor.line]->get_line();
	else
		line = lines[cursor.line];

	cursor.pos = line->get_previous_word(cursor.pos);

	/* Keep skipping to next line if no word can be found */
	while (cursor.pos < 0 && cursor.line > 0) {
		if (wrap) {
			while (cursor.line > 0 && line == wraplines[cursor.line - 1]->get_line())
				cursor.line--;
			if (cursor.line == 0)
				break;
			line = wraplines[--cursor.line]->get_line();
		} else {
			line = lines[--cursor.line];
		}
		cursor.pos = line->get_previous_word(-1);
	}

	/* Convert cursor.line and cursor.pos to text coordinate again. */
	if (cursor.pos < 0)
		cursor.pos = 0;

	if (wrap) {
		while (wraplines[cursor.line]->get_start() > cursor.pos)
			cursor.line--;
	}
}

bool text_file_t::init_wrap_lines(void) {
	int i;

	for (i = 0; (size_t) i < lines.size(); i++) {
		break_pos_t breakpos = { 0, 0 };

		do {
			if (breakpos.pos != 0)
				wraplines[wraplines.size() - 1]->set_flags(breakpos.flags);
			wraplines.push_back(new subline_t(lines[i], breakpos.pos));
		} while ((breakpos = lines[i]->find_next_break_pos(breakpos.pos, wrap_width - 1, tabsize)).pos >= 0);
	}

	return true;
}

/* rewrap a real line starting with wrapped line 'line' */
bool text_file_t::rewrap_line(int line) {
	int lastline;
	break_pos_t breakpos = wraplines[line]->get_line()->find_next_break_pos(wraplines[line]->get_start(), wrap_width - 1, tabsize);

	/* First simply rewrap the existing sublines_t */
	while (breakpos.pos >= 0 && (size_t) line < wraplines.size() - 1 &&
			wraplines[line]->get_line() == wraplines[line + 1]->get_line())
	{
		wraplines[line]->set_flags(breakpos.flags);
		line++;
		wraplines[line]->set_start(breakpos.pos);
		breakpos = wraplines[line]->get_line()->find_next_break_pos(breakpos.pos, wrap_width - 1, tabsize);
	}

	while (breakpos.pos >= 0) {
		subline_t *next;

		wraplines[line]->set_flags(breakpos.flags);
		if ((next = new subline_t(wraplines[line]->get_line(), breakpos.pos)) == NULL)
			return false;

		line++;
		wraplines.insert(wraplines.begin() + line, next);
		breakpos = wraplines[line]->get_line()->find_next_break_pos(breakpos.pos, wrap_width - 1, tabsize);
	}
	wraplines[line]->set_flags(0);

	lastline = line;
	while ((size_t) lastline < wraplines.size() - 1 &&
			wraplines[line]->get_line() == wraplines[lastline + 1]->get_line())
		lastline++;

	if (lastline > line)
		wraplines.erase(wraplines.begin() + line + 1, wraplines.begin() + lastline + 1);

	return true;
}

void text_file_t::get_line_info(text_coordinate_t *new_coord) const {
	new_coord->line = wrap ? find_line(cursor.line) : cursor.line;
	new_coord->pos = lines[new_coord->line]->calculate_screen_width(0, cursor.pos, tabsize);
}

void text_file_t::adjust_position(int adjust) {
	/* FIXME check that an add does not go to the next wrapped line (should
	   not happen given the checks in the caller) */

	if (wrap) {
		cursor.pos = wraplines[cursor.line]->get_line()->adjust_position(cursor.pos, adjust);
	} else {
		cursor.pos = lines[cursor.line]->adjust_position(cursor.pos, adjust);
	}
}

void text_file_t::rewrap(void) {
	line_t *base_line;
	int i, wrap_index = 0;

	if (window != NULL) {
		int discard, width;
		window->get_dimensions(&discard, &width, &discard, &discard);
		wrap_width = width - 1;
	}

	base_line = wraplines[cursor.line]->get_line();

	for (i = 0; (size_t) i < lines.size(); i++) {
		while (lines[i] != wraplines[wrap_index]->get_line()) wrap_index++;
		if (!rewrap_line(wrap_index))
			break;
		if (base_line == lines[i])
			cursor.line = wrap_index;
	}

	locate_pos();
}

bool text_file_t::get_wrap(void) const {
	return wrap;
}

int text_file_t::width_at_cursor(void) const {
	if (wrap)
		return wraplines[cursor.line]->get_line()->width_at(cursor.pos);
	else
		return lines[cursor.line]->width_at(cursor.pos);
}

bool text_file_t::selection_empty(void) const {
	return selection_start.line == selection_end.line && selection_start.pos == selection_end.pos;
}

void text_file_t::set_selection_start(int line, int pos) {
	selection_start.line = get_real_line(line);
	selection_start.pos = pos;
}

void text_file_t::set_selection_end(int line, int pos)  {
	selection_end.line = get_real_line(line);
	selection_end.pos = pos;
}

text_coordinate_t text_file_t::get_selection_start(void) const { return selection_start.pos < 0 ? selection_start : get_wrap_line(selection_start); }
text_coordinate_t text_file_t::get_selection_end(void) const { return selection_end.pos < 0 ? selection_end : get_wrap_line(selection_end); }

void text_file_t::delete_block(text_coordinate_t start, text_coordinate_t end, undo_t *undo) {
	line_t *start_part = NULL, *end_part = NULL;
	int i, wrapped_start_line = 0 /* Shut up compiler */;

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

	if (start.line == end.line ||
			(wrap && wraplines[start.line]->get_line() == wraplines[end.line]->get_line())) {
		line_t *selected_text;
		if (wrap) {
			selected_text = wraplines[start.line]->get_line()->cut_line(start.pos, end.pos);
			if (start.line > 0 && wraplines[start.line]->get_line() == wraplines[start.line - 1]->get_line())
				rewrap_line(start.line - 1);
			else
				rewrap_line(start.line);
		} else {
			selected_text = lines[start.line]->cut_line(start.pos, end.pos);
		}
		if (undo != NULL)
			undo->get_text()->merge(selected_text);
		else
			delete selected_text;
		return;
	}

	if (wrap) {
		line_t *end_line;
		int wrapped_end_line;

		wrapped_start_line = start.line;
		start.line = find_line(wrapped_start_line);
		/* Find first wrapped line that points to the starting line_t * */
		while (wrapped_start_line > 0 && wraplines[wrapped_start_line - 1]->get_line() == wraplines[start.line]->get_line())
			wrapped_start_line--;

		end_line = wraplines[end.line]->get_line();
		/* Find first wrapped line that points to the line_t * after the ending line */
		for (wrapped_end_line = end.line;
				(size_t) wrapped_end_line < wraplines.size() && wraplines[wrapped_end_line]->get_line() == end_line;
				wrapped_end_line++) {}

		/* Throw away all the wrapping information for all the lines that won't exist after
		   we delete the lines */
		for (i = wrapped_start_line + 1; i < wrapped_end_line; i++)
			delete wraplines[i];

		wraplines.erase(wraplines.begin() + wrapped_start_line + 1, wraplines.begin() + wrapped_end_line);

		/* Find the index of the ending line_t * */
		for (end.line = start.line + 1; lines[end.line] != end_line; end.line++) {}
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
		line_t *retval = lines[start.line]->break_line(start.pos);
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
			/* For wrapped text we need to change the pointer in the wrapped lines as well. */
			if (wrap) {
				wraplines[wrapped_start_line]->set_line(lines[start.line]);
				wraplines[wrapped_start_line]->set_start(0);
				wraplines[wrapped_start_line]->set_flags(0);
			}
		} else {
			if (undo != NULL)
				undo->get_text()->merge(lines[start.line]);
			else
				delete lines[start.line];
			lines[start.line] = new line_t();
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

	if (wrap)
		rewrap_line(wrapped_start_line);
}


void text_file_t::delete_selection(void) {
	text_coordinate_t current_start, current_end;

	current_start = get_selection_start();
	current_end = get_selection_end();

	/* Don't do anything on empty selection */
	if (current_start.line == current_end.line && current_start.pos == current_end.pos)
		return;

	last_undo = new undo_single_text_double_coord_t(UNDO_DELETE_BLOCK, get_real_line(current_start.line),
			current_start.pos, get_real_line(current_end.line), current_end.pos);
	last_undo_type = UNDO_DELETE_BLOCK;

	delete_block(current_start, current_end, last_undo);

	undo_list.add(last_undo);
}

void text_file_t::insert_block_internal(text_coordinate_t insertAt, line_t *block) {
	line_t *second_half = NULL, *next_line;
	int next_start = 0, unwrapped_line, saved_line;
	int i, j;

	if (wrap) {
		unwrapped_line = find_line(insertAt.line);
		saved_line = unwrapped_line;
	} else {
		unwrapped_line = insertAt.line;
	}

	if (insertAt.pos >= 0 && insertAt.pos < lines[unwrapped_line]->get_length())
		second_half = lines[unwrapped_line]->break_line(insertAt.pos);

	next_line = block->break_on_nl(&next_start);

	lines[unwrapped_line]->merge(next_line);

	while (next_start > 0) {
		unwrapped_line++;
		next_line = block->break_on_nl(&next_start);
		lines.insert(lines.begin() + unwrapped_line, next_line);
	}

	cursor.pos = lines[unwrapped_line]->get_length();

	if (second_half != NULL)
		lines[unwrapped_line]->merge(second_half);

	if (wrap) {
		/* - remove wrapping info for indicated line because inserting may cause different wrapping
		   - at end add at least one subline_t for each inserted line and allow rewrap to do its magic.
		*/
		int wrap_start, wrap_end;

		for (wrap_start = insertAt.line; wrap_start > 0 &&
			wraplines[wrap_start]->get_line() == wraplines[wrap_start - 1]->get_line(); wrap_start--) {}
		for (wrap_end = insertAt.line; (size_t) wrap_end < wraplines.size() - 1 &&
			wraplines[wrap_end]->get_line() == wraplines[wrap_end + 1]->get_line(); wrap_end++) {}

 		if (wrap_end - wrap_start < unwrapped_line - saved_line) {
			int needed_sublines = unwrapped_line - saved_line + 1;
			int available_sublines = wrap_end - wrap_start + 1;

			wraplines.reserve(wraplines.size() + needed_sublines - available_sublines);
			wraplines.insert(wraplines.begin() + wrap_end + 1, needed_sublines - available_sublines, (subline_t *) NULL);

			for (i = wrap_end + 1; i <= wrap_start + unwrapped_line - saved_line; i++)
				wraplines[i] = new subline_t(NULL, 0);
			wrap_end = wrap_start + unwrapped_line - saved_line;
		}

		for (i = unwrapped_line, j = wrap_end; i > saved_line; i--, j--) {
			wraplines[j]->set_line(lines[i]);
			wraplines[j]->set_start(0);
			wraplines[j]->set_flags(0);
			rewrap_line(j);
		}

		rewrap_line(wrap_start);

		for (cursor.line = wrap_end; wraplines[cursor.line]->get_line() != lines[unwrapped_line] &&
			(size_t) cursor.line < wraplines.size(); cursor.line++) {}
		ASSERT((size_t) cursor.line < wraplines.size());
		locate_pos();
	} else {
		cursor.line = unwrapped_line;
	}
}

int text_file_t::insert_block(line_t *block) {
	text_coordinate_t cursor_at_start = cursor;

	insert_block_internal(cursor, block);

	last_undo = new undo_single_text_double_coord_t(UNDO_ADD_BLOCK, get_real_line(cursor_at_start.line), cursor_at_start.pos, get_real_line(cursor.line), cursor.pos);
	last_undo_type = UNDO_ADD_BLOCK;
	//FIXME: clone may return NULL!
	last_undo->get_text()->merge(block->clone(0, -1));
	undo_list.add(last_undo);
	return 0;
}

void text_file_t::replace_selection(line_t *block) {
	text_coordinate_t current_start, current_end;
	undo_double_text_triple_coord_t *undo;

	current_start = get_selection_start();
	current_end = get_selection_end();

	//FIXME: make sure original state is restored on failing sub action
	/* Simply insert on empty selection */
	if (current_start.line == current_end.line && current_start.pos == current_end.pos) {
		insert_block(block);
		return;
	}

	last_undo = undo = new undo_double_text_triple_coord_t(UNDO_REPLACE_BLOCK, get_real_line(current_start.line),
			current_start.pos, get_real_line(current_end.line), current_end.pos);
	last_undo_type = UNDO_REPLACE_BLOCK;

	delete_block(current_start, current_end, undo);

	if (current_end.line < current_start.line || (current_end.line == current_start.line && current_end.pos < current_start.pos))
		current_start = current_end;

	insert_block_internal(current_start, block);

	undo->get_replacement()->merge(block->clone(0, -1));
	undo->setNewEnd(cursor);

	undo_list.add(undo);
}

line_t *text_file_t::convert_selection(void) {
	text_coordinate_t current_start, current_end;
	line_t *retval;
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

	if (wrap) {
		current_start.line = find_line(current_start.line);
		current_end.line = find_line(current_end.line);
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

undo_t *text_file_t::get_undo(undo_type_t type) {
	return get_undo(type, cursor.line, cursor.pos);
}

undo_t *text_file_t::get_undo(undo_type_t type, int line, int pos) {
	ASSERT(type != UNDO_ADD_BLOCK && type != UNDO_DELETE_BLOCK);

	line = get_real_line(line);

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


int text_file_t::apply_undo_redo(undo_type_t type, undo_t *current) {
	text_coordinate_t start, end;

	switch (type) {
		case UNDO_ADD:
			end = start = get_wrap_line(current->get_start());
			end.pos += current->get_text()->get_length();
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_ADD_REDO:
		case UNDO_DELETE:
			start = get_wrap_line(current->get_start());
			insert_block_internal(start, current->get_text());
			if (type == UNDO_DELETE)
				cursor = start;
			break;
		case UNDO_ADD_BLOCK:
			start = get_wrap_line(current->get_start());
			end = get_wrap_line(current->get_end());
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_DELETE_BLOCK:
			start = get_wrap_line(current->get_start());
			end = get_wrap_line(current->get_end());
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			insert_block_internal(start, current->get_text());
			cursor = end;
			break;
		case UNDO_BACKSPACE:
			start = get_wrap_line(current->get_start());
			start.pos -= current->get_text()->get_length();
			insert_block_internal(start, current->get_text());
			break;
		case UNDO_BACKSPACE_REDO:
			end = start = get_wrap_line(current->get_start());
			start.pos -= current->get_text()->get_length();
			delete_block(start, end, NULL);
			cursor = start;
			break;
		case UNDO_OVERWRITE:
			end = start = get_wrap_line(current->get_start());
			end.pos += current->get_replacement()->get_length();
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_text());
			cursor = start;
			break;
		case UNDO_OVERWRITE_REDO:
			end = start = get_wrap_line(current->get_start());
			end.pos += current->get_text()->get_length();
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_replacement());
			break;
		case UNDO_REPLACE_BLOCK:
			start = get_wrap_line(current->get_start());
			end = get_wrap_line(current->get_end());
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			end = get_wrap_line(current->get_new_end());
			delete_block(start, end, NULL);
			insert_block_internal(start, current->get_text());
			cursor = get_wrap_line(current->get_end());
			break;
		case UNDO_REPLACE_BLOCK_REDO:
			start = get_wrap_line(current->get_start());
			end = get_wrap_line(current->get_end());
			delete_block(start, end, NULL);
			if (end.line < start.line || (end.line == start.line && end.pos < start.pos))
				start = end;
			insert_block_internal(start, current->get_replacement());
			break;
		case UNDO_BACKSPACE_NEWLINE:
			cursor = get_wrap_line(current->get_start());
			break_line_internal();
			cursor.line++;
			cursor.pos = 0;
			break;
		case UNDO_DELETE_NEWLINE:
			cursor = get_wrap_line(current->get_start());
			break_line_internal();
			break;
		case UNDO_ADD_NEWLINE:
			start = get_wrap_line(current->get_start());
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
int text_file_t::apply_undo(void) {
	undo_t *current = undo_list.back();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_type(), current);
	last_undo_type = UNDO_NONE;

	return 0;
}

int text_file_t::apply_redo(void) {
	undo_t *current = undo_list.forward();

	if (current == NULL)
		return -1;

	apply_undo_redo(current->get_redo_type(), current);
	last_undo_type = UNDO_NONE;

	return 0;
}

void text_file_t::set_selection_from_find(int line, find_result_t *result) {
	selection_start.line = line;
	selection_start.pos = result->start;

	selection_end.line = line;
	selection_end.pos = result->end;

	cursor = get_selection_end();
	selection_mode = selection_mode_t::SHIFT;
}

bool text_file_t::find(const string *what, int flags, const line_t *replacement) {
	find_result_t result;
	size_t start, idx;

	// Prepare state for finding
	if (what == NULL) {
		if (!(last_find.flags & find_flags_t::VALID))
			return true;

		last_find.flags = flags ^ last_find.original_flags; // flags will only contain NEXT and optionally BACKWARD
	} else {
		if (last_find.matcher != NULL) {
			delete last_find.matcher;
			last_find.matcher = NULL;
		}
		if (last_find.regex != NULL) {
			pcre_free(last_find.regex);
			last_find.regex = NULL;
		}
		if (last_find.replacement != NULL) {
			delete last_find.replacement;
			last_find.replacement = NULL;
		}

		last_find.flags = last_find.original_flags = flags | find_flags_t::VALID;
		if (flags & find_flags_t::REGEX) {
			const char *error_message;
			int error_offset;
			int pcre_flags = PCRE_UTF8;
			string pattern = string(flags & find_flags_t::WHOLE_WORD ? "(?:\\b" : "(?:") + *what + string(flags & find_flags_t::WHOLE_WORD ? "\\b)(?C0)" : ")(?C0)");

			if (flags & find_flags_t::ICASE)
				pcre_flags |= PCRE_CASELESS;
			last_find.regex = pcre_compile(pattern.c_str(), pcre_flags, &error_message, &error_offset, NULL);
			if (last_find.regex == NULL) {
				string message;
				#warning FIXME: show error message here
/*				printf_into(&message, "Error in regular expression: %s", error_message);
				last_find.flags = 0;
				activate_window(WindowID::ERROR_DIALOG, &message);*/
				return true;
			}
			last_find.pattern_length = pattern.size();
		} else {
			string searchFor(*what);

			#warning FIXME: implement transfom_backslash search
/*			if (flags & find_flags_t::TRANSFROM_BACKSLASH) {
				const char *error_message;
				if (!parseEscapes(searchFor, &error_message)) {
					string message(error_message);
					last_find.flags = 0;
					activate_window(WindowID::ERROR_DIALOG, &message);
					return true;
				}
			}*/

			if (flags & find_flags_t::ICASE) {
				char *folded;
				size_t folded_size;
				folded_size = t3_casefold(searchFor.data(), searchFor.size(), &folded, NULL, t3_false);
				last_find.matcher = new string_matcher_t(folded, folded_size);
			} else {
				last_find.matcher = new string_matcher_t(searchFor);
			}
		}

		if (replacement != NULL)
			last_find.replacement = new line_t(*replacement);
	}

	start = idx = get_real_line(cursor.line);

	// Perform search
	if (last_find.flags & find_flags_t::BACKWARD) {
		result.start = (last_find.flags & find_flags_t::NEXT) && selection_mode != selection_mode_t::NONE ? selection_start.pos : cursor.pos;
		result.end = 0;
		if (lines[idx]->find(&last_find, &result)) {
			set_selection_from_find(idx, &result);
			return true;
		}

		result.start = INT_MAX;
		for (; idx > 0; ) {
			idx--;
			if (lines[idx]->find(&last_find, &result)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}

		if (!(last_find.flags & find_flags_t::WRAP))
			return false;

		result.start = INT_MAX;
		for (idx = lines.size(); idx > start; ) {
			idx--;
			if (lines[idx]->find(&last_find, &result)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}
	} else {
		result.start = cursor.pos;
		result.end = INT_MAX;
		if (lines[idx]->find(&last_find, &result)) {
			set_selection_from_find(idx, &result);
			return true;
		}

		result.start = 0;
		for (idx++; idx < lines.size(); idx++) {
			if (lines[idx]->find(&last_find, &result)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}

		if (!(last_find.flags & find_flags_t::WRAP))
			return false;

		for (idx = 0; idx <= start; idx++) {
			if (lines[idx]->find(&last_find, &result)) {
				set_selection_from_find(idx, &result);
				return true;
			}
		}
	}

	return false;
}

void text_file_t::replace(void) {
	//~ lprintf("flags: %02X, repl: %s\n", last_find.flags, last_find.replacement == NULL ? (char *) NULL : last_find.replacement->getData()->c_str());
	if (!(last_find.flags & find_flags_t::VALID) || last_find.replacement == NULL)
			return;

	#warning FIXME: regexes may have references to found text!!
	replace_selection(last_find.replacement);
}

bool text_file_t::get_bullet_status(void) {
	return window != NULL;
}

const char *text_file_t::get_name(void) const {
	return name;
}
