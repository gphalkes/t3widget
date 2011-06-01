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

#include "colorscheme.h"
#include "textline.h"
#include "util.h"
#include "undo.h"
#include "stringmatcher.h"
#include "unicode/unicode.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

char text_line_t::spaces[MAX_TAB];
char text_line_t::dots[16];
const char *text_line_t::control_map = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]_^";
const char *text_line_t::wrap_symbol = "\xE2\x86\xB5";

/* Meta Information for each character:
  - screen width of character [2 bits]
  - is graphic char [1 bit]
  - is alnum char [1 bit]
  - is space char [1 bit]
  - bad print, i.e. terminal may not be able to draw properly
*/

#define WIDTH_MASK (T3_UNICODE_WIDTH_MASK)
#define GRAPH_BIT (T3_UNICODE_GRAPH_BIT)
#define ALNUM_BIT (T3_UNICODE_ALNUM_BIT)
#define SPACE_BIT (T3_UNICODE_SPACE_BIT)
#define BAD_DRAW_BIT (T3_UNICODE_NFC_QC_BIT)

char text_line_t::conversion_buffer[5];
int text_line_t::conversion_length;
char text_line_t::conversion_meta_data;

/* 5 WCS chars should be enough because in any UTF encoding no more
   characters are required than the 5 for the UTF-8 encoding. */
#define MAX_WCS_CHARS 5

/** Convert one UCS-4 character to UTF-8.
	@param c The character to convert.
	@return the number of @a char's written.

	This function does not check for high/low surrogates.
*/
void text_line_t::convert_key(key_t c) {
	int i;

	conversion_meta_data = t3_unicode_get_info(c, INT_MAX);
	/* Mask out only what we need. */
	conversion_meta_data &= (WIDTH_MASK | GRAPH_BIT | ALNUM_BIT | SPACE_BIT);

	//FIXME: instead of doing a conversion here, would it not be better to change the
	// rest of the code? On the other hand, the control chars < 32 need special treatment anyway.
	/* Convert width as returned by t3_unicode_get_info to what we need locally. */
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
}

text_line_t::text_line_t(int buffersize) : starts_with_combining(false) {
	reserve(buffersize);
}

void text_line_t::fill_line(const char *_buffer, int length) {
	size_t char_bytes;
	key_t next;

	reserve(length);

	while (length > 0) {
		char_bytes = length;
		next = t3_unicode_get(_buffer, &char_bytes);
		append_char(next, NULL);
		length -= char_bytes;
		_buffer += char_bytes;
	}

	reserve(length);
}

text_line_t::text_line_t(const char *_buffer) : starts_with_combining(false) {
	fill_line(_buffer, strlen(_buffer));
}

text_line_t::text_line_t(const char *_buffer, int length) : starts_with_combining(false) {
	fill_line(_buffer, length);
}

text_line_t::text_line_t(const string *str) : starts_with_combining(false) {
	fill_line(str->data(), str->size());
}


void text_line_t::set_text(const char *_buffer) {
	buffer.clear();
	meta_buffer.clear();
	fill_line(_buffer, strlen(_buffer));
}

void text_line_t::set_text(const char *_buffer, size_t length) {
	buffer.clear();
	meta_buffer.clear();
	fill_line(_buffer, length);
}

void text_line_t::set_text(const string *str) {
	buffer.clear();
	meta_buffer.clear();
	fill_line(str->data(), str->size());
}

/* Merge line2 into line1, freeing line2 */
void text_line_t::merge(text_line_t *other) {
	int buffer_len = buffer.size();

	if (buffer_len == 0 && other->starts_with_combining)
		starts_with_combining = true;

	reserve(buffer.size() + other->buffer.size());

	buffer += other->buffer;
	meta_buffer += other->meta_buffer;

	if (other->starts_with_combining)
		check_bad_draw(adjust_position(buffer_len, -1));

	delete other;
}

/* Break up 'line' at position 'pos'. This means that the character at 'pos'
   is the first character in the new line. The right part of the line is
   returned the left part remains in 'line' */
text_line_t *text_line_t::break_line(int pos) {
	text_line_t *newline;

	//FIXME: cut_line and break_line are very similar. Maybe we should combine them!
	if (pos == buffer.size())
		return new text_line_t();

	/* Only allow line breaks at non-combining marks. */
	ASSERT(meta_buffer[pos] & WIDTH_MASK);

	/* copy the right part of the string into the new buffer */
	newline = new text_line_t(buffer.size() - pos);
	newline->buffer.assign(buffer.data() + pos, buffer.size() - pos);
	newline->meta_buffer.assign(meta_buffer.data() + pos, meta_buffer.size() - pos);

	buffer.resize(pos);
	meta_buffer.resize(pos);
	return newline;
}

text_line_t *text_line_t::cut_line(int start, int end) {
	text_line_t *retval;

	ASSERT((size_t) end == buffer.size() || width_at(end) > 0);
	//FIXME: special case: if the selection cover a whole text_line_t (note: not wrapped) we shouldn't copy

	retval = clone(start, end);

	buffer.erase(start, end - start);
	meta_buffer.erase(start, end - start);
	check_bad_draw(adjust_position(start, -1));

	starts_with_combining = buffer.size() > 0 && (meta_buffer[0] & WIDTH_MASK) == 0;

	return retval;
}

text_line_t *text_line_t::clone(int start, int end) {
	text_line_t *retval;

	if (end == -1)
		end = buffer.size();

	ASSERT((size_t) end <= buffer.size());
	ASSERT(start >= 0);
	ASSERT(start <= end);

	if (start == end)
		return new text_line_t(0);

	retval = new text_line_t(end - start);

	retval->buffer.assign(buffer.data() + start, end - start);
	retval->meta_buffer.assign(meta_buffer.data() + start, end - start);
	if ((retval->meta_buffer[0] & WIDTH_MASK) == 0)
		retval->starts_with_combining = true;

	return retval;
}

text_line_t *text_line_t::break_on_nl(int *startFrom) {
	text_line_t *retval;
	int i;

	for (i = *startFrom; (size_t) i < buffer.size(); i++) {
		if (buffer[i] == '\n')
			break;
	}

	retval = clone(*startFrom, i);

	*startFrom = (size_t) i == buffer.size() ? *startFrom = -1 : i + 1;
	return retval;
}

void text_line_t::minimize(void) {
	reserve(0);
}

/* Calculate the screen width of the characters from 'start' to 'pos' with tabsize 'tabsize' */
/* tabsize == 0 -> tab as control */
int text_line_t::calculate_screen_width(int start, int pos, int tabsize) const {
	int i, total = 0;

	if (starts_with_combining && start == 0 && pos > 0)
		total++;

	for (i = start; (size_t) i < buffer.size() && i < pos; i += byte_width_from_first(i)) {
		if (buffer[i] == '\t')
			total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
		else
			total += width_at(i);
	}

	return total;
}

/* Return the line position in text_line_t associated with screen position 'pos' or
   length if it is outside of 'line'. */
int text_line_t::calculate_line_pos(int start, int max, int pos, int tabsize) const {
	int i, total = 0;

	if (pos == 0)
		return start;

	for (i = start; (size_t) i < buffer.size() && i < max; i += byte_width_from_first(i)) {
		if (buffer[i] == '\t')
			total += tabsize - (total % tabsize);
		else
			total += width_at(i);

		if (total > pos)
			return i;
	}

	return min(max, get_length());
}


void text_line_t::paint_part(t3_window_t *win, const char *paint_buffer, bool is_print, int todo, t3_attr_t selection_attr) {
	if (todo <= 0)
		return;

	if (is_print) {
		t3_win_addnstr(win, paint_buffer, todo, selection_attr);
	} else {
		for (; (size_t) todo > sizeof(dots); todo -= sizeof(dots))
			t3_win_addnstr(win, dots, sizeof(dots), attributes.non_print | selection_attr);
		t3_win_addnstr(win, dots, todo, attributes.non_print | selection_attr);
	}
}

t3_attr_t text_line_t::get_draw_attrs(int i, const text_line_t::paint_info_t *info) const {
	t3_attr_t retval = info->normal_attr;

	if (i >= info->selection_start && i < info->selection_end)
		retval = i == info->cursor ? attributes.selection_cursor2 : info->selected_attr;
	else if (i == info->cursor)
		retval = i == info->selection_end ? attributes.selection_cursor : attributes.text_cursor;

	if (is_bad_draw(i))
		retval = t3_term_combine_attrs(attributes.bad_draw, retval);

	//FIXME: also take highlighting into account
	return retval;
}

void text_line_t::paint_line(t3_window_t *win, const text_line_t::paint_info_t *info) const {
	int i, j,
		total = 0,
		print_from,
		tabspaces,
		accumulated = 0,
		endchars = 0;
	bool _is_print,
		new_is_print;
	t3_attr_t selection_attr = 0, new_selection_attr;
	int flags = info->flags,
		size = info->size,
		selection_start = info->selection_start,
		selection_end = info->selection_end;

	if (info->tabsize == 0)
		flags |= text_line_t::TAB_AS_CONTROL;

	/* For reverse selections (end before info->start) we swap the info->start and end for rendering */
	if (selection_start > selection_end) {
		int tmp;
		tmp = selection_start;
		selection_start = selection_end;
		selection_end = tmp;
	}

	size += info->leftcol;
	if (flags & text_line_t::BREAK)
		size--;

	if (size < 0)
		return;

	if (starts_with_combining && info->leftcol > 0 && info->start == 0)
		total++;

	for (i = info->start; (size_t) i < buffer.size() && i < info->max && total < info->leftcol; i += byte_width_from_first(i)) {
		selection_attr = get_draw_attrs(i, info);

		if (buffer[i] == '\t' && !(flags & text_line_t::TAB_AS_CONTROL)) {
			tabspaces = info->tabsize - (total % info->tabsize);
			total += tabspaces;
			if (total >= size)
				total = size;
			if (total > info->leftcol)
				t3_win_addnstr(win, spaces, total - info->leftcol, selection_attr);
		} else if ((unsigned char) buffer[i] < 32) {
			total += 2;
			// If total > info->leftcol than only the right side character is visible
			if (total > info->leftcol)
				t3_win_addch(win, control_map[(int) buffer[i]], attributes.non_print | selection_attr);
		} else if (width_at(i) > 1) {
			total += width_at(i);
			if (total > info->leftcol) {
				for (j = info->leftcol; j < total; j++)
					t3_win_addch(win, '<',  attributes.non_print | selection_attr);
			}
		} else {
			total += width_at(i);
		}
	}

	if (starts_with_combining && info->leftcol == 0 && info->start == 0) {
		paint_part(win, " ", true, 1, attributes.non_print | selection_attr);
		accumulated++;
	} else {
		/* Skip to first non-zero-width char */
		while ((size_t) i < buffer.size() && i < info->max && width_at(i) == 0)
			i += byte_width_from_first(i);
	}

	_is_print = is_print(i);
	print_from = i;
	for (; (size_t) i < buffer.size() && i < info->max && total + accumulated < size; i += byte_width_from_first(i)) {
		/* If selection changed between this char and the previous, print what
		   we had so far. */
		new_selection_attr = get_draw_attrs(i, info);

		if (new_selection_attr != selection_attr) {
			paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
			total += accumulated;
			accumulated = 0;
			print_from = i;
		}
		selection_attr = new_selection_attr;

		new_is_print = is_print(i);
		if (buffer[i] == '\t' && !(flags & text_line_t::TAB_AS_CONTROL)) {
			/* Calculate the correct number of spaces for a tab character. */
			paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
			total += accumulated;
			accumulated = 0;
			tabspaces = info->tabsize - (total % info->tabsize);
			if (total + tabspaces >= size)
				tabspaces = size - total;
			if (i == info->cursor) {
				t3_win_addch(win, ' ', selection_attr);
				tabspaces--;
				total++;
				selection_attr = i >= info->selection_start && i < info->selection_end ? info->selected_attr : info->normal_attr;
			}
			if (tabspaces > 0)
				t3_win_addnstr(win, spaces, tabspaces, selection_attr);
			total += tabspaces;
			print_from = i + 1;
		} else if ((unsigned char) buffer[i] < 32) {
			/* Print control characters as a dot with special markup. */
			paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
			total += accumulated;
			accumulated = 0;
			t3_win_addch(win, '^', attributes.non_print | selection_attr);
			total += 2;
			if (total <= size)
				t3_win_addch(win, control_map[(int) buffer[i]], attributes.non_print | selection_attr);
			print_from = i + 1;
		} else if (_is_print != new_is_print) {
			/* Print part of the buffer as either printable or control characters, because
			   the next character is in the other category. */
			paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
			total += accumulated;
			accumulated = width_at(i);
			print_from = i;
		} else {
			/* Take care of double width characters that cross the right screen edge. */
			if (total + accumulated + width_at(i) > size) {
				endchars = size - total + accumulated;
				break;
			}
			accumulated += width_at(i);
		}
		_is_print = new_is_print;
	}
	while ((size_t) i < buffer.size() && i < info->max && width_at(i) == 0)
		i += byte_width_from_first(i);

	paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
	total += accumulated;

	if ((flags & text_line_t::PARTIAL_CHAR) && i >= info->max)
		endchars = 1;

	for (j = 0; j < endchars; j++)
		t3_win_addch(win, '>', attributes.non_print | selection_attr);
	total += endchars;

	/* Add a selected space when the selection crosses the end of this line
	   and this line is not merely a part of a broken line */
	if (total < size && !(flags & text_line_t::BREAK)) {
		if (i <= selection_end || i == info->cursor) {
			t3_win_addch(win, ' ', get_draw_attrs(i, info));
			total++;
		}
	}

	if (flags & text_line_t::BREAK) {
		for (; total < size; total++)
			t3_win_addch(win, ' ', info->normal_attr);
		t3_win_addnstr(win, wrap_symbol, 3, attributes.non_print);
	} else if (flags & text_line_t::SPACECLEAR) {
		for (; total + sizeof(spaces) < (size_t) size; total += sizeof(spaces))
			t3_win_addnstr(win, spaces, sizeof(spaces), (flags & text_line_t::EXTEND_SELECTION) ? info->selected_attr : info->normal_attr);
		t3_win_addnstr(win, spaces, size - total, (flags & text_line_t::EXTEND_SELECTION) ? info->selected_attr : info->normal_attr);
	} else {
		t3_win_clrtoeol(win);
	}
}


/* FIXME:
Several things need to be done:
  - check what to do with no-break space etc. (especially zero-width no-break
	space and zero-width space)
  - should control characters break both before and after?
*/
/* tabsize == 0 -> tab as control */
break_pos_t text_line_t::find_next_break_pos(int start, int length, int tabsize) const {
	int i, total = 0;
	break_pos_t possible_break = { 0, 0 };
	bool graph_seen = false, last_was_graph = false;

	if (starts_with_combining && start == 0)
		total++;

	for (i = start; (size_t) i < buffer.size() && total < length; i = adjust_position(i, 1)) {
		if (buffer[i] == '\t')
			total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
		else
			total += width_at(i);

		if (total > length && (buffer[i] != '\t' || tabsize == 0)) {
			if (possible_break.pos == 0)
				possible_break.flags = text_line_t::PARTIAL_CHAR;
			break;
		}

		if (!graph_seen) {
			if (is_graph(i) || buffer[i] < 32) {
				graph_seen = true;
				last_was_graph = true;
			} else {
				possible_break.pos = i;
			}
		} else if (is_space(i) && last_was_graph) {
			possible_break.pos = adjust_position(i, 1);
			last_was_graph = false;
		} else if (is_graph(i) || buffer[i] < 32) {
			if (last_was_graph && !is_alnum(i) && buffer[i] != '_')
				possible_break.pos = adjust_position(i, 1);
			last_was_graph = true;
		}
	}

	if ((size_t) i < buffer.size()) {
		if (possible_break.pos == 0)
			possible_break.pos = i;
		possible_break.flags |= text_line_t::BREAK;
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

int text_line_t::get_class(int pos) const {
	if (is_space(pos))
		return CLASS_WHITESPACE;
	if (is_alnum(pos))
		return CLASS_ALNUM;
	if (is_graph(pos))
		return CLASS_GRAPH;
	return CLASS_OTHER;
}

int text_line_t::get_next_word(int start) const {
	int i, cclass, newCclass;

	if (start < 0) {
		start = 0;
		cclass = CLASS_WHITESPACE;
	} else {
		cclass = get_class(start);
		start = adjust_position(start, 1);
	}

	for (i = start;
			(size_t) i < buffer.size() && ((newCclass = get_class(i)) == cclass || newCclass == CLASS_WHITESPACE);
			i = adjust_position(i, 1))
	{
		cclass = newCclass;
	}

	return (size_t) i >= buffer.size() ? -1 : i;
}

int text_line_t::get_previous_word(int start) const {
	int i, cclass = CLASS_WHITESPACE, savePos;

	if (start == 0)
		return -1;

	if (start < 0)
		start = buffer.size();

	for (i = adjust_position(start, -1);
			i > 0 && (cclass = get_class(i)) == CLASS_WHITESPACE;
			i = adjust_position(i, -1))
	{}

	if (i == 0 && cclass == CLASS_WHITESPACE)
		return -1;

	savePos = i;

	for (i = adjust_position(i, -1);
			i > 0 && get_class(i) == cclass;
			i = adjust_position(i, -1))
	{
		savePos = i;
	}

	if (i == 0 && get_class(i) == cclass)
		savePos = i;

	return cclass != CLASS_WHITESPACE ? savePos : -1;
}


void text_line_t::insert_bytes(int pos, const char *bytes, int space, char meta_data) {
	buffer.insert(pos, bytes, space);
	meta_buffer.insert(pos, space, 0);
	meta_buffer[pos] = meta_data;
}


/* Insert character 'c' into 'line' at position 'pos' */
bool text_line_t::insert_char(int pos, key_t c, undo_t *undo) {
	convert_key(c);

	reserve(buffer.size() + conversion_length + 1);

	if (undo != NULL) {
		text_line_t *undo_text = undo->get_text();
		undo_text->reserve(undo_text->buffer.size() + conversion_length);

		ASSERT(undo->get_type() == UNDO_ADD);
		undo_text->insert_bytes(undo_text->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
	}

	if (pos == 0) {
		if ((conversion_meta_data & WIDTH_MASK) == 0)
			starts_with_combining = true;
		else if (starts_with_combining)
			starts_with_combining = false;
	}

	insert_bytes(pos, conversion_buffer, conversion_length, conversion_meta_data);
	check_bad_draw(adjust_position(pos, 0));

	return true;
}

/* Overwrite a character with 'c' in 'line' at position 'pos' */
bool text_line_t::overwrite_char(int pos, key_t c, undo_t *undo) {
	int oldspace;
	text_line_t *undo_text, *replacement_text;

	convert_key(c);

	/* Zero-width characters don't overwrite, only insert. */
	if ((conversion_meta_data & WIDTH_MASK) == 0) {
		if (pos == 0)
			return false;

		if (undo != NULL) {
			ASSERT(undo->get_type() == UNDO_OVERWRITE);
			replacement_text = undo->get_replacement();
			replacement_text->reserve(replacement_text->buffer.size() + conversion_length);
			replacement_text->insert_bytes(replacement_text->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
		}
		return insert_char(pos, c, NULL);
	}

	if (starts_with_combining && pos == 0)
		starts_with_combining = false;

	oldspace = adjust_position(pos, 1) - pos;
	if (oldspace < conversion_length)
		reserve(buffer.size() + conversion_length - oldspace);

	if (undo != NULL) {
		ASSERT(undo->get_type() == UNDO_OVERWRITE);
		undo_text = undo->get_text();
		undo_text->reserve(undo_text->buffer.size() + oldspace);
		replacement_text = undo->get_replacement();
		replacement_text->reserve(replacement_text->buffer.size() + conversion_length);

		undo_text->insert_bytes(undo_text->buffer.size(), buffer.data() + pos, oldspace, meta_buffer[pos]);
		replacement_text->insert_bytes(replacement_text->buffer.size(), conversion_buffer, conversion_length, conversion_meta_data);
	}

	buffer.replace(pos, oldspace, conversion_buffer, conversion_length);
	meta_buffer.replace(pos, oldspace, conversion_length, 0);
	meta_buffer[pos] = conversion_meta_data;
	check_bad_draw(pos);
	return true;
}

/* Delete the character at position 'pos' */
int text_line_t::delete_char(int pos, undo_t *undo) {
	int oldspace;

	if (pos < 0 || (size_t) pos >= buffer.size())
		return -1;

	if (starts_with_combining && pos == 0)
		starts_with_combining = false;

	oldspace = adjust_position(pos, 1) - pos;

	if (undo != NULL) {
		text_line_t *undo_text = undo->get_text();
		undo_text->reserve(oldspace);

		ASSERT(undo->get_type() == UNDO_DELETE || undo->get_type() == UNDO_BACKSPACE);
		undo_text->insert_bytes(undo->get_type() == UNDO_DELETE ? undo_text->buffer.size() : 0, buffer.data() + pos, oldspace, meta_buffer[pos]);
	}

	buffer.erase(pos, oldspace);
	meta_buffer.erase(pos, oldspace);

	return 0;
}

/* Append character 'c' to 'line' */
bool text_line_t::append_char(key_t c, undo_t *undo) {
	return insert_char(buffer.size(), c, undo);
}

/* Delete char at 'pos - 1' */
int text_line_t::backspace_char(int pos, undo_t *undo) {
	if (pos == 0)
		return -1;
	return delete_char(adjust_position(pos, -1), undo);
}

/*
void text_line_t::write_line_data(FileWriteWrapper *file, bool appendNewline) const {
	file->write(buffer.data(), buffer.size());

	if (appendNewline)
		file->write("\n", 1);
}
*/

/** Adjust the line position @a adjust non-zero-width characters.
	@param line The @a text_line_t to operate on.
	@param pos The starting position.
	@param adjust How many characters to adjust.

	This function finds the next (previous) point in the line at which the
    cursor could be. This means skipping all zero-width characters between the
	current position and the next non-zero-width character, and repeating for
	@a adjust times.
*/
int text_line_t::adjust_position(int pos, int adjust) const {
	if (adjust > 0) {
		for (; adjust > 0 && (size_t) pos < buffer.size(); adjust -= (width_at(pos) ? 1 : 0))
			pos += byte_width_from_first(pos);
	} else {
		for (; adjust < 0 && pos > 0; adjust += (width_at(pos) ? 1 : 0)) {
			pos--;
			while (pos > 0 && !width_at(pos))
				pos--;
		}
	}
	return pos;
}

int text_line_t::get_length(void) const {
	return buffer.size();
}

int text_line_t::byte_width_from_first(int pos) const {
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
}

int text_line_t::width_at(int pos) const { return meta_buffer[pos] & WIDTH_MASK; }
int text_line_t::is_print(int pos) const { return (meta_buffer[pos] & (GRAPH_BIT | SPACE_BIT)) != 0; }
int text_line_t::is_graph(int pos) const { return meta_buffer[pos] & GRAPH_BIT; }
int text_line_t::is_alnum(int pos) const { return meta_buffer[pos] & ALNUM_BIT; }
int text_line_t::is_space(int pos) const { return meta_buffer[pos] & SPACE_BIT; }
int text_line_t::is_bad_draw(int pos) const { return meta_buffer[pos] & BAD_DRAW_BIT; }

const string *text_line_t::get_data(void) const {
	return &buffer;
}

void text_line_t::init(void) {
	memset(spaces, ' ', sizeof(spaces));
	memset(dots, '.', sizeof(dots));
	if (!t3_term_can_draw(wrap_symbol, strlen(wrap_symbol)))
		wrap_symbol = "$";
}

void text_line_t::reserve(int size) {
	buffer.reserve(size);
	meta_buffer.reserve(size);
}

int text_line_t::callout(pcre_callout_block *block) {
	find_context_t *context = (find_context_t *) block->callout_data;
	if (block->pattern_position != context->pattern_length)
		return 0;

	if (context->flags & find_flags_t::BACKWARD) {
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

bool text_line_t::check_boundaries(int match_start, int match_end) const {
	return (match_start == 0 || get_class(match_start) != get_class(adjust_position(match_start, -1))) &&
							(match_end == get_length() || get_class(match_end) != get_class(adjust_position(match_end, 1)));
}

#if 0
bool text_line_t::find(find_context_t *context, find_result_t *result) const {
	string substr;
	int curr_char, next_char;
	int match_result;
	int start, end;
	size_t c_size;
	const char *c;

	if (context->flags & find_flags_t::REGEX) {
		pcre_extra extra;
		int pcre_flags = PCRE_NOTEMPTY;
		int ovector[30];

		context->ovector[0] = -1;
		context->ovector[1] = -1;
		context->found = false;

		if (context->flags & find_flags_t::BACKWARD) {
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
		match_result = pcre_exec(context->regex, &extra, buffer.data(), end, start, pcre_flags,
			(int *) ovector, sizeof(ovector) / sizeof(ovector[0]));
		if (!context->found)
			return false;
		result->start = context->ovector[0];
		result->end = context->ovector[1];
		return true;
	}

	start = result->start >= 0 && (size_t) result->start > buffer.size() ? buffer.size() : result->start;
	curr_char = start;

	if (context->flags & find_flags_t::BACKWARD) {
		context->matcher->reset();
		while((size_t) curr_char > 0) {
			next_char = adjust_position(curr_char, -1);
			substr.clear();
			substr = buffer.substr(next_char, curr_char - next_char);
			if (context->flags & find_flags_t::ICASE) {
				c_size = t3_unicode_casefold(substr.data(), substr.size(), &context->folded, &context->folded_size, t3_false);
				c = context->folded;
			} else {
				c = substr.data();
				c_size = substr.size();
			}
			match_result = context->matcher->previous_char(c, c_size);
			if (match_result >= 0 &&
					(!(context->flags & find_flags_t::WHOLE_WORD) || check_boundaries(next_char, adjust_position(start, -match_result)))) {
				result->end = adjust_position(start, -match_result);
				result->start = next_char;
				return true;
			}
			curr_char = next_char;
		}
		return false;
	} else {
		context->matcher->reset();
		while((size_t) curr_char < buffer.size()) {
			next_char = adjust_position(curr_char, 1);
			substr.clear();
			substr = buffer.substr(curr_char, next_char - curr_char);
			if (context->flags & find_flags_t::ICASE) {
				c_size = t3_unicode_casefold(substr.data(), substr.size(), &context->folded, &context->folded_size, t3_false);
				c = context->folded;
			} else {
				c = substr.data();
				c_size = substr.size();
			}
			match_result = context->matcher->next_char(c, c_size);
			if (match_result >= 0 &&
					(!(context->flags & find_flags_t::WHOLE_WORD) || check_boundaries(adjust_position(start, match_result), next_char))) {
				result->start = adjust_position(start, match_result);
				result->end = next_char;
				return true;
			}
			curr_char = next_char;
		}
		return false;
	}
}
#endif

void text_line_t::check_bad_draw(int i) {
	if (t3_term_can_draw(buffer.data() + i, adjust_position(i, 1) - i))
		meta_buffer[i] &= ~BAD_DRAW_BIT;
	else
		meta_buffer[i] |= BAD_DRAW_BIT;
}

}; // namespace
