/* Copyright (C) 2011-2012 G.P. Halkes
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
#include <t3window/utf8.h>

#include "findcontext.h"
#include "colorscheme.h"
#include "textline.h"
#include "util.h"
#include "undo.h"
#include "stringmatcher.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

char text_line_t::spaces[_T3_MAX_TAB];
char text_line_t::dashes[_T3_MAX_TAB];
char text_line_t::dots[16];
const char *text_line_t::control_map = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]_^";
const char *text_line_t::wrap_symbol = "\xE2\x86\xB5";

text_line_factory_t default_text_line_factory;

text_line_t::text_line_t(int buffersize, text_line_factory_t *_factory) : starts_with_combining(false),
		factory(_factory == NULL ? &default_text_line_factory : _factory)
{
	reserve(buffersize);
}

void text_line_t::fill_line(const char *_buffer, int length) {
	size_t char_bytes, round_trip_bytes;
	key_t next;
	char byte_buffer[5];

	/* If _buffer is valid UTF-8, we will end up with a buffer of size length.
	   So just tell the buffer that, such that it can allocate an appropriately
	   sized buffer. */
	reserve(length);

	while (length > 0) {
		char_bytes = length;
		next = t3_utf8_get(_buffer, &char_bytes);
		round_trip_bytes = t3_utf8_put(next, byte_buffer);
		buffer.append(byte_buffer, round_trip_bytes);
		length -= char_bytes;
		_buffer += char_bytes;
	}
	starts_with_combining = buffer.size() > 0 && width_at(0) == 0;
}

text_line_t::text_line_t(const char *_buffer, text_line_factory_t *_factory) : starts_with_combining(false),
		factory(_factory == NULL ? &default_text_line_factory : _factory)
{
	fill_line(_buffer, strlen(_buffer));
}

text_line_t::text_line_t(const char *_buffer, int length, text_line_factory_t *_factory) :
		starts_with_combining(false), factory(_factory == NULL ? &default_text_line_factory : _factory)
{
	fill_line(_buffer, length);
}

text_line_t::text_line_t(const string *str, text_line_factory_t *_factory) : starts_with_combining(false),
		factory(_factory == NULL ? &default_text_line_factory : _factory)
{
	fill_line(str->data(), str->size());
}

void text_line_t::set_text(const char *_buffer) {
	buffer.clear();
	fill_line(_buffer, strlen(_buffer));
}

void text_line_t::set_text(const char *_buffer, size_t length) {
	buffer.clear();
	fill_line(_buffer, length);
}

void text_line_t::set_text(const string *str) {
	buffer.clear();
	fill_line(str->data(), str->size());
}

/* Merge line2 into line1, freeing line2 */
void text_line_t::merge(text_line_t *other) {
	int buffer_len = buffer.size();

	if (buffer_len == 0 && other->starts_with_combining)
		starts_with_combining = true;

	reserve(buffer.size() + other->buffer.size());

	buffer += other->buffer;
	delete other;
}

/* Break up 'line' at position 'pos'. This means that the character at 'pos'
   is the first character in the new line. The right part of the line is
   returned the left part remains in 'line' */
text_line_t *text_line_t::break_line(int pos) {
	text_line_t *newline;

	//FIXME: cut_line and break_line are very similar. Maybe we should combine them!
	if ((size_t) pos == buffer.size())
		return factory->new_text_line_t();

	/* Only allow line breaks at non-combining marks. */
	ASSERT(width_at(pos));

	/* copy the right part of the string into the new buffer */
	newline = factory->new_text_line_t(buffer.size() - pos);
	newline->buffer.assign(buffer.data() + pos, buffer.size() - pos);

	buffer.resize(pos);
	return newline;
}

text_line_t *text_line_t::cut_line(int start, int end) {
	text_line_t *retval;

	ASSERT((size_t) end == buffer.size() || width_at(end) > 0);
	//FIXME: special case: if the selection cover a whole text_line_t (note: not wrapped) we shouldn't copy

	retval = clone(start, end);

	buffer.erase(start, end - start);
	starts_with_combining = buffer.size() > 0 && width_at(0) == 0;

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
		return factory->new_text_line_t(0);

	retval = factory->new_text_line_t(end - start);

	retval->buffer.assign(buffer.data() + start, end - start);
	retval->starts_with_combining = width_at(start) == 0;

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

	*startFrom = (size_t) i == buffer.size() ? -1 : i + 1;
	return retval;
}

void text_line_t::insert(text_line_t *other, int pos) {
	ASSERT(pos >= 0 && (size_t) pos <= buffer.size());

	reserve(buffer.size() + other->buffer.size());
	buffer.insert(pos, other->buffer);
	if (pos == 0)
		starts_with_combining = other->starts_with_combining;
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
		selection_attr = t3_term_combine_attrs(attributes.non_print, selection_attr);
		for (; (size_t) todo > sizeof(dots); todo -= sizeof(dots))
			t3_win_addnstr(win, dots, sizeof(dots), selection_attr);
		t3_win_addnstr(win, dots, todo, selection_attr);
	}
}

t3_attr_t text_line_t::get_base_attr(int i, const text_line_t::paint_info_t *info) {
	(void) i;
	return info->normal_attr;
}

t3_attr_t text_line_t::get_draw_attrs(int i, const text_line_t::paint_info_t *info) {
	t3_attr_t retval = get_base_attr(i, info);

	if (i >= info->selection_start && i < info->selection_end)
		retval = i == info->cursor ? t3_term_combine_attrs(attributes.selection_cursor2, retval) : info->selected_attr;
	else if (i == info->cursor)
		retval = t3_term_combine_attrs(i == info->selection_end ? attributes.selection_cursor : attributes.text_cursor, retval);

	if (is_bad_draw(i))
		retval = t3_term_combine_attrs(attributes.bad_draw, retval);

	return retval;
}

void text_line_t::paint_line(t3_window_t *win, const text_line_t::paint_info_t *info) {
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
			if (total > info->leftcol) {
				if (flags & text_line_t::SHOW_TABS) {
					selection_attr = t3_term_combine_attrs(selection_attr, attributes.meta_text);
					if (total - info->leftcol > 1)
						t3_win_addnstr(win, dashes, total - info->leftcol - 1, selection_attr);
					t3_win_addch(win, '>', selection_attr);
				} else {
					t3_win_addnstr(win, spaces, total - info->leftcol, selection_attr);
				}
			}
		} else if ((unsigned char) buffer[i] < 32) {
			total += 2;
			// If total > info->leftcol than only the right side character is visible
			if (total > info->leftcol)
				t3_win_addch(win, control_map[(int) buffer[i]], t3_term_combine_attrs(attributes.non_print, selection_attr));
		} else if (width_at(i) > 1) {
			total += width_at(i);
			if (total > info->leftcol) {
				for (j = info->leftcol; j < total; j++)
					t3_win_addch(win, '<',  t3_term_combine_attrs(attributes.non_print, selection_attr));
			}
		} else {
			total += width_at(i);
		}
	}

	if (starts_with_combining && info->leftcol == 0 && info->start == 0) {
		selection_attr = get_draw_attrs(0, info);
		paint_part(win, " ", true, 1, t3_term_combine_attrs(attributes.non_print, selection_attr));
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
			/*if (total + tabspaces >= size)
				tabspaces = size - total;*/
			if (i == info->cursor) {
				char cursor_char = (flags & text_line_t::SHOW_TABS) ? (tabspaces > 1 ? '<' : '>') : ' ';
				t3_win_addch(win, cursor_char, selection_attr);
				tabspaces--;
				total++;
				selection_attr = i >= info->selection_start && i < info->selection_end ? info->selected_attr : info->normal_attr;
			}
			if (tabspaces > 0) {
				if (flags & text_line_t::SHOW_TABS) {
					selection_attr = t3_term_combine_attrs(selection_attr, attributes.meta_text);
					if (tabspaces > 1)
						t3_win_addch(win, i == info->cursor ? '-' : '<', selection_attr);
					if (tabspaces > 2)
						t3_win_addnstr(win, dashes, tabspaces - 2, selection_attr);
					t3_win_addch(win, '>', selection_attr);
				} else {
					t3_win_addnstr(win, spaces, tabspaces, selection_attr);
				}
			}
			total += tabspaces;
			print_from = i + 1;
		} else if ((unsigned char) buffer[i] < 32) {
			/* Print control characters as ^ followed by a letter indicating the control char. */
			paint_part(win, buffer.data() + print_from, _is_print, _is_print ? i - print_from : accumulated, selection_attr);
			total += accumulated;
			accumulated = 0;
			t3_win_addch(win, '^', t3_term_combine_attrs(attributes.non_print, selection_attr));
			total += 2;
			if (total <= size)
				t3_win_addch(win, control_map[(int) buffer[i]], t3_term_combine_attrs(attributes.non_print, selection_attr));
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
		t3_win_addch(win, '>', t3_term_combine_attrs(attributes.non_print, selection_attr));
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
		t3_win_addstr(win, wrap_symbol, t3_term_combine_attrs(attributes.meta_text, info->normal_attr));
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
text_line_t::break_pos_t text_line_t::find_next_break_pos(int start, int length, int tabsize) const {
	int i, total = 0, cclass;
	break_pos_t possible_break = { start, 0 };
	bool graph_seen = false, last_was_graph = false;

	if (starts_with_combining && start == 0)
		total++;

	for (i = start; (size_t) i < buffer.size() && total < length; i = adjust_position(i, 1)) {
		if (buffer[i] == '\t')
			total += tabsize > 0 ? tabsize - (total % tabsize) : 2;
		else
			total += width_at(i);

		if (total > length) {
			if (possible_break.pos == start)
				possible_break.flags = text_line_t::PARTIAL_CHAR;
			break;
		}

		cclass = get_class(&buffer, i);
		if (buffer[i] < 32 && (buffer[i] != '\t' || tabsize == 0))
			cclass = CLASS_GRAPH;

		if (!graph_seen) {
			if (cclass == CLASS_ALNUM || cclass == CLASS_GRAPH) {
				graph_seen = true;
				last_was_graph = true;
			}
			possible_break.pos = i;
		} else if (cclass == CLASS_WHITESPACE && last_was_graph) {
			possible_break.pos = adjust_position(i, 1);
			last_was_graph = false;
		} else if (cclass == CLASS_ALNUM || cclass == CLASS_GRAPH) {
			last_was_graph = true;
		}
	}

	if ((size_t) i < buffer.size()) {
		if (possible_break.pos == start)
			possible_break.pos = i;
		possible_break.flags |= text_line_t::BREAK;
		return possible_break;
	}
	possible_break.flags = 0;
	possible_break.pos = -1;
	return possible_break;
}

int text_line_t::get_next_word(int start) const {
	int i, cclass, newCclass;

	if (start < 0) {
		start = 0;
		cclass = CLASS_WHITESPACE;
	} else {
		cclass = get_class(&buffer, start);
		start = adjust_position(start, 1);
	}

	for (i = start;
			(size_t) i < buffer.size() && ((newCclass = get_class(&buffer, i)) == cclass || newCclass == CLASS_WHITESPACE);
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
			i > 0 && (cclass = get_class(&buffer, i)) == CLASS_WHITESPACE;
			i = adjust_position(i, -1))
	{}

	if (i == 0 && cclass == CLASS_WHITESPACE)
		return -1;

	savePos = i;

	for (i = adjust_position(i, -1);
			i > 0 && get_class(&buffer, i) == cclass;
			i = adjust_position(i, -1))
	{
		savePos = i;
	}

	if (i == 0 && get_class(&buffer, i) == cclass)
		savePos = i;

	return cclass != CLASS_WHITESPACE ? savePos : -1;
}


void text_line_t::insert_bytes(int pos, const char *bytes, int space) {
	buffer.insert(pos, bytes, space);
}


/* Insert character 'c' into 'line' at position 'pos' */
bool text_line_t::insert_char(int pos, key_t c, undo_t *undo) {
	char conversion_buffer[5];
	int conversion_length;

	conversion_length = t3_utf8_put(c, conversion_buffer);

	reserve(buffer.size() + conversion_length + 1);

	if (undo != NULL) {
		string *undo_text = undo->get_text();
		undo_text->reserve(undo_text->size() + conversion_length);

		ASSERT(undo->get_type() == UNDO_ADD);
		undo_text->append(conversion_buffer, conversion_length);
	}

	if (pos == 0)
		starts_with_combining = key_width(c) == 0;

	insert_bytes(pos, conversion_buffer, conversion_length);
	return true;
}

/* Overwrite a character with 'c' in 'line' at position 'pos' */
bool text_line_t::overwrite_char(int pos, key_t c, undo_t *undo) {
	int oldspace;
	string *undo_text, *replacement_text;
	char conversion_buffer[5];
	int conversion_length;

	conversion_length = t3_utf8_put(c, conversion_buffer);

	/* Zero-width characters don't overwrite, only insert. */
	if (key_width(c) == 0) {
		//FIXME: shouldn't this simply insert and set starts_with_combining to true?
		if (pos == 0)
			return false;

		if (undo != NULL) {
			ASSERT(undo->get_type() == UNDO_OVERWRITE);
			replacement_text = undo->get_replacement();
			replacement_text->reserve(replacement_text->size() + conversion_length);
			replacement_text->append(conversion_buffer, conversion_length);
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
		undo_text->reserve(undo_text->size() + oldspace);
		replacement_text = undo->get_replacement();
		replacement_text->reserve(replacement_text->size() + conversion_length);

		undo_text->append(buffer.data() + pos, oldspace);
		replacement_text->append(conversion_buffer, conversion_length);
	}

	buffer.replace(pos, oldspace, conversion_buffer, conversion_length);
	return true;
}

/* Delete the character at position 'pos' */
bool text_line_t::delete_char(int pos, undo_t *undo) {
	int oldspace;

	if (pos < 0 || (size_t) pos >= buffer.size())
		return false;

	if (starts_with_combining && pos == 0)
		starts_with_combining = false;

	oldspace = adjust_position(pos, 1) - pos;

	if (undo != NULL) {
		string *undo_text = undo->get_text();
		undo_text->reserve(oldspace);

		ASSERT(undo->get_type() == UNDO_DELETE || undo->get_type() == UNDO_BACKSPACE);
		undo_text->insert(undo->get_type() == UNDO_DELETE ? undo_text->size() : 0, buffer.data() + pos, oldspace);
	}

	buffer.erase(pos, oldspace);
	return true;
}

/* Append character 'c' to 'line' */
bool text_line_t::append_char(key_t c, undo_t *undo) {
	return insert_char(buffer.size(), c, undo);
}

/* Delete char at 'pos - 1' */
bool text_line_t::backspace_char(int pos, undo_t *undo) {
	if (pos == 0)
		return false;
	return delete_char(adjust_position(pos, -1), undo);
}

/** Adjust the line position @a adjust non-zero-width characters.
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
			while (pos > 0 && (buffer[pos] & 0xc0) == 0x80)
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

int text_line_t::key_width(key_t key) {
	int width = t3_utf8_wcwidth((uint32_t) key);
	if (width < 0)
		width = key < 32 && key != '\t' ? 2 : 1;
	return width;
}
int text_line_t::width_at(int pos) const {
	return key_width(t3_utf8_get(buffer.data() + pos, NULL));
}
bool text_line_t::is_print(int pos) const {
	return buffer[pos] == '\t' || !uc_is_general_category_withtable(t3_utf8_get(buffer.data() + pos, NULL), T3_UTF8_CONTROL_MASK);
}
bool text_line_t::is_alnum(int pos) const {
	return get_class(&buffer, pos) == CLASS_ALNUM;
}
bool text_line_t::is_space(int pos) const {
	return get_class(&buffer, pos) == CLASS_WHITESPACE;
}
bool text_line_t::is_bad_draw(int pos) const {
	return !t3_term_can_draw(buffer.data() + pos, adjust_position(pos, 1) - pos);
}

const string *text_line_t::get_data(void) const {
	return &buffer;
}

void text_line_t::init(void) {
	memset(spaces, ' ', sizeof(spaces));
	memset(dashes, '-', sizeof(dashes));
	memset(dots, '.', sizeof(dots));
	if (!t3_term_can_draw(wrap_symbol, strlen(wrap_symbol)))
		wrap_symbol = "$";
}

void text_line_t::reserve(int size) {
	buffer.reserve(size);
}

bool text_line_t::check_boundaries(int match_start, int match_end) const {
	return (match_start == 0 || get_class(&buffer, match_start) != get_class(&buffer, adjust_position(match_start, -1))) &&
		(match_end == get_length() || get_class(&buffer, match_end) != get_class(&buffer, adjust_position(match_end, 1)));
}

//============================= text_line_factory_t ========================

text_line_factory_t::text_line_factory_t(void) {}
text_line_factory_t::~text_line_factory_t(void) {}
text_line_t *text_line_factory_t::new_text_line_t(int buffersize) { return new text_line_t(buffersize, this); }
text_line_t *text_line_factory_t::new_text_line_t(const char *_buffer) { return new text_line_t(_buffer, this); }
text_line_t *text_line_factory_t::new_text_line_t(const char *_buffer, int length) { return new text_line_t(_buffer, length, this); }
text_line_t *text_line_factory_t::new_text_line_t(const std::string *str) { return new text_line_t(str, this); }

}; // namespace
