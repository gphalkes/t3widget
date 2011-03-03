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
/* _XOPEN_SOURCE is defined to enable wcswidth. */
#define _XOPEN_SOURCE

#include <cstring>
//~ #include <cwctype>
//~ #include <cwchar>
#include <cctype>
#include <cerrno>
#include <unistd.h>

#include "options.h"
#include "lines.h"
#include "log.h"
#include "util.h"
#include "undo.h"
#include "main.h"
#include "stringmatcher.h"
#include "casefold.h"
#include "unicode/unicode.h"

char line_t::spaces[MAX_TAB];
char line_t::dots[16];
const char *line_t::control_map = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]_^";

/* Meta Information for each character:
  - screen width of character [2 bits]
  - is graphic char [1 bit]
  - is alnum char [1 bit]
  - is space char [1 bit]
  - bad print, i.e. terminal may not be able to draw properly
*/

#define WIDTH_MASK (T3_WIDTH_MASK)
#define GRAPH_BIT (T3_GRAPH_BIT)
#define ALNUM_BIT (T3_ALNUM_BIT)
#define SPACE_BIT (T3_SPACE_BIT)
#define BAD_DRAW_BIT (T3_NFC_QC_BIT)
/* Get metadata for a single byte character. */
char line_t::get_s_b_c_meta_data(key_t c) {
	char meta_data;

	ASSERT(!UTF8mode);

	meta_data = c < 32 && c != '\t' ? 2 : 1;

	if (isgraph(c)) {
		meta_data |= GRAPH_BIT;
		if (isalnum(c))
			meta_data |= ALNUM_BIT;
	} else if (isspace(c)) {
		meta_data |= SPACE_BIT;
	}
	return meta_data;
}


char line_t::conversion_buffer[5];
int line_t::conversion_length;
char line_t::conversion_meta_data;

/* 5 WCS chars should be enough because in any UTF encoding no more
   characters are required than the 5 for the UTF-8 encoding. */
#define MAX_WCS_CHARS 5

/** Convert one UCS-4 character to UTF-8.
	@param c The character to convert.
	@return the number of @a char's written.

	This function does not check for high/low surrogates.
*/
void line_t::convertKey(key_t c) {
	if (UTF8mode) {
		int i;

		conversion_meta_data = t3_get_codepoint_info(c);
		/* Mask out only what we need. */
		conversion_meta_data &= (WIDTH_MASK | GRAPH_BIT | ALNUM_BIT | SPACE_BIT);

		//FIXME: instead of doing a conversion here, would it not be better to change the
		// rest of the code? On the other hand, the control chars < 32 need special treatment anyway.
		/* Convert width as returned by t3_get_codepoint_info to what we need locally. */
		if ((conversion_meta_data & WIDTH_MASK) == 0) {
			int width = c < 32 && c != '\t' ? 2 : 1;
			conversion_meta_data = (conversion_meta_data & ~WIDTH_MASK) | width;
			// Just making sure...
			conversion_meta_data &= ~(GRAPH_BIT | SPACE_BIT);
		} else {
			conversion_meta_data--;
		}

		if (c > 0x10000L)
			conversion_length = 4;
		else if (c > 0x800)
			conversion_length = 3;
		else if (c > 0x80)
			conversion_length = 2;
		else {
			conversion_buffer[0] = c;
			conversion_buffer[1] = 0;
			conversion_length = 1;
		}

		if (conversion_length > 1) {
			for (i = conversion_length - 1; i >= 0; i--) {
				conversion_buffer[i] = (c & 0x3F) | 0x80;
				c >>= 6;
			}
			conversion_buffer[0] |= 0xF0 << (4 - conversion_length);
			conversion_buffer[conversion_length] = 0;
		}
	} else {
		conversion_buffer[0] = c;
		conversion_length = 1;
		conversion_meta_data = get_s_b_c_meta_data(c);
	}
}

line_t::line_t(int buffersize) : starts_with_combining(false) {
	reserve(buffersize);
}

void line_t::fillLine(const char *_buffer, int length) {
	if (length == -1)
		length = strlen(_buffer);

	reserve(length);

	if (UTF8mode) {
		UTF8StringInput input(_buffer, length);
		key_t next;

		//FIXME: can't we do this some quicker/better way?
		while ((next = getNextUTF8Char(&input)) != UEOF) {
			//FIXME: message to user, other charset etc.
			if (next >= 0)
				append_char(next, NULL);
		}
	} else {
		int i;

		buffer.assign(_buffer, length);

		for (i = 0; i < length; i++)
			meta_buffer.push_back(get_s_b_c_meta_data((unsigned char) buffer[i]));
	}
}

line_t::line_t(const char *_buffer, int length) : starts_with_combining(false) {
	fillLine(_buffer, length);
}

line_t::line_t(const string *str) : starts_with_combining(false) {
	fillLine(str->data(), str->size());
}

/* Merge line2 into line1, freeing line2 */
void line_t::merge(line_t *other) {
	int buffer_len = buffer.size();

	if (buffer_len == 0 && other->starts_with_combining)
		starts_with_combining = true;

	reserve(buffer.size() + other->buffer.size());

	buffer += other->buffer;
	meta_buffer += other->meta_buffer;

	if (other->starts_with_combining)
		checkBadDraw(adjust_position(buffer_len, -1));

	delete other;
}

/* Break up 'line' at position 'pos'. This means that the character at 'pos'
   is the first character in the new line. The right part of the line is
   returned the left part remains in 'line' */
line_t *line_t::break_line(int pos) {
	line_t *newline;

	#warning FIXME: cutLine and break_line are very similar. Maybe we should combine them!

	/* Only allow line breaks at non-combining marks. */
	ASSERT(meta_buffer[pos] & WIDTH_MASK);

	/* copy the right part of the string into the new buffer */
	newline = new line_t(buffer.size() - pos);
	newline->buffer.assign(buffer.data() + pos, buffer.size() - pos);
	newline->meta_buffer.assign(meta_buffer.data() + pos, meta_buffer.size() - pos);

	buffer.resize(pos);
	meta_buffer.resize(pos);
	return newline;
}

line_t *line_t::cutLine(int start, int end) {
	line_t *retval;

	ASSERT((size_t) end == buffer.size() || widthAt(end) > 0);
	//FIXME: special case: if the selection cover a whole line_t (note: not wrapped) we shouldn't copy

	retval = clone(start, end);

	buffer.erase(start, end - start);
	meta_buffer.erase(start, end - start);
	checkBadDraw(adjust_position(start, -1));

	starts_with_combining = buffer.size() > 0 && (meta_buffer[0] & WIDTH_MASK) == 0;

	return retval;
}

line_t *line_t::clone(int start, int end) {
	line_t *retval;

	if (end == -1)
		end = buffer.size();

	ASSERT((size_t) end <= buffer.size());
	ASSERT(start >= 0);
	ASSERT(start <= end);

	if (start == end)
		return new line_t(0);

	retval = new line_t(end - start);

	retval->buffer.assign(buffer.data() + start, end - start);
	retval->meta_buffer.assign(meta_buffer.data() + start, end - start);
	if ((retval->meta_buffer[0] & WIDTH_MASK) == 0)
		retval->starts_with_combining = true;

	return retval;
}

line_t *line_t::breakOnNL(int *startFrom) {
	line_t *retval;
	int i;

	for (i = *startFrom; (size_t) i < buffer.size(); i++) {
		if (buffer[i] == '\n')
			break;
	}

	retval = clone(*startFrom, i);

	*startFrom = (size_t) i == buffer.size() ? *startFrom = -1 : i + 1;
	return retval;
}

void line_t::minimize(void) {
	reserve(0);
}

/* Calculate the screen width of the characters from 'start' to 'pos' with tabsize 'tabsize' */
/* tabsize == 0 -> tab as control */
int line_t::calculateScreenWidth(int start, int pos, int tabsize) const {
	int i, total = 0;

	if (starts_with_combining && start == 0 && pos > 0)
		total++;

	for (i = start; (size_t) i < buffer.size() && i < pos; i += byteWidthFromFirst(i)) {
		if (buffer[i] == '\t')
			total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
		else
			total += widthAt(i);
	}

	return total;
}

/* Return the line position in line_t associated with screen position 'pos' or
   length if it is outside of 'line'. */
int line_t::calculate_line_pos(int start, int max, int pos, int tabsize) const {
	int i, total = 0;

	if (pos == 0)
		return start;

	for (i = start; (size_t) i < buffer.size() && i < max; i += byteWidthFromFirst(i)) {
		if (buffer[i] == '\t')
			total += tabsize - (total % tabsize);
		else
			total += widthAt(i);

		if (total > pos)
			return i;
	}

	return min(max, get_length());
}


void line_t::paintpart(t3_window_t *win, const char *paintBuffer, bool isPrint, int todo, t3_attr_t selectionAttr) {
	if (todo <= 0)
		return;

	if (isPrint) {
		t3_win_addnstr(win, paintBuffer, todo, selectionAttr);
	} else {
		for (; (size_t) todo > sizeof(dots); todo -= sizeof(dots))
			t3_win_addnstr(win, dots, sizeof(dots), option.non_print_attrs | selectionAttr);
		t3_win_addnstr(win, dots, todo, option.non_print_attrs | selectionAttr);
	}
}

t3_attr_t line_t::getDrawAttrs(int i, const line_t::paint_info_t *info) const {
	t3_attr_t retval = info->normal_attr;

	if (i >= info->selection_start && i < info->selection_end)
		retval = i == info->cursor ? option.attr_selection_cursor2 : info->selected_attr;
	else if (i == info->cursor)
		retval = i == info->selection_end ? option.attr_selection_cursor : option.attr_cursor;

	if (isBadDraw(i))
		retval = t3_term_combine_attrs(option.attr_bad_draw, retval);

	//FIXME: also take highlighting into account
	return retval;
}

void line_t::paint_line(t3_window_t *win, const line_t::paint_info_t *info) const {
	int i, j,
		total = 0,
		printFrom,
		tabspaces,
		accumulated = 0,
		endchars = 0;
	bool _isPrint,
		newIsPrint;
	t3_attr_t selectionAttr = 0, newSelectionAttr;
	int flags = info->flags,
		size = info->size,
		selection_start = info->selection_start,
		selection_end = info->selection_end;

	if (info->tabsize == 0)
		flags |= line_t::TAB_AS_CONTROL;

	/* For reverse selections (end before info->start) we swap the info->start and end for rendering */
	if (selection_start > selection_end) {
		int tmp;
		tmp = selection_start;
		selection_start = selection_end;
		selection_end = tmp;
	}

	size += info->leftcol;
	if (flags & line_t::BREAK)
		size--;

	if (starts_with_combining && info->leftcol > 0 && info->start == 0)
		total++;

	for (i = info->start; (size_t) i < buffer.size() && i < info->max && total < info->leftcol; i += byteWidthFromFirst(i)) {
		selectionAttr = getDrawAttrs(i, info);

		if (buffer[i] == '\t' && !(flags & line_t::TAB_AS_CONTROL)) {
			tabspaces = info->tabsize - (total % info->tabsize);
			total += tabspaces;
			if (total >= size)
				total = size;
			if (total > info->leftcol)
				t3_win_addnstr(win, spaces, total - info->leftcol, selectionAttr);
		} else if ((unsigned char) buffer[i] < 32) {
			total += 2;
			// If total > info->leftcol than only the right side character is visible
			if (total > info->leftcol)
				t3_win_addch(win, control_map[(int) buffer[i]], option.non_print_attrs | selectionAttr);
		} else if (widthAt(i) > 1) {
			total += widthAt(i);
			if (total > info->leftcol) {
				for (j = info->leftcol; j < total; j++)
					t3_win_addch(win, '<',  option.non_print_attrs | selectionAttr);
			}
		} else {
			total += widthAt(i);
		}
	}

	if (starts_with_combining && info->leftcol == 0 && info->start == 0) {
		paintpart(win, " ", true, 1, option.non_print_attrs | selectionAttr);
		accumulated++;
	} else {
		/* Skip to first non-zero-width char */
		while ((size_t) i < buffer.size() && i < info->max && widthAt(i) == 0)
			i += byteWidthFromFirst(i);
	}

	_isPrint = isPrint(i);
	printFrom = i;
	for (; (size_t) i < buffer.size() && i < info->max && total + accumulated < size; i += byteWidthFromFirst(i)) {
		/* If selection changed between this char and the previous, print what
		   we had so far. */
		newSelectionAttr = getDrawAttrs(i, info);

		if (newSelectionAttr != selectionAttr) {
			paintpart(win, buffer.data() + printFrom, _isPrint, _isPrint ? i - printFrom : accumulated, selectionAttr);
			total += accumulated;
			accumulated = 0;
			printFrom = i;
		}
		selectionAttr = newSelectionAttr;

		newIsPrint = isPrint(i);
		if (buffer[i] == '\t' && !(flags & line_t::TAB_AS_CONTROL)) {
			/* Calculate the correct number of spaces for a tab character. */
			paintpart(win, buffer.data() + printFrom, _isPrint, _isPrint ? i - printFrom : accumulated, selectionAttr);
			total += accumulated;
			accumulated = 0;
			tabspaces = info->tabsize - (total % info->tabsize);
			if (total + tabspaces >= size)
				tabspaces = size - total;
			t3_win_addnstr(win, spaces, tabspaces, selectionAttr);
			total += tabspaces;
			printFrom = i + 1;
		} else if ((unsigned char) buffer[i] < 32) {
			/* Print control characters as a dot with special markup. */
			paintpart(win, buffer.data() + printFrom, _isPrint, _isPrint ? i - printFrom : accumulated, selectionAttr);
			total += accumulated;
			accumulated = 0;
			t3_win_addch(win, '^', option.non_print_attrs | selectionAttr);
			total += 2;
			if (total <= size)
				t3_win_addch(win, control_map[(int) buffer[i]], option.non_print_attrs | selectionAttr);
			printFrom = i + 1;
		} else if (_isPrint != newIsPrint) {
			/* Print part of the buffer as either printable or control characters, because
			   the next character is in the other category. */
			paintpart(win, buffer.data() + printFrom, _isPrint, _isPrint ? i - printFrom : accumulated, selectionAttr);
			total += accumulated;
			accumulated = widthAt(i);
			printFrom = i;
		} else {
			/* Take care of double width characters that cross the right screen edge. */
			if (total + accumulated + widthAt(i) > size) {
				endchars = size - total + accumulated;
				break;
			}
			accumulated += widthAt(i);
		}
		_isPrint = newIsPrint;
	}
	while ((size_t) i < buffer.size() && i < info->max && widthAt(i) == 0)
		i += byteWidthFromFirst(i);

	paintpart(win, buffer.data() + printFrom, _isPrint, _isPrint ? i - printFrom : accumulated, selectionAttr);
	total += accumulated;

	if ((flags & line_t::PARTIAL_CHAR) && i >= info->max)
		endchars = 1;

	for (j = 0; j < endchars; j++)
		t3_win_addch(win, '>', option.non_print_attrs | selectionAttr);
	total += endchars;

	/* Add a selected space when the selection crosses the end of this line
	   and this line is not merely a part of a broken line */
	if (total < size && !(flags & line_t::BREAK)) {
		if (i <= selection_end || i == info->cursor) {
			t3_win_addch(win, ' ', getDrawAttrs(i, info));
			total++;
		}
	}

	if (flags & line_t::BREAK) {
		for (; total < size; total++)
			t3_win_addch(win, ' ', info->normal_attr);
		paintWrapSymbol(win);
	} else if (flags & line_t::SPACECLEAR) {
		for (; total + sizeof(spaces) < (size_t) size; total += sizeof(spaces))
			t3_win_addnstr(win, spaces, sizeof(spaces), (flags & line_t::EXTEND_SELECTION) ? info->selected_attr : info->normal_attr);
		t3_win_addnstr(win, spaces, size - total, (flags & line_t::EXTEND_SELECTION) ? info->selected_attr : info->normal_attr);
	} else {
		t3_win_clrtoeol(win);
	}
}

void line_t::paintWrapSymbol(t3_window_t *win) {
	if (UTF8mode)
		t3_win_addnstr(win, "\xE2\x86\xB5", 3, option.non_print_attrs);
	else
		t3_win_addch(win, '$', option.non_print_attrs);
}


/* FIXME:
Several things need to be done:
  - check what to do with no-break space etc. (especially zero-width no-break
	space and zero-width space)
  - should control characters break both before and after?
*/
/* tabsize == 0 -> tab as control */
break_pos_t line_t::findNextBreakPos(int start, int length, int tabsize) const {
	int i, total = 0;
	break_pos_t possible_break = { 0, 0 };
	bool graph_seen = false, last_was_graph = false;

	if (starts_with_combining && start == 0)
		total++;

	for (i = start; (size_t) i < buffer.size() && total < length; i = adjust_position(i, 1)) {
		if (buffer[i] == '\t')
			total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
		else
			total += widthAt(i);

		if (total > length && (buffer[i] != '\t' || tabsize == 0)) {
			if (possible_break.pos == 0)
				possible_break.flags = line_t::PARTIAL_CHAR;
			break;
		}

		if (!graph_seen) {
			if (isGraph(i) || buffer[i] < 32) {
				graph_seen = true;
				last_was_graph = true;
			} else {
				possible_break.pos = i;
			}
		} else if (isSpace(i) && last_was_graph) {
			possible_break.pos = adjust_position(i, 1);
			last_was_graph = false;
		} else if (isGraph(i) || buffer[i] < 32) {
			if (last_was_graph && !isAlnum(i))
				possible_break.pos = adjust_position(i, 1);
			last_was_graph = true;
		}
	}

	if ((size_t) i < buffer.size()) {
		if (possible_break.pos == 0)
			possible_break.pos = i;
		possible_break.flags |= line_t::BREAK;
		return possible_break;
	}
	possible_break.flags = 0;
	possible_break.pos = -1;
	return possible_break;
}

enum {
	CLASS_WHITESPACE,
	CLASS_ALNUM,
	CLASS_GRAPH,
	CLASS_OTHER
};

int line_t::getClass(int pos) const {
	if (isSpace(pos))
		return CLASS_WHITESPACE;
	if (isAlnum(pos))
		return CLASS_ALNUM;
	if (isGraph(pos))
		return CLASS_GRAPH;
	return CLASS_OTHER;
}

int line_t::get_next_word(int start) const {
	int i, cclass, newCclass;

	if (start < 0) {
		start = 0;
		cclass = CLASS_WHITESPACE;
	} else {
		cclass = getClass(start);
		start = adjust_position(start, 1);
	}

	for (i = start;
			(size_t) i < buffer.size() && ((newCclass = getClass(i)) == cclass || newCclass == CLASS_WHITESPACE);
			i = adjust_position(i, 1))
	{
		cclass = newCclass;
	}

	return (size_t) i >= buffer.size() ? -1 : i;
}

int line_t::get_previous_word(int start) const {
	int i, cclass = CLASS_WHITESPACE, savePos;

	if (start == 0)
		return -1;

	if (start < 0)
		start = buffer.size();

	for (i = adjust_position(start, -1);
			i > 0 && (cclass = getClass(i)) == CLASS_WHITESPACE;
			i = adjust_position(i, -1))
	{}

	if (i == 0 && cclass == CLASS_WHITESPACE)
		return -1;

	savePos = i;

	for (i = adjust_position(i, -1);
			i > 0 && getClass(i) == cclass;
			i = adjust_position(i, -1))
	{
		savePos = i;
	}

	if (i == 0 && getClass(i) == cclass)
		savePos = i;

	return cclass != CLASS_WHITESPACE ? savePos : -1;
}


void line_t::insertBytes(int pos, const char *bytes, int space, char meta_data) {
	buffer.insert(pos, bytes, space);
	meta_buffer.insert(pos, space, 0);
	meta_buffer[pos] = meta_data;
}


/* Insert character 'c' into 'line' at position 'pos' */
bool line_t::insert_char(int pos, key_t c, Undo *undo) {
	convertKey(c);

	reserve(buffer.size() + conversion_length + 1);

	if (undo != NULL) {
		line_t *undoText = undo->get_text();
		undoText->reserve(undoText->buffer.size() + conversion_length);

		ASSERT(undo->getType() == UNDO_ADD);
		undoText->insertBytes(undoText->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
	}

	if (pos == 0) {
		if ((conversion_meta_data & WIDTH_MASK) == 0)
			starts_with_combining = true;
		else if (starts_with_combining)
			starts_with_combining = false;
	}

	insertBytes(pos, conversion_buffer, conversion_length, conversion_meta_data);
	checkBadDraw(adjust_position(pos, -1));

	return true;
}

/* Overwrite a character with 'c' in 'line' at position 'pos' */
bool line_t::overwrite_char(int pos, key_t c, Undo *undo) {
	int oldspace;
	line_t *undoText, *replacementText;

	convertKey(c);

	/* Zero-width characters don't overwrite, only insert. */
	if ((conversion_meta_data & WIDTH_MASK) == 0) {
		if (pos == 0)
			return false;

		if (undo != NULL) {
			ASSERT(undo->getType() == UNDO_OVERWRITE);
			replacementText = undo->getReplacement();
			replacementText->reserve(replacementText->buffer.size() + conversion_length);
			replacementText->insertBytes(replacementText->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
		}
		return insert_char(pos, c, NULL);
	}

	if (starts_with_combining && pos == 0)
		starts_with_combining = false;

	oldspace = adjust_position(pos, 1) - pos;
	if (oldspace < conversion_length)
		reserve(buffer.size() + conversion_length - oldspace);

	if (undo != NULL) {
		ASSERT(undo->getType() == UNDO_OVERWRITE);
		undoText = undo->get_text();
		undoText->reserve(undoText->buffer.size() + oldspace);
		replacementText = undo->getReplacement();
		replacementText->reserve(replacementText->buffer.size() + conversion_length);

		undoText->insertBytes(undoText->buffer.size(), buffer.data() + pos, oldspace, meta_buffer[pos]);
		replacementText->insertBytes(replacementText->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
	}

	buffer.replace(pos, oldspace, conversion_buffer, conversion_length);
	meta_buffer.replace(pos, oldspace, conversion_length, 0);
	meta_buffer[pos] = conversion_meta_data;
	checkBadDraw(pos);
	return true;
}

/* Delete the character at position 'pos' */
int line_t::delete_char(int pos, Undo *undo) {
	int oldspace;

	if (pos < 0 || (size_t) pos >= buffer.size())
		return -1;

	if (starts_with_combining && pos == 0)
		starts_with_combining = false;

	oldspace = adjust_position(pos, 1) - pos;

	if (undo != NULL) {
		line_t *undoText = undo->get_text();
		undoText->reserve(oldspace);

		ASSERT(undo->getType() == UNDO_DELETE || undo->getType() == UNDO_BACKSPACE);
		undoText->insertBytes(undo->getType() == UNDO_DELETE ? undoText->buffer.size() : 0, buffer.data() + pos, oldspace, meta_buffer[pos]);
	}

	buffer.erase(pos, oldspace);
	meta_buffer.erase(pos, oldspace);

	return 0;
}

/* Append character 'c' to 'line' */
bool line_t::append_char(key_t c, Undo *undo) {
	return insert_char(buffer.size(), c, undo);
}

/* Delete char at 'pos - 1' */
int line_t::backspace_char(int pos, Undo *undo) {
	if (pos == 0)
		return -1;
	return delete_char(adjust_position(pos, -1), undo);
}

void line_t::writeLineData(FileWriteWrapper *file, bool appendNewline) const {
	file->write(buffer.data(), buffer.size());

	if (appendNewline)
		file->write("\n", 1);
}

/** Adjust the line position @a adjust non-zero-width characters.
	@param line The @a line_t to operate on.
	@param pos The starting position.
	@param adjust How many characters to adjust.

	This function finds the next (previous) point in the line at which the
    cursor could be. This means skipping all zero-width characters between the
	current position and the next non-zero-width character, and repeating for
	@a adjust times.
*/
int line_t::adjust_position(int pos, int adjust) const {
	if (adjust > 0) {
		for (; adjust > 0 && (size_t) pos < buffer.size(); adjust -= (widthAt(pos) ? 1 : 0))
			pos += byteWidthFromFirst(pos);
	} else {
		for (; adjust < 0 && pos > 0; adjust += (widthAt(pos) ? 1 : 0)) {
			pos--;
			while (pos > 0 && !widthAt(pos))
				pos--;
		}
	}
	return pos;
}

int line_t::get_length(void) const {
	return buffer.size();
}

int line_t::byteWidthFromFirst(int pos) const {
	if (UTF8mode) {
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
	} else {
		return 1;
	}
}

int line_t::widthAt(int pos) const { return meta_buffer[pos] & WIDTH_MASK; }
int line_t::isPrint(int pos) const { return (meta_buffer[pos] & (GRAPH_BIT | SPACE_BIT)) != 0; }
int line_t::isGraph(int pos) const { return meta_buffer[pos] & GRAPH_BIT; }
int line_t::isAlnum(int pos) const { return meta_buffer[pos] & ALNUM_BIT; }
int line_t::isSpace(int pos) const { return meta_buffer[pos] & SPACE_BIT; }
int line_t::isBadDraw(int pos) const { return meta_buffer[pos] & BAD_DRAW_BIT; }

const string *line_t::getData(void) {
	return &buffer;
}

void line_t::init(void) {
	memset(spaces, ' ', sizeof(spaces));
	memset(dots, '.', sizeof(dots));
}

void line_t::reserve(int size) {
	buffer.reserve(size);
	meta_buffer.reserve(size);
}

int line_t::callout(pcre_callout_block *block) {
	find_context_t *context = (find_context_t *) block->callout_data;
	if (block->pattern_position != context->pattern_length)
		return 0;

	if (context->flags & FindFlags::BACKWARD) {
		if (block->start_match > context->ovector[0] ||
				(block->start_match == context->ovector[0] && block->current_position > context->ovector[1])) {
			memcpy(context->ovector, block->offset_vector, block->capture_top * sizeof(int) * 2);
			context->ovector[0] = block->start_match;
			context->ovector[1] = block->current_position;
			context->captures = block->capture_top;
			context->found = true;
		}
		return 1;
	} else {
		memcpy(context->ovector, block->offset_vector, block->capture_top * sizeof(int) * 2);
		context->ovector[0] = block->start_match;
		context->ovector[1] = block->current_position;
		context->captures = block->capture_top;
		context->found = true;
		return 0;
	}
}

bool line_t::checkBoundaries(int matchStart, int matchEnd) const {
	return (matchStart == 0 || getClass(matchStart) != getClass(adjust_position(matchStart, -1))) &&
							(matchEnd == get_length() || getClass(matchEnd) != getClass(adjust_position(matchEnd, 1)));
}

bool line_t::find(find_context_t *context, find_result_t *result) const {
	string substr, folded;
	int currChar, next_char;
	int matchResult;
	int start, end;

	if (context->flags & FindFlags::REGEX) {
		pcre_extra extra;
		int pcre_flags = PCRE_NOTEMPTY;
		int ovector[30];

		context->ovector[0] = -1;
		context->ovector[1] = -1;
		context->found = false;

		if (context->flags & FindFlags::BACKWARD) {
			start = result->end;
			end = result->start;
		} else {
			start = result->start;
			end = result->end;
		}

		extra.flags = PCRE_EXTRA_CALLOUT_DATA;
		extra.callout_data = context;
		//FIXME: doesn't work for backward!
		if ((size_t) end >= buffer.size())
			end = buffer.size();
		else
			pcre_flags |= PCRE_NOTEOL;

		if (start != 0)
			pcre_flags |= PCRE_NOTBOL;

		pcre_callout = callout;
		matchResult = pcre_exec(context->regex, &extra, buffer.data(), end, start, pcre_flags,
			(int *) ovector, sizeof(ovector) / sizeof(ovector[0]));
		if (!context->found)
			return false;
		result->start = context->ovector[0];
		result->end = context->ovector[1];
		return true;
	}

	start = result->start >= 0 && (size_t) result->start > buffer.size() ? buffer.size() : result->start;
	currChar = start;

	if (context->flags & FindFlags::BACKWARD) {
		context->matcher->reset();
		while((size_t) currChar > 0) {
			next_char = adjust_position(currChar, -1);
			substr.clear();
			substr = buffer.substr(next_char, currChar - next_char);
			if (context->flags & FindFlags::ICASE) {
				folded.clear();
				case_fold_t::fold(&substr, &folded);
				substr = folded;
			}
			matchResult = context->matcher->previous_char(&substr);
			if (matchResult >= 0 &&
					(!(context->flags & FindFlags::WHOLE_WORD) || checkBoundaries(next_char, adjust_position(start, -matchResult)))) {
				result->end = adjust_position(start, -matchResult);
				result->start = next_char;
				return true;
			}
			currChar = next_char;
		}
		return false;
	} else {
		context->matcher->reset();
		while((size_t) currChar < buffer.size()) {
			next_char = adjust_position(currChar, 1);
			substr.clear();
			substr = buffer.substr(currChar, next_char - currChar);
			if (context->flags & FindFlags::ICASE) {
				folded.clear();
				case_fold_t::fold(&substr, &folded);
				substr = folded;
			}
			matchResult = context->matcher->next_char(&substr);
			if (matchResult >= 0 &&
					(!(context->flags & FindFlags::WHOLE_WORD) || checkBoundaries(adjust_position(start, matchResult), next_char))) {
				result->start = adjust_position(start, matchResult);
				result->end = next_char;
				return true;
			}
			currChar = next_char;
		}
		return false;
	}
}

void line_t::checkBadDraw(int i) {
	if (t3_term_can_draw(buffer.data() + i, adjust_position(i, 1) - i))
		meta_buffer[i] &= ~BAD_DRAW_BIT;
	else
		meta_buffer[i] |= BAD_DRAW_BIT;
}
